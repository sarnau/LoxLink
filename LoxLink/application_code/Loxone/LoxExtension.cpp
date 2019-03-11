//
//  LoxExtension.cpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#include "LoxExtension.hpp"
#include "LED.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/***
 *  Update the extension state
 ***/
void LoxExtension::SetState(eDeviceState state) {
  if (state != this->state) {
    switch (state) {
    case eDeviceState_offline:
      gLED.blink_red();
      break;
    case eDeviceState_parked:
      gLED.blink_orange();
      break;
    case eDeviceState_online:
      gLED.blink_green();
      break;
    }
    this->state = state;
  }
}

LoxExtension::LoxExtension(LoxCANDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version)
  : driver(driver), serial(serial), device_type(device_type), hardware_version(hardware_version), version(version), state(eDeviceState(-1)) // illegal state to force the SetState() to update
{
  assert(serial != 0);
#if DEBUG
  printf("LoxExtension(%07x,%04x,%d,%d)\n", this->serial, this->device_type, this->hardware_version, this->version);
#endif
  SetState(eDeviceState_offline);
  driver.AddExtension(this);
}