//
//  LED.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LED.hpp"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include <ctl_api.h>
#include <stdio.h>
#include <string.h>

LED gLED;

/***
 *   Set the LED gpio
 ***/
static void LED_on_off(eLED_color color) {
  switch (color) {
  case eLED_green: // right LED on
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    break;
  case eLED_orange: // left LED on
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    break;
  case eLED_red: // both LEDs on
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    break;
  default: // both LEDs off
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
  }
}

/***
 *  CAN TX Task to send pending messages to the CAN bus
 ***/
void LED::vLEDTask(void *pvParameters) {
  LED *_this = (LED *)pvParameters;
  const int base_period = 1000;
  const int identifySpeedup = 10;
  while (1) {
  restart:
    _this->resync_flag = false;
    eLED_state state;
    state.state = _this->led_state.state;

    if (state.identify) { // no delay during identify
      LED_on_off(state.color);
      int period = ((base_period * 12) / 100) / identifySpeedup; // 12% on (measured via looking at video material)
      for (int i = 0; i < period/10; ++i) {
        ctl_timeout_wait(ctl_get_current_time() + 10);
        if (state.state != _this->led_state.state || _this->resync_flag)
          goto restart;
      }
      LED_on_off(eLED_off);
      period = (base_period - period) / identifySpeedup;
      for (int i = 0; i < period/10; ++i) {
        ctl_timeout_wait(ctl_get_current_time() + 10);
        if (state.state != _this->led_state.state || _this->resync_flag)
          goto restart;
      }
    } else {
      int ldelay = 0;
      // 15ms delay per unit in the rack, in which 5 is 15ms
      ldelay = _this->sync_offset * 3;
      for (int i = 0; i < ldelay/10; ++i) {
        ctl_timeout_wait(ctl_get_current_time() + 10);
        if (state.state != _this->led_state.state || _this->resync_flag)
          goto restart; // force resync when the LED change or a sync is received
      }
      LED_on_off(state.color);
      int period = (base_period * 12) / 100; // 12% on (measured via looking at video material)
      for (int i = 0; i < period/10; ++i) {
        ctl_timeout_wait(ctl_get_current_time() + 10);
        if (state.state != _this->led_state.state || _this->resync_flag)
          goto restart;
      }
      LED_on_off(eLED_off);
      for (int i = 0; i < (base_period - period - ldelay)/10; ++i) {
        ctl_timeout_wait(ctl_get_current_time() + 10);
        if (state.state != _this->led_state.state || _this->resync_flag)
          goto restart;
      }
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

  #define STACKSIZE 128          
  static unsigned stack[1+STACKSIZE+1];
  static CTL_TASK_t led_task;
  ctl_task_run(&led_task, 2, LED::vLEDTask, this, "LED", STACKSIZE, stack+1, 0);
}

void LED::off(void) {
  //  debug_printf("LED blinking green\n");
  this->led_state.color = eLED_off;
}

void LED::blink_green(void) {
  //  debug_printf("LED blinking green\n");
  this->led_state.color = eLED_green;
}

void LED::blink_orange(void) {
  //  debug_printf("LED blinking orange\n");
  this->led_state.color = eLED_orange;
}

void LED::blink_red(void) {
  //  debug_printf("LED blinking red\n");
  this->led_state.color = eLED_red;
}

void LED::identify_on(void) {
  //  debug_printf("LED identify on\n");
  this->led_state.identify = true;
}

void LED::identify_off(void) {
  //  debug_printf("LED identify off\n");
  this->led_state.identify = false;
}

void LED::sync(uint32_t timeInMs) {
  //  debug_printf("LED sync(%u)\n", timeInMs);
  this->resync_flag = true;
}

void LED::set_sync_offset(uint8_t sync_offset) {
  //  debug_printf("LED sync_offset(%u)\n", sync_offset);
  this->sync_offset = sync_offset;
}