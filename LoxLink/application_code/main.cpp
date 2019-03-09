#include "stm32f1xx_hal.h" // HAL_GetUID()
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

#include "main.hpp"
#include "rtos_code.h"
#include "system.hpp"

#include "LoxBusDIExtension.hpp"

EventGroupHandle_t gEventGroup;

static LoxCANDriver gLoxCANDriver(tLoxCANDriverType_LoxoneLink);
static LoxBusDIExtension gDIExtension(gLoxCANDriver, 0x123456);

static void vMainTask(void *pvParameters) {
  while (1) {
    EventBits_t uxBits = xEventGroupWaitBits(gEventGroup, (eMainEvents_buttonLeft | eMainEvents_buttonRight | eMainEvents_anyButtonPressed) | eMainEvents_LoxCanMessageReceived | eMainEvents_10msTimer, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBits & eMainEvents_anyButtonPressed) {
      uint8_t keyBitmask = uxBits & (eMainEvents_buttonLeft | eMainEvents_buttonRight);
    }
    if (uxBits & eMainEvents_LoxCanMessageReceived) {
      LoxCanMessage msg;
      while (xQueueReceive(&gCanReceiveQueue, &msg, 0)) {
        printf("CANR:"); msg.print(gLoxCANDriver);
        gDIExtension.ReceiveMessage(msg);
      }
    }
    if (uxBits & eMainEvents_10msTimer) {
      //printf("Ticks %d\n", HAL_GetTick());
      gDIExtension.Timer10ms();
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

  //  MX_print_cpu_info();

  gLoxCANDriver.Startup();

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

  gDIExtension.Startup();

  static StackType_t sMainTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sMainTask;
  xTaskCreateStatic(vMainTask, "MainTask", configMINIMAL_STACK_SIZE, NULL, 2, sMainTaskStack, &sMainTask);

  static StackType_t sKeyInputTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sKeyInputTaskTask;
  xTaskCreateStatic(vKeyInputTask, "KeyInputTask", configMINIMAL_STACK_SIZE, NULL, 1, sKeyInputTaskStack, &sKeyInputTaskTask);
  xFreeRTOSActive = pdTRUE; // if the boot takes too long, the SysTick ISR will trigger a crash in FreeRTOS, if it is not already initialized.
  vTaskStartScheduler();
  NVIC_SystemReset(); // should never reach this point
  return 0;
}