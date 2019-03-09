//
//  LoxCANDriver.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxCANDriver_hpp
#define LoxCANDriver_hpp

#include "FreeRTOS.h"
#include "LoxCanMessage.hpp"
#include "queue.h"
#include "stm32f1xx_hal_conf.h"

extern StaticQueue_t gCanReceiveQueue;

class LoxExtension;

typedef enum {
  tLoxCANDriverType_LoxoneLink,
  tLoxCANDriverType_TreeBus,
} tLoxCANDriverType;

class LoxCANDriver {
  tLoxCANDriverType driverType;
  int extensionCount;
  LoxExtension *extensions[16]; // up to 16 extensions per driver

public:
  LoxCANDriver(tLoxCANDriverType type);

  void Startup(void);

  tLoxCANDriverType GetDriverType() const;

  void AddExtension(LoxExtension *);

  // setup various CAN filters. At least one is required to receive messages!
  void FilterAllowAll(uint32_t filterBank);
  void FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment);
  void FilterSetupNAT(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT);

  // CAN error reporting
  uint8_t GetTransmitErrorCounter() const;
  uint8_t GetReceiveErrorCounter() const;

  void Delay(int msDelay) const;

  void SendMessage(LoxCanMessage &message);
  void ReceiveMessage(LoxCanMessage &message);
};

#endif /* LoxCANDriver_hpp */