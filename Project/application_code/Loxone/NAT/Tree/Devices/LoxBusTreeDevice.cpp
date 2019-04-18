//
//  LoxBusTreeDevice.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeDevice.hpp"

/***
 *  Constructor
 ***/
LoxBusTreeDevice::LoxBusTreeDevice(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version, uint8_t configVersion, uint8_t configSize, tConfigHeader *configPtr, eAliveReason_t alive)
  : LoxNATExtension(driver, serial, device_type, hardware_version, version, configVersion, configSize, configPtr, alive) {
  this->busType = LoxCmdNATBus_t_TreeBus;
}