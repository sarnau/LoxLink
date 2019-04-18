//
//  LoxBusTreeTouch.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeTouch.hpp"
#include <stdio.h>

/***
 *  Constructor
 ***/
LoxBusTreeTouch::LoxBusTreeTouch(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_TouchTree, 0, 10020130, 2, sizeof(config), &config, alive) {
}

void LoxBusTreeTouch::ConfigUpdate(void) {
  printf("unknown = %d\n", config.unknown);
  printf("audibleFeedbackB = %d\n", config.audibleFeedbackB);
}

void LoxBusTreeTouch::ConfigLoadDefaults(void) {
}