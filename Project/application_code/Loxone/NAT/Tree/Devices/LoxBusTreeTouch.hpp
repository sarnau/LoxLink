//
//  LoxBusTreeTouch.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeTouch_hpp
#define LoxBusTreeTouch_hpp

#include "LoxBusTreeDevice.hpp"

class __attribute__((__packed__)) tLoxBusTreeTouchConfig : public tConfigHeader {
public:
  uint32_t unknown;
  uint8_t audibleFeedbackB;

private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeTouch : public LoxBusTreeDevice {
  tLoxBusTreeTouchConfig config;

  virtual void ConfigUpdate(void);
  virtual void ConfigLoadDefaults(void);

public:
  LoxBusTreeTouch(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeTouch_hpp */