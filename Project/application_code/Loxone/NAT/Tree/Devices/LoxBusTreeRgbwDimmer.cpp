//
//  LoxBusTreeRgbwDimmer.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeRgbwDimmer.hpp"
#include <__cross_studio_io.h>

/***
 *  Constructor
 ***/
LoxBusTreeRgbwDimmer::LoxBusTreeRgbwDimmer(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_RGBW24VDimmerTree, 0, 10031111, 1, sizeof(config), &config, alive) {
}

void LoxBusTreeRgbwDimmer::ConfigUpdate(void) {
  debug_printf("lossOfConnectionState = %08lx\n", config.lossOfConnectionState);
  debug_printf("fadeRate = %08lx\n", config.fadeRate);
  debug_printf("ledType = %08lx\n", config.ledType);
}

void LoxBusTreeRgbwDimmer::ConfigLoadDefaults(void) {
}

void LoxBusTreeRgbwDimmer::ReceiveDirect(LoxCanMessage &message) {
    message.print(this->driver);
    LoxBusTreeDevice::ReceiveDirect(message);
}

void LoxBusTreeRgbwDimmer::ReceiveDirectFragment(LoxMsgNATCommand_t command, uint8_t extensionNAT, uint8_t deviceNAT, const uint8_t *data, uint16_t size) {
    if(command != Config_Data) {
        debug_printf("frag %02x\n", command);
    }
    LoxBusTreeDevice::ReceiveDirectFragment(command, extensionNAT, deviceNAT, data, size);
}
