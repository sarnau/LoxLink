//
//  LED.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LED.hpp"
#include "FreeRTOS.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

LED gLED;

/***
 *  CAN TX Task to send pending messages to the CAN bus
 ***/
void LED::vLEDTask(void *pvParameters) {
  LED *_this = (LED *)pvParameters;
  const int period = 1000;
  const int onPeriod = period / 10; // 10% on
  const int offPeriod = period - onPeriod; // 90% off
  const int identifySpeedup = 10;
  while (1) {
    eLED_state state;
    state.state = _this->led_state.state;

    switch (state.color) {
    case eLED_green:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
      break;
    case eLED_orange:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
      break;
    case eLED_red:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
      break;
    default: // off
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    }
    for (int i = 0; i < (state.identify ? onPeriod/identifySpeedup : onPeriod); ++i) {
      vTaskDelay(pdMS_TO_TICKS(1));
      if (state.state != _this->led_state.state)
        break;
    }
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    for (int i = 0; i < (state.identify ? offPeriod/identifySpeedup : offPeriod); ++i) {
      vTaskDelay(pdMS_TO_TICKS(1));
      if (state.state != _this->led_state.state)
        break;
    }
  }
}

void LED::Startup(void) {
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

  static StackType_t sLEDTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t sLEDTask;
  xTaskCreateStatic(LED::vLEDTask, "LEDTask", configMINIMAL_STACK_SIZE, this, 2, sLEDTaskStack, &sLEDTask);
}

void LED::off(void) {
  //  printf("LED blinking green\n");
  this->led_state.color = eLED_off;
}

void LED::blink_green(void) {
  //  printf("LED blinking green\n");
  this->led_state.color = eLED_green;
}

void LED::blink_orange(void) {
  //  printf("LED blinking orange\n");
  this->led_state.color = eLED_orange;
}

void LED::blink_red(void) {
  //  printf("LED blinking red\n");
  this->led_state.color = eLED_red;
}

void LED::identify_on(void) {
  printf("LED identify on\n");
  this->led_state.identify = true;
}

void LED::identify_off(void) {
  printf("LED identify off\n");
  this->led_state.identify = false;
}

void LED::sync(uint8_t syncOffset, uint32_t timeInMs) {
  //  printf("LED sync(%d,%u)\n", syncOffset, timeInMs);
}