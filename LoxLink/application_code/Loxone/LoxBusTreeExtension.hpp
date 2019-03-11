//
//  LoxBusTreeExtension.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeExtension_hpp
#define LoxBusTreeExtension_hpp

#include "LoxNATExtension.hpp"

class tTreeExtensionConfig : public tConfigHeader {
public:
  // no custom configuration
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeExtension : public LoxNATExtension {
public:
  tTreeExtensionConfig config;

  virtual void SendValues(void);
  virtual void ReceiveDirect(LoxCanMessage &message);
  virtual void ReceiveBroadcast(LoxCanMessage &message);
  virtual void ReceiveDirectFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);
  virtual void ReceiveBroadcastFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);

public:
  LoxBusTreeExtension(LoxCANDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeExtension_hpp */