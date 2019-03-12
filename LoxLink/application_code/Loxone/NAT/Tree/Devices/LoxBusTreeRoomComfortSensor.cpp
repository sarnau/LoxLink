//
//  LoxBusTreeRoomComfortSensor.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeRoomComfortSensor.hpp"
#include <stdio.h>

/***
 *  Constructor
 ***/
LoxBusTreeRoomComfortSensor::LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_RoomComfortSensorTree, 0, 10000725, 1, sizeof(config), &config, alive) {
}

void LoxBusTreeRoomComfortSensor::ConfigUpdate(void) {
  printf("unknownA = %d\n", config.unknownA);
  printf("unknownB = %d\n", config.unknownB);
}

void LoxBusTreeRoomComfortSensor::ConfigLoadDefaults(void) {
}