#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_ll_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stm32f1xx.h>

void SysTick_Handler(void) {
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}

void printCPUInfo() {
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Clock:%dMHz FLASH:%dkb, Unique device ID:%x.%x.%x\n", SystemCoreClock / 1000000, LL_GetFlashSize(), uid[0], uid[1], uid[2]);
}

void main(void) {
  SystemCoreClockUpdate(); // update SystemCoreClock
  HAL_Init();

  printCPUInfo();

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_Init;
  GPIO_Init.Pin = GPIO_PIN_13 | GPIO_PIN_14;
  GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_Init);

  while (1) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13);
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    HAL_Delay(1000);
  }
}