//
//  LoxBusTreeRoomComfortSensor.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeRoomComfortSensor_hpp
#define LoxBusTreeRoomComfortSensor_hpp

#include "LoxBusTreeDevice.hpp"

class __attribute__((__packed__)) tTreeRoomComfortSensorConfig : public tConfigHeader {
public:
  uint32_t unknownA;
  uint32_t unknownB;
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeRoomComfortSensor : public LoxBusTreeDevice {
  tTreeRoomComfortSensorConfig config;

  virtual void ConfigUpdate(void);
  virtual void ConfigLoadDefaults(void);

public:
  LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeRoomComfortSensor_hpp */