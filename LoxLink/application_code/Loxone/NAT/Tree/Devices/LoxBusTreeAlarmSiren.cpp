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
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_AlarmSirenTree, 0, 10000725, 1, sizeof(config), &config, alive), hardwareTamperStatusOk(true), tamperStatusOk(true), tamperStatusTimer(0), alarmSoundMaxDurationTimer(-1) {
}

void LoxBusTreeAlarmSiren::send_tamper_status(void) {
  send_digital_value(0, this->hardwareTamperStatusOk); // 1 = tamper status ok, 0 = tamper status failure
}

void LoxBusTreeAlarmSiren::hardware_strobe_light(bool status) {
  printf("# Strobe light %s\n", status ? "on" : "off");
}

void LoxBusTreeAlarmSiren::hardware_alarm_sound(bool status) {
  this->alarmSoundMaxDurationTimer = status ? config.maxAudibleAlarmDuration * 1000 : -1;
  printf("# Alarm sound %s\n", status ? "on" : "off");
}

void LoxBusTreeAlarmSiren::Timer10ms(void) {
  // send tamper status, if changed or an update every 30s as an alive message
  if (this->hardwareTamperStatusOk != this->tamperStatusOk or this->tamperStatusTimer >= 30 * 1000) {
    this->tamperStatusTimer = 0;
    this->tamperStatusOk = this->hardwareTamperStatusOk;
    send_tamper_status();
  }
  this->tamperStatusTimer += 10;

  // check the alarm sound timeout
  if (this->alarmSoundMaxDurationTimer >= 0) {
    this->alarmSoundMaxDurationTimer -= 10;
    if (this->alarmSoundMaxDurationTimer < 0) {
      hardware_alarm_sound(false);
    }
  }
  LoxBusTreeDevice::Timer10ms();
}

void LoxBusTreeAlarmSiren::ConfigUpdate(void) {
  int offState = config.offlineHardwareState;
  if ((offState & 3) == 1)
    printf("offlineHardwareState strobe on\n");
  else if ((offState & 3) == 2)
    printf("offlineHardwareState strobe off\n");
  offState >>= 2;
  if ((offState & 3) == 1)
    printf("offlineHardwareState alarm on\n");
  else if ((offState & 3) == 2)
    printf("offlineHardwareState alarm off\n");
  printf("maxAudibleAlarmDuration = %ds\n", config.maxAudibleAlarmDuration);
}

void LoxBusTreeAlarmSiren::SendValues(void) {
  send_tamper_status();
}

void LoxBusTreeAlarmSiren::ReceiveDirect(LoxCanMessage &message) {
  switch (message.commandNat) {
  case Digital_Value:
    hardware_strobe_light((message.value32 & 1) == 1);
    hardware_alarm_sound((message.value32 & 1) == 1);
    break;
  default:
    LoxBusTreeDevice::ReceiveDirect(message);
    break;
  }
}

/***
 *  Update the extension state
 ***/
void LoxBusTreeAlarmSiren::SetState(eDeviceState state) {
  LoxNATExtension::SetState(state);
  if (state == eDeviceState_offline) {
    int offState = config.offlineHardwareState;
    switch (offState & 3) {
    case 1:
      hardware_strobe_light(true);
      break;
    case 2:
      hardware_strobe_light(false);
      break;
    default:
      break; // no change
    }
    switch ((offState >> 2) & 3) {
    case 1:
      hardware_alarm_sound(true);
      break;
    case 2:
      hardware_alarm_sound(false);
      break;
    default:
      break; // no change
    }
  }
}