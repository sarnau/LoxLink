//
//  LoxCANDriver_STM32.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxCANDriver_STM32_hpp
#define LoxCANDriver_STM32_hpp

#include "LoxCANBaseDriver.hpp"

#include "FreeRTOS.h"
#include "LoxCanMessage.hpp"
#include "queue.h"
#include "stm32f1xx_hal_conf.h"

class LoxExtension;

class LoxCANDriver_STM32 : public LoxCANBaseDriver {
  StaticQueue_t transmitQueue;
  static void vCANRXTask(void *pvParameters);
  static void vCANTXTask(void *pvParameters);

public: // internal method
  static void CANSysTick(void);

public:
  LoxCANDriver_STM32(tLoxCANDriverType type);
  void Startup(void);

  // setup various CAN filters. At least one is required to receive messages!
  void FilterAllowAll(uint32_t filterBank);
  void FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment);

  // CAN bus statistics and errors
  uint32_t GetErrorCounter() const;
  uint8_t GetTransmitErrorCounter() const;
  uint8_t GetReceiveErrorCounter() const;

  // send a message onto the CAN bus
  void SendMessage(LoxCanMessage &message);
};

#endif /* LoxCANDriver_STM32_hpp */