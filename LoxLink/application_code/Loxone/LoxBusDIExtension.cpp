//
//  LoxBusDIExtension.cpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusDIExtension.hpp"
#include "global_functions.hpp"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include <stdio.h>

static const struct {
  uint16_t pin;
  GPIO_TypeDef *gpio;
} gPins[DI_EXTENSION_INPUTS] = {
  {GPIO_PIN_0, GPIOE},
  {GPIO_PIN_1, GPIOE},
  {GPIO_PIN_2, GPIOE},
  {GPIO_PIN_3, GPIOE},
  {GPIO_PIN_4, GPIOE},
  {GPIO_PIN_5, GPIOE},
  {GPIO_PIN_6, GPIOE},
  {GPIO_PIN_7, GPIOE},
  {GPIO_PIN_8, GPIOE},
  {GPIO_PIN_9, GPIOE},
  {GPIO_PIN_10, GPIOE},
  {GPIO_PIN_11, GPIOE},
  {GPIO_PIN_12, GPIOE},
  {GPIO_PIN_13, GPIOE},
  {GPIO_PIN_14, GPIOE},
  {GPIO_PIN_15, GPIOE},
  {GPIO_PIN_0, GPIOC},
  {GPIO_PIN_1, GPIOC},
  {GPIO_PIN_2, GPIOC},
  {GPIO_PIN_3, GPIOC},
};

TIM_HandleTypeDef g1000HzTimer;
LoxBusDIExtension *gDIExt;

extern "C" void TIM3_IRQHandler(void) {
  HAL_TIM_IRQHandler(&g1000HzTimer);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM3) {
    static volatile uint32_t sMillisecondsCounter;
    uint32_t gpioBits = 0;
    for (int i = 0; i < DI_EXTENSION_INPUTS; ++i) {
      GPIO_PinState state = HAL_GPIO_ReadPin(gPins[i].gpio, gPins[i].pin);
      if (gDIExt->config.frequencyInputsBitmask & (1 << i)) { // is this pin a frequency counter?
        if (state != gDIExt->hardwareFrequencyStates[i].state) {
          gDIExt->hardwareFrequencyStates[i].state = state;
          gDIExt->hardwareFrequencyStates[i].impulseCounter++; // count every change of a flank
        }
      } else { // regular (non-frequency) input
        if (state == GPIO_PIN_SET)
          gpioBits |= 1 << i;
      }
      // once a second transfer the frequency counter into the frequency and reset the counter
      if (sMillisecondsCounter == 0) {
        gDIExt->hardwareFrequencyStates[i].frequencyHz = gDIExt->hardwareFrequencyStates[i].impulseCounter;
        gDIExt->hardwareFrequencyStates[i].impulseCounter = 0;
      }
    }
    gDIExt->hardwareBitmask = gpioBits;
    sMillisecondsCounter++;
    if (sMillisecondsCounter >= 1000) // reset every second
      sMillisecondsCounter = 0;
  }
}

LoxBusDIExtension::LoxBusDIExtension(LoxCANDriver &driver, uint32_t serial)
  : LoxNATExtension(driver, (serial & 0xFFFFFF) | (eDeviceType_t_DIExtension << 24), eDeviceType_t_DIExtension, 0, 9021122, 1, sizeof(config), &config), hardwareBitmask(0), lastBitmaskSendTime(0), lastBitmaskSend(0), lastFrequencyTime(0) {
  gDIExt = this;
}

void LoxBusDIExtension::Startup(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Configure all inputs
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  for (int i = 0; i < DI_EXTENSION_INPUTS; ++i) {
    GPIO_InitTypeDef GPIO_Init;
    GPIO_Init.Pin = gPins[i].pin;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(gPins[i].gpio, &GPIO_Init);
  }

  g1000HzTimer.Instance = TIM3;
  g1000HzTimer.Init.Prescaler = HAL_RCC_GetPCLK1Freq() / 2000 - 1;
  g1000HzTimer.Init.Period = 2 - 1; // 2000HZ / 2 = 1000Hz

  __HAL_RCC_TIM3_CLK_ENABLE();
  HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);

  HAL_TIM_Base_Init(&g1000HzTimer);
  HAL_TIM_Base_Start_IT(&g1000HzTimer);
}

/***
 *  Send current status back. This happens on value changes and typically on being assigned a NAT,
 *  to provide the Miniserver with the current value after a reboot.
 ***/
void LoxBusDIExtension::SendValues() {
  // read all gpio bits
  uint32_t gpioBits = 0;
  for (int i = 0; i < DI_EXTENSION_INPUTS; ++i) {
    GPIO_PinState state = HAL_GPIO_ReadPin(gPins[i].gpio, gPins[i].pin);
      if ((this->config.frequencyInputsBitmask & (1 << i)) // is this pin a frequency counter?
        continue;
        // regular (non-frequency) input
        if (state == GPIO_PIN_SET)
          gpioBits |= 1 << i;
  }
  this->hardwareBitmask = gpioBits;
  send_digital_value(0, this->hardwareBitmask);
}

/***
 *  A new configuration has been uploaded. Here the extension need to get reconfigured accordingly.
 ***/
void LoxBusDIExtension::ConfigUpdate(void) {
  //printf("Config updated: 0x%04x\n", this->config.frequencyInputsBitmask);
}

/***
 *  10ms Timer to be called 100x per second
 ***/
void LoxBusDIExtension::Timer10ms(void) {
  LoxNATExtension::Timer10ms();

  if (this->lastFrequencyTime >= 1000) { // frequencies are sent once per second
    this->lastFrequencyTime = 0;
    for (int i = 0; i < DI_EXTENSION_INPUTS; ++i) {
      if (this->config.frequencyInputsBitmask & (1 << i)) { // is this pin a frequency counter?
        uint16_t freq = this->hardwareFrequencyStates[i].frequencyHz;
        if (freq) {
          send_frequency_value(i, freq);
          this->hardwareFrequencyStates[i].zeroHzSent = false;
        } else {
          // this flag avoids sending 0Hz repeatedly, once the frequency drops to nothing
          if (!this->hardwareFrequencyStates[i].zeroHzSent) {
            this->hardwareFrequencyStates[i].zeroHzSent = true;
            send_frequency_value(i, 0);
          }
        }
      }
    }
  }
  this->lastFrequencyTime += 10;

  // simulate the inputs changing every second. They are sent back on every value change,
  // but not faster than 20ms (= 50Hz)
  if ((this->lastBitmaskSendTime % 50) == 0) { // throttle to 50Hz
    if (this->lastBitmaskSend != this->hardwareBitmask) {
      this->lastBitmaskSend = this->hardwareBitmask;
      this->lastBitmaskSendTime = 0;
      send_digital_value(0, this->lastBitmaskSend);
    }
  }
  this->lastBitmaskSendTime += 10;
}