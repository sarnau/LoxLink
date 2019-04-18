//
//  LoxBusTreeRgbwDimmer.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeRgbwDimmer_hpp
#define LoxBusTreeRgbwDimmer_hpp

#include "LoxBusTreeDevice.hpp"

class __attribute__((__packed__)) tLoxBusTreeRgbwDimmerConfig : public tConfigHeader {
public:
  uint32_t lossOfConnectionState; // RGBW, 101 = retain value, otherwise 0..100%
  uint32_t fadeRate; // in %/s, bit 7: perception correction active
  uint32_t ledType;

private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeRgbwDimmer : public LoxBusTreeDevice {
  tLoxBusTreeRgbwDimmerConfig config;

  virtual void ConfigUpdate(void);
  virtual void ConfigLoadDefaults(void);
  virtual void ReceiveDirect(LoxCanMessage &message);
  virtual void ReceiveDirectFragment(LoxMsgNATCommand_t command, uint8_t extensionNAT, uint8_t deviceNAT, const uint8_t *data, uint16_t size);

public:
  LoxBusTreeRgbwDimmer(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeRgbwDimmer_hpp */