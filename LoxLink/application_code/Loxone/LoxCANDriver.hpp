//
//  LoxCANDriver.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxCANDriver_hpp
#define LoxCANDriver_hpp

#include "LoxCanMessage.hpp"

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

  tLoxCANDriverType GetDriverType() const;

  void AddExtension(LoxExtension *);

  void SetupCANFilter(int filterIndex, LoxCmdNATBus_t busType, uint8_t extensionNAT);

  // CAN error reporting
  uint8_t GetTransmitErrorCounter() const;
  uint8_t GetReceiveErrorCounter() const;

  void Delay(int msDelay) const;

  void SendMessage(LoxCanMessage &message);
  void ReceiveMessage(LoxCanMessage &message);
};

#endif /* LoxCANDriver_hpp */