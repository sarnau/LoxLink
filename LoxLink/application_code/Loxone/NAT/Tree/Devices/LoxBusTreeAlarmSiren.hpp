//
//  LoxBusTreeAlarmSiren.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeAlarmSiren_hpp
#define LoxBusTreeAlarmSiren_hpp

#include "LoxBusTreeDevice.hpp"

class __attribute__((__packed__)) tTreeAlarmSirenConfig : public tConfigHeader {
public:
  uint8_t offlineHardwareState;
  uint16_t maxAudibleAlarmDuration; // 0=no limit, 1-1800s
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeAlarmSiren : public LoxBusTreeDevice {
  tTreeAlarmSirenConfig config;

  virtual void ConfigUpdate(void);
  virtual void ConfigLoadDefaults(void);

public:
  LoxBusTreeAlarmSiren(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeAlarmSiren_hpp */