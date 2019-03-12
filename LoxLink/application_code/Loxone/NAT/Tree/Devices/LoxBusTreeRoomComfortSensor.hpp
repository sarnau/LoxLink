//
//  LoxBusTreeRoomComfortSensor.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeRoomComfortSensor_hpp
#define LoxBusTreeRoomComfortSensor_hpp

#include "LoxBusTreeDevice.hpp"

class tTreeRoomComfortSensorConfig : public tConfigHeader {
public:
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeRoomComfortSensor : public LoxBusTreeDevice {
  tTreeRoomComfortSensorConfig config;

public:
  LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeRoomComfortSensor_hpp */