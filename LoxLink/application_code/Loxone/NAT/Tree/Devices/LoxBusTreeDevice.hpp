//
//  LoxBusTreeDevice.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeDevice_hpp
#define LoxBusTreeDevice_hpp

#include "LoxNATExtension.hpp"
#include <stdio.h>

class LoxBusTreeDevice : public LoxNATExtension {
public:
  LoxBusTreeDevice(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version, uint8_t configVersion, uint8_t configSize, tConfigHeader *configPtr, eAliveReason_t alive);
};

#endif /* LoxBusTreeDevice_hpp */