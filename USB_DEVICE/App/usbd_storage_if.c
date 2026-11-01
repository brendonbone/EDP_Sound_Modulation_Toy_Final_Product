/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_storage_if.c
  * @version        : v1.0_Cube
  * @brief          : Memory management layer.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* TEAM CODE (owner: Brendon): ownership note for this file (the ST text above is the CubeMX header) */
/*
 * Owner: Brendon Bone — USB-C MSC <-> SD-card glue for the Battery/USB-C
 * subsystem. CubeMX generated the STORAGE_*_FS skeleton; the bodies inside the
 * USER CODE blocks below (SD-backed capacity/ready/read/write, plus the
 * thread-context STORAGE_SD_Poll bring-up I added) are mine. This is the device
 * end of PD2 reqs 2.14-2.16 (enumerate as MSC, drag-and-drop WAV transfer) and
 * 2.21 (safe disconnect: when the PC goes, BrendonTask gives the card back to
 * JackStorageTask and stops the USB stack, and shows "Transfer aborted" if the
 * PC wrote to the card but never ejected it).
 * Everything is kept inside USER CODE markers so a CubeMX regen won't wipe it.
 *
 * Words used here:
 *   MSC  USB Mass Storage Class: the standard way for a USB device to look
 *        like a drive, so the PC needs no driver of ours.
 *   SCSI the disk commands the PC sends inside MSC. ST's MSC class
 *        (usbd_msc_scsi.c) decodes them and calls the functions below:
 *        TEST UNIT READY -> STORAGE_IsReady_FS, READ CAPACITY ->
 *        STORAGE_GetCapacity_FS, READ(10) -> STORAGE_Read_FS, WRITE(10) ->
 *        STORAGE_Write_FS. START STOP UNIT (eject) only sets the class's
 *        own medium state, which BrendonTask reads.
 *   LUN  logical unit number: one drive inside the device. This one has a
 *        single LUN (0 = the SD card), so every lun argument is ignored.
 *   PD2 req 2.14 = requirement 2.14 of the team's Prototype Design 2
 *        document; PD5b = the host-tested modules, PD6 = the RTOS version.
 *
 * Where these run: every STORAGE_*_FS function is called from the OTG_FS
 * (USB) interrupt, not from a task, so none of them may call os* or log.
 *
 * The hand-over gate, g_msc_media_ready (app_rtos.h): the card has two
 * possible users, JackStorageTask (FatFs) and the PC (these functions), and
 * they must never use it at the same time. Jack sets the flag only after he
 * has unmounted (JACK_CMD_ENTER_MSC) and clears it before he mounts again
 * (JACK_CMD_EXIT_MSC). While it is 0 every function below answers at once,
 * without touching the card. A plain flag is enough: each of these functions
 * runs to its end inside the interrupt before any task runs again, so Jack
 * cannot change the flag while one of them is using the card.
 *
 * A PC session, step by step:
 *   1. cable in: BrendonTask starts the USB stack (usb_gate) and the PC
 *      enumerates the toy; while the gate is 0, IsReady says "no medium"
 *   2. "configured" for 500 ms: BrendonTask sends ENTER_MSC, Jack unmounts
 *      and opens the gate
 *   3. IsReady says ready once the card answers; the PC reads the capacity
 *      and shows the drive
 *   4. copying: STORAGE_Read_FS / STORAGE_Write_FS (successful writes are
 *      counted in g_msc_host_writes)
 *   5. cable out: BrendonTask sends EXIT_MSC (Jack closes the gate and
 *      mounts the card again) and the USB stack is stopped
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"

/* USER CODE BEGIN INCLUDE */
/* TEAM CODE (owner: Brendon): headers and the SD handle for the USB-MSC to SD glue */
#include "bsp_driver_sd.h"
#include "main.h"
#include "app_rtos.h"          /* g_msc_media_ready: the "eject-to-use" hand-over flag */
#include "brendon_ports.h"     /* MSC_Store (PD5b msc_store) backed by HAL_SD_* */
#include "msc_store.h"
extern SD_HandleTypeDef hsd;   /* defined in main.c by CubeMX */
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* TEAM CODE (owner: Brendon): note on where the MSC block logic lives (PD5b msc_store) */
/* Private variables ---------------------------------------------------------*/

/* PD6: the bring-up rate limit, readiness, capacity and block I/O now live in
 * the host-tested PD5b msc_store module (MSC_Poll/MSC_IsReady/MSC_GetCapacity/
 * MSC_ReadBlocks/MSC_WriteBlocks) with a BOUNDED post-I/O wait. */

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @defgroup USBD_STORAGE
  * @brief Usb mass storage device module
  * @{
  */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Defines
  * @brief Private defines.
  * @{
  */

#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  0x10000
#define STORAGE_BLK_SIZ                  0x200

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN INQUIRY_DATA_FS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_FS[] = {/* 36 */

  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,
  0x00,
  'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  '0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_FS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS =
{
  STORAGE_Init_FS,
  STORAGE_GetCapacity_FS,
  STORAGE_IsReady_FS,
  STORAGE_IsWriteProtected_FS,
  STORAGE_Read_FS,
  STORAGE_Write_FS,
  STORAGE_GetMaxLun_FS,
  (int8_t *)STORAGE_Inquirydata_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the storage unit (medium) over USB FS IP
  * @param  lun: Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Init_FS(uint8_t lun)
{
  /* USER CODE BEGIN 2 */
  /* TEAM CODE (owner: Brendon): comment only - why the SD card is not initialised here */
  UNUSED(lun);
  /* Nothing to do here. The SD card is brought up in THREAD context by
   * MSC_Poll() in BrendonTask (brendon_battery_usb.c), NOT here: this callback
   * runs in the USB interrupt, and HAL_SD_Init() uses HAL_Delay()/HAL_GetTick().
   * The HAL tick comes from the TIM5 interrupt (priority 15), which cannot
   * preempt OTG_FS (priority 6), so the tick would never advance in here
   * -> the MCU would hang during enumeration. STORAGE_IsReady_FS() reports
   * the card once it is up. */
  return (USBD_OK);
  /* USER CODE END 2 */
}

/**
  * @brief  Returns the medium capacity.
  * @param  lun: Logical unit number.
  * @param  block_num: Number of total block number.
  * @param  block_size: Block size.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
  /* USER CODE BEGIN 3 */
  /* TEAM CODE (owner: Brendon): report the SD card's capacity, behind the hand-over gate */
  UNUSED(lun);
  /* Same hand-over gate as IsReady/Read/Write: until JACK_CMD_ENTER_MSC has
   * given the card to the host, JackStorageTask owns the SDIO bus, and
   * MSC_GetCapacity() would send a CMD13 (sd_get_state) from this OTG_FS
   * interrupt in the middle of whatever command the task is running. A host
   * asks for the capacity (READ CAPACITY / READ FORMAT CAPACITIES) as soon as
   * it enumerates, before usb_watch's hand-over. Answer with the fallback
   * geometry without touching the card, exactly what a not-ready card
   * reported before; IsReady keeps saying "no medium" until the hand-over,
   * and the host reads the real capacity after that. */
  if (!g_msc_media_ready) {
    *block_num  = STORAGE_BLK_NBR;
    *block_size = STORAGE_BLK_SIZ;
    return (USBD_OK);
  }
  // Tell the USB host the card's size: true geometry when ready, else the fallback constants (MSC_FALLBACK) so enumeration survives.
  if (MSC_GetCapacity(BrendonPorts_MscStore(), block_num, block_size) == MSC_BAD_PARAM) {
    *block_num  = STORAGE_BLK_NBR;
    *block_size = STORAGE_BLK_SIZ;
  }
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief   Checks whether the medium is ready.
  * @param  lun:  Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsReady_FS(uint8_t lun)
{
  /* USER CODE BEGIN 4 */
  /* TEAM CODE (owner: Brendon): report the card ready only after the hand-over */
  UNUSED(lun);
  //It reports the SD card as "ready" to the USB host only if the card has finished initialising and is sitting idle, and "not ready" otherwise.
  /* RTOS "eject-to-use" hand-over (spec section 7): JackStorageTask owns the
   * card. The medium is only offered to the host after JACK_CMD_ENTER_MSC has
   * unmounted FATFS and set g_msc_media_ready; until then the host sees
   * "no medium" and never races JackStorageTask (recording, playback, effect
   * files) for the SDIO bus. */
  if (!g_msc_media_ready) {
    return (USBD_FAIL);
  }
  return MSC_IsReady(BrendonPorts_MscStore()) ? (USBD_OK) : (USBD_FAIL);
  /* USER CODE END 4 */
}

/**
  * @brief  Checks whether the medium is write protected.
  * @param  lun: Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
  /* USER CODE BEGIN 5 */
  UNUSED(lun);

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Reads data from the medium.
  * @param  lun: Logical unit number.
  * @param  buf: data buffer.
  * @param  blk_addr: Logical block address.
  * @param  blk_len: Blocks number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 6 */
  /* TEAM CODE (owner: Brendon): read blocks from the SD card, behind the hand-over gate */
  UNUSED(lun);

  if (!g_msc_media_ready) {
    return (USBD_FAIL);    /* card still belongs to FATFS / JackStorageTask */
  }
  // Read the blocks the host asked for; msc_store waits (BOUNDED) for the card to go idle again.
  return (MSC_ReadBlocks(BrendonPorts_MscStore(), buf, blk_addr, blk_len) == MSC_OK) ? (USBD_OK) : (USBD_FAIL);
  /* USER CODE END 6 */
}

/**
  * @brief  Writes data into the medium.
  * @param  lun: Logical unit number.
  * @param  buf: data buffer.
  * @param  blk_addr: Logical block address.
  * @param  blk_len: Blocks number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 7 */
  /* TEAM CODE (owner: Brendon): write blocks to the SD card, behind the hand-over gate */
  UNUSED(lun);

  // writes the data blocks the USB host sent into the SD card, waits for the card to finish committing them to flash, and reports success (or failure if the write didn't work).
  if (!g_msc_media_ready) {
    return (USBD_FAIL);    /* card still belongs to FATFS / JackStorageTask */
  }
  // Write the host's blocks; msc_store waits (BOUNDED) for the card to finish programming.
  if (MSC_WriteBlocks(BrendonPorts_MscStore(), buf, blk_addr, blk_len) != MSC_OK) {
    return (USBD_FAIL);
  }
  g_msc_host_writes++;   /* ISR: count only; BrendonTask checks it when the host leaves (brendon_ports.h) */
  return (USBD_OK);
  /* USER CODE END 7 */
}

/**
  * @brief  Returns the Max Supported LUNs.
  * @param  None
  * @retval Lun(s) number.
  */
int8_t STORAGE_GetMaxLun_FS(void)
{
  /* USER CODE BEGIN 8 */
  return (STORAGE_LUN_NBR - 1);
  /* USER CODE END 8 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
/* TEAM CODE (owner: Brendon): STORAGE_SD_Poll(), the thread-context SD bring-up */


/* Thread-context SD bring-up for the MSC medium. Nothing calls it at the
 * moment: BrendonTask calls MSC_Poll() itself (brendon_battery_usb.c), only
 * while the card has been handed over, which is the same thing. Rate limit,
 * busy check and the best-effort 4-bit upgrade are in the PD5b msc_store
 * module. */
void STORAGE_SD_Poll(void)
{
  (void)MSC_Poll(BrendonPorts_MscStore(), HAL_GetTick());
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

