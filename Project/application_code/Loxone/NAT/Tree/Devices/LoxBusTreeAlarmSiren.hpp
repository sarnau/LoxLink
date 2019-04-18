//
//  LoxBusTreeAlarmSiren.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeAlarmSiren_hpp
#define LoxBusTreeAlarmSiren_hpp

#include "LoxBusTreeDevice.hpp"

class __attribute__((__packed__)) tTreeAlarmSirenConfig : public tConfigHeader {
public:
  uint8_t offlineHardwareState;
  uint16_t maxAudibleAlarmDuration; // 0=no limit, 1-1800s
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeAlarmSiren : public LoxBusTreeDevice {
  tTreeAlarmSirenConfig config;

  bool hardwareTamperStatusOk;
  bool tamperStatusOk;
  int32_t tamperStatusTimer;
  int32_t alarmSoundMaxDurationTimer;

  void send_tamper_status(void);
  void hardware_strobe_light(bool status);
  void hardware_alarm_sound(bool status);

  virtual void ConfigUpdate(void);
  virtual void SendValues(void);
  virtual void Timer10ms(void);
  virtual void ReceiveDirect(LoxCanMessage &message);
  virtual void SetState(eDeviceState state);

public:
  LoxBusTreeAlarmSiren(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeAlarmSiren_hpp */