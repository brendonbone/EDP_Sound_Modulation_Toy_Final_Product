/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v1.0_Cube
  * @brief          : This file implements the USB Device
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB device Library, add supported class and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */
  /* TEAM CODE (owner: Brendon): comment only - what the USB-MSC bring-up calls below do */
  /* ===== Brendon: USB-MSC device bring-up =====
   * The four calls below stand up the device side of the USB-C link (PD2 reqs
   * 2.13-2.16):
   *   USBD_Init                 binds the OTG-FS low-level driver (usbd_conf.c)
   *                             and my descriptors (FS_Desc, usbd_desc.c);
   *   USBD_RegisterClass        installs ST's Mass-Storage Class onto the device;
   *   USBD_MSC_RegisterStorage  points that class at MY block interface
   *                             (USBD_Storage_Interface_fops_FS in
   *                             usbd_storage_if.c, backed by the microSD);
   *   USBD_Start                enables OTG-FS so the host can enumerate.
   * The SD medium itself is brought up lazily later by MSC_Poll() in
   * BrendonTask, once JackStorageTask has handed the card over — see
   * usbd_storage_if.c for why it cannot happen inside the USB interrupt.
   *
   * When this function runs: not at boot (the .ioc has "Do Not Generate
   * Function Call" ticked for it). BrendonTask's usb_gate
   * (brendon_battery_usb.c) calls it only while VBUS (PA9, the USB cable's
   * 5 V line) is high, and stops the stack again with USBD_DeInit() when VBUS
   * goes, the PC leaves, or no PC configures it within 3 s. Why: USBD_Start
   * switches on the D+ pull-up that tells a PC "device plugged in", and a
   * self-powered device must not do that without VBUS (ST's AN4879).
   * (PD2 reqs = requirement numbers from the team's Prototype Design 2.) */
  /* USER CODE END USB_DEVICE_Init_PreTreatment */

  /* Init Device Library, add supported class and start the library. */
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */

  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */

