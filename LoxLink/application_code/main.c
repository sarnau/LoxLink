#include "stm32f1xx_hal.h" // HAL_GetUID()
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_ll_utils.h" // LL_GetFlashSize()
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

#include "MMM_can.h"
#include "main.h"
#include "system.h"

EventGroupHandle_t gEventGroup;

static void vMainTsask(void *pvParameters) {
  const TickType_t xDelay1000ms = pdMS_TO_TICKS(1000);
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, 7 | 0x100, pdTRUE, pdFALSE, xDelay1000ms);
    if (uxBits == 0) {
      printf("%d\n", HAL_GetTick());
    }
    if (uxBits & 4) {
      uint8_t keyBitmask = uxBits & 3;
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, !((keyBitmask & 1) == 1)); // 0=LED on, 1=LED off
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, !((keyBitmask & 2) == 2)); // 0=LED on, 1=LED off
      if (keyBitmask == 3) {
        LoxCanMessage msg = {
          .identifier = 0x106ff0fd,
          .data = {0xff, 0x01, 0x00, 0x00, 0x6c, 0x10, 0x10, 0x13},
        };
        MMM_CAN_Send(&msg);
      }
    }
    if (uxBits & 0x100) {
      LoxCanMessage msg;
      while (xQueueReceive(&gCanReceiveQueue, &msg, 0)) {
        printf("%08x %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", msg.identifier, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
      }
    }
  }
}

static void vKeyCheck(void *pvParameters) {
  const TickType_t xDelay50ms = pdMS_TO_TICKS(50);
  uint8_t lastKey = 0;
  while (1) {
    uint8_t key = ~((HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) << 1) | (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) << 0)) & 3; // 0=Button pressed, 1=Button released
    if (key != lastKey) {
      lastKey = key;
      xEventGroupSetBits(gEventGroup, key | 4);
    }
    vTaskDelay(xDelay50ms);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Clock:%dMHz/%dMHz FLASH:%dkb, Unique device ID:%08x.%08x.%08x\n", SystemCoreClock / 1000000, HAL_RCC_GetSysClockFreq() / 1000000, LL_GetFlashSize(), uid[0], uid[1], uid[2]);

  MMM_CAN_Init();
  //MMM_CAN_FilterLoxNAT(0, 0x10, 0xFF, 1, CAN_FILTER_FIFO0);
  MMM_CAN_FilterAllowAll(0);

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

  static StaticEventGroup_t sEventGroup;
  gEventGroup = xEventGroupCreateStatic(&sEventGroup);

  static StackType_t sMainTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sMainTask;
  xTaskCreateStatic(vMainTsask, "MainTask", configMINIMAL_STACK_SIZE, NULL, 1, sMainTaskStack, &sMainTask);

  static StackType_t sKeyCheckStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sKeyCheckTask;
  xTaskCreateStatic(vKeyCheck, "KeyCheck", configMINIMAL_STACK_SIZE, NULL, 1, sKeyCheckStack, &sKeyCheckTask);

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}