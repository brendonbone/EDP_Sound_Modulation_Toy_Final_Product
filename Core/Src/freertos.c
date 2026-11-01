/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* TEAM CODE (integration: Brendon): HAL header for the stack-overflow hook's GPIO calls */
/* The team's only code in this CubeMX file is the stack-overflow hook
 * (USER CODE 4) and the variable it fills. The tasks are created in main.c,
 * the queues in app_rtos.c. */
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* TEAM CODE (integration: Brendon): name of the task that overflowed, for the debugger */
/* Name of the task that overflowed its stack — read it in the debugger
 * (Live Expressions / Watch) after the board halts with LD2 flashing. */
volatile const char *g_stack_overflow_task = NULL;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
/* TEAM CODE (integration: Brendon): stack-overflow hook body (stop, save the task name, blink LD2); the function shell and its first comment are the CubeMX default */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
   (void)xTask;
   /* How FreeRTOS notices: configCHECK_FOR_STACK_OVERFLOW is 2 here
    * (FreeRTOSConfig.h). Each task's stack is filled with the byte 0xA5 when
    * the task is created, and every time a task is switched out the kernel
    * checks that the last 16 bytes at the far end of its stack still hold
    * 0xA5. If the task ever used that much stack, they no longer do and this
    * hook runs (inside the task switch, so it must not use the RTOS). An
    * overflow that jumps past those 16 bytes can be missed, which is why the
    * debug screen's System page also shows each task's free stack. */
   /* Never return: the offending stack has already been corrupted. Stop the
    * scheduler and every ISR, park the task name for the debugger and flash
    * LD2 (PA5 — shared with SPI1 SCK, harmless once halted) fast forever. */
   __disable_irq();
   g_stack_overflow_task = (const char *)pcTaskName;
   GPIO_InitTypeDef g = {0};
   g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOA, &g);
   for (;;) {
     HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
     for (volatile uint32_t i = 0; i < 400000U; i++) { __NOP(); }   /* ~70 ms at 72 MHz, no SysTick needed */
   }
}
/* USER CODE END 4 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

