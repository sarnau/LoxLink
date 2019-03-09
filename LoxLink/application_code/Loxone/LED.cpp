//
//  LED.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LED.hpp"
#include "stm32f1xx_hal_gpio.h"
#include <stdio.h>

LED gLED;

void LED::blink_green(void) const {
//  printf("LED blinking green\n");
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

void LED::blink_orange(void) const {
//  printf("LED blinking orange\n");
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
}

void LED::blink_red(void) const {
//  printf("LED blinking red\n");
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

void LED::identify_on(void) const {
  printf("LED identify on\n");
}

void LED::identify_off(void) const {
  printf("LED identify off\n");
}

void LED::sync(uint8_t syncOffset, uint32_t timeInMs) const {
//  printf("LED sync(%d,%u)\n", syncOffset, timeInMs);
}