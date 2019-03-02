#include "stm32f1xx_hal.h" // HAL_GetUID()
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_ll_utils.h" // LL_GetFlashSize()
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "MMM_can.h"


void SystemClock_Config(void);

static StaticQueue_t gKeyQueue;

static void vLEDBlink(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xDelay500ms = pdMS_TO_TICKS(500);
  while (1) {
#if 0
    printf("A");
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13);
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
#else
    uint8_t key;
    if (xQueueReceive(&gKeyQueue, &key, xDelay500ms)) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (key & 1) == 1);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, (key & 2) == 2);
      if (key == 3) {
        CAN_TxHeaderTypeDef hdr = {
          .ExtId = 0x106ff0fd,
          .IDE = CAN_ID_EXT,
          .RTR = CAN_RTR_DATA,
          .DLC = 8,
          .TransmitGlobalTime = DISABLE,
        };
        uint8_t data[8] = {0xff, 0x01, 0x00, 0x00, 0x6c, 0x10, 0x10, 0x13};
        uint32_t txMailbox = 0;
        HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&gCan, &hdr, data, &txMailbox);
        //printf("status = %d\n", status);
      }
    }
#endif
  }
}

static void vKeyCheck(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xDelay10ms = pdMS_TO_TICKS(10);
  uint8_t lastKey = 0;
  while (1) {
#if 0
    printf("B");
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
#else
    uint8_t key = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) << 1) | (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) << 0);
    if (key != lastKey) {
      lastKey = key;
      xQueueSend(&gKeyQueue, &key, 0);
    }
    vTaskDelay(xDelay10ms);
#endif
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Clock:%dMHz/%dMHz FLASH:%dkb, Unique device ID:%08x.%08x.%08x\n", SystemCoreClock / 1000000, HAL_RCC_GetSysClockFreq() / 1000000, LL_GetFlashSize(), uid[0], uid[1], uid[2]);

  MMM_CAN_Init();

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_Init;
  // 2 LEDs as outputs
  GPIO_Init.Pin = GPIO_PIN_13 | GPIO_PIN_14;
  GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  // 2 buttons as inputs
  GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_15;
  GPIO_Init.Mode = GPIO_MODE_INPUT;
  GPIO_Init.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  // queue for 8 key-presses
  static uint8_t sKeyQueueBuffer[8];
  static StaticQueue_t sKeyQueue;
  xQueueCreateStatic(sizeof(sKeyQueueBuffer) / sizeof(sKeyQueueBuffer[0]), sizeof(sKeyQueueBuffer[0]), sKeyQueueBuffer, &gKeyQueue);

  static StackType_t sLEDBlinkStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sLEDBlinkTask;
  xTaskCreateStatic(vLEDBlink, "LEDBlink", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, sLEDBlinkStack, &sLEDBlinkTask);

  static StackType_t sKeyCheckStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sKeyCheckTask;
  xTaskCreateStatic(vKeyCheck, "KeyCheck", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, sKeyCheckStack, &sKeyCheckTask);

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}