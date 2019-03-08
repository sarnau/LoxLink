#include "stm32f1xx_hal.h" // HAL_GetUID()
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stm32f1xx.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

#include "MMM_can.hpp"
#include "main.hpp"
#include "rtos_code.h"
#include "system.hpp"

#include "LoxBusDIExtension.hpp"

EventGroupHandle_t gEventGroup;

static     LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
static     LoxBusDIExtension gDIExtension(gLoxCANDriver, 0x123456);


static void vMainTask(void *pvParameters) {
  xFreeRTOSActive = pdTRUE; // if the boot takes too long, the SysTick ISR will trigger a crash in FreeRTOS, if it is not already initialized.
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, (eMainEvents_buttonLeft | eMainEvents_buttonRight | eMainEvents_anyButtonPressed) | eMainEvents_LoxCanMessageReceived | eMainEvents_10msTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_anyButtonPressed) {
      uint8_t keyBitmask = uxBits & (eMainEvents_buttonLeft | eMainEvents_buttonRight);
      if (keyBitmask == 3) {
        const uint8_t can_package[8] = {0xff, 0x01, 0x00, 0x00, 0x6c, 0x10, 0x10, 0x13};
        LoxCanMessage msg;
        msg.identifier = 0x106ff0fd;
        memcpy(&msg.can_data, can_package, sizeof(msg.can_data));
        MMM_CAN_Send(msg);
      }
    }
    if (uxBits & eMainEvents_LoxCanMessageReceived) {
      LoxCanMessage msg;
      while (xQueueReceive(&gCanReceiveQueue, &msg, 0)) {
        gDIExtension.ReceiveMessage(msg);
        printf("%08x %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n", msg.identifier, msg.can_data[0], msg.can_data[1], msg.can_data[2], msg.can_data[3], msg.can_data[4], msg.can_data[5], msg.can_data[6], msg.can_data[7]);
      }
    }
    if (uxBits & eMainEvents_10msTimer) {
      //printf("Ticks %d\n", HAL_GetTick());
      gDIExtension.Timer10ms();
    }
  }
}

static void vLEDTask(void *pvParameters) {
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, (eMainEvents_buttonLeft | eMainEvents_buttonRight | eMainEvents_anyButtonPressed) | eMainEvents_1sTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_anyButtonPressed) {
      uint8_t keyBitmask = uxBits & (eMainEvents_buttonLeft | eMainEvents_buttonRight);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, !((keyBitmask & 1) == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET); // 0=LED on, 1=LED off
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, !((keyBitmask & 2) == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET); // 0=LED on, 1=LED off
    }
    if (uxBits & eMainEvents_1sTimer) {
      //printf("Ticks %d (LED)\n", HAL_GetTick());
    }
  }
}

static void vKeyInputTask(void *pvParameters) {
  const TickType_t xDelay50ms = pdMS_TO_TICKS(50);
  uint32_t lastMainEventMask = eMainEvents_none;
  while (1) {
    uint32_t eventMask = eMainEvents_anyButtonPressed;
    if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)) // 0=Button pressed, 1=Button released
      eventMask |= eMainEvents_buttonLeft;
    if (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))
      eventMask |= eMainEvents_buttonRight;
    if (eventMask != lastMainEventMask) {
      lastMainEventMask = eventMask;
      xEventGroupSetBits(gEventGroup, eventMask);
    }
    vTaskDelay(xDelay50ms);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();

  MX_print_cpu_info();

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
  xTaskCreateStatic(vMainTask, "MainTask", configMINIMAL_STACK_SIZE, NULL, 2, sMainTaskStack, &sMainTask);

  static StackType_t sLEDTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sLEDTask;
  xTaskCreateStatic(vLEDTask, "LEDTask", configMINIMAL_STACK_SIZE, NULL, 1, sLEDTaskStack, &sLEDTask);

  static StackType_t sKeyInputTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sKeyInputTaskTask;
  xTaskCreateStatic(vKeyInputTask, "KeyInputTask", configMINIMAL_STACK_SIZE, NULL, 1, sKeyInputTaskStack, &sKeyInputTaskTask);

  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}