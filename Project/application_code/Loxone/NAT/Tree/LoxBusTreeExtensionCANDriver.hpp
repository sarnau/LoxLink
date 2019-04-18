//
//  LoxBusTreeExtensionCANDriver.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeExtensionCANDriver_hpp
#define LoxBusTreeExtensionCANDriver_hpp

#include "LoxCANBaseDriver.hpp"
#include "LoxNATExtension.hpp"

class LoxBusTreeExtension;

class LoxBusTreeExtensionCANDriver : public LoxCANBaseDriver {
  LoxBusTreeExtension *parentTreeExtension;
  eTreeBranch treeBranch;

public:
  LoxBusTreeExtensionCANDriver(LoxBusTreeExtension *parentTreeExtension, eTreeBranch treeBranch);

  // setup various CAN filters. At least one is required to receive messages!
  virtual void FilterAllowAll(uint32_t filterBank){};
  virtual void FilterSetup(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment){};

  // CAN bus statistics and errors
  uint32_t GetErrorCounter() const;
  uint8_t GetTransmitErrorCounter() const;
  uint8_t GetReceiveErrorCounter() const;

  // send a message onto the CAN bus
  void SendMessage(LoxCanMessage &message);
};

#endif /* LoxBusTreeExtensionCANDriver_hpp */