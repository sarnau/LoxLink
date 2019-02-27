#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_hal.h" // HAL_GetUID()
#include "stm32f1xx_ll_utils.h" // LL_GetFlashSize()
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

void SetSysClockTo72(void);
void MMM_initialize_heap(void);

static void printCPUInfo() {
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Clock:%dMHz/%dMHz FLASH:%dkb, Unique device ID:%x.%x.%x\n", SystemCoreClock / 1000000, HAL_RCC_GetSysClockFreq() / 1000000, LL_GetFlashSize(), uid[0], uid[1], uid[2]);
}

static void vTaskCode(void *pvParameters) {
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
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  const TickType_t xDelay = 10 / portTICK_PERIOD_MS;
  while (1) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15));
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1));
    vTaskDelay(xDelay);
  }
}

int main(void) {
  SetSysClockTo72();
  SystemCoreClockUpdate();
  HAL_Init();
  MMM_initialize_heap();

  printCPUInfo();

  TaskHandle_t xHandle = NULL;
  xTaskCreate(vTaskCode, "LEDBlink", configMINIMAL_STACK_SIZE, NULL, 8, &xHandle);
  configASSERT(xHandle);

  vTaskStartScheduler();
  return 0;
}
