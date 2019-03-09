//
//  LoxBusDIExtension.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusDIExtension_hpp
#define LoxBusDIExtension_hpp

#include "LoxNATExtension.hpp"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_gpio.h"

#define DI_EXTENSION_INPUTS 20

class tDIExtensionConfig : public tConfigHeader {
public:
  uint32_t frequencyInputsBitmask; // bit is set, if an input is used as a frequency counter
private:
  tConfigHeaderFiller filler;
};

class LoxBusDIExtension : public LoxNATExtension {
public:
  // used by the TIM3 IRQ
  volatile uint32_t hardwareBitmask;
  volatile struct {
    GPIO_PinState state;
    bool zeroHzSent;
    uint16_t impulseCounter;
    uint16_t frequencyHz;
  } hardwareFrequencyStates[DI_EXTENSION_INPUTS];
  tDIExtensionConfig config;

private:
  uint32_t lastBitmaskSendTime;
  uint32_t lastBitmaskSend;
  uint32_t lastFrequencyTime;

  virtual void ConfigUpdate(void);
  virtual void SendValues();

public:
  LoxBusDIExtension(LoxCANDriver &driver, uint32_t serial);

  virtual void Startup(void);
  virtual void Timer10ms(void);
};

#endif /* LoxBusDIExtension_hpp */