//
//  LoxBusTreeRoomComfortSensor.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeRoomComfortSensor.hpp"

/***
 *  Constructor
 ***/
LoxBusTreeRoomComfortSensor::LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_RoomComfortSensorTree, 0, 10000725, 1, sizeof(config), &config, alive) {
}
