//
//  LoxBusTreeAlarmSiren.cpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxBusTreeAlarmSiren.hpp"
#include <stdio.h>

/***
 *  Constructor
 ***/
LoxBusTreeAlarmSiren::LoxBusTreeAlarmSiren(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_AlarmSirenTree, 0, 10000725, 1, sizeof(config), &config, alive) {
}

void LoxBusTreeAlarmSiren::ConfigUpdate(void) {
  if((config.offlineHardwareState & 3) == 1)
    printf("offlineHardwareState strobe on\n");
  else if((config.offlineHardwareState & 3) == 2)
    printf("offlineHardwareState strobe off\n");
  if((config.offlineHardwareState & 12) == 4)
    printf("offlineHardwareState alarm on\n");
  else if((config.offlineHardwareState & 12) == 8)
    printf("offlineHardwareState alarm off\n");
  printf("maxAudibleAlarmDuration = %ds\n", config.maxAudibleAlarmDuration);
}

void LoxBusTreeAlarmSiren::ConfigLoadDefaults(void) {
}