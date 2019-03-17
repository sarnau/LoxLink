//
//  LoxExtension.hpp
//
//  Created by Markus Fritze on 04.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxExtension_hpp
#define LoxExtension_hpp

#include "LoxCANBaseDriver.hpp"
#include "LoxCanMessage.hpp"

// The different state, in which the extension can be
typedef enum {
  eDeviceState_offline = 0, // no Miniserver communication
  eDeviceState_parked,      // detected by the Miniserver, but Extension is not part of the current configuration
  eDeviceState_online,      // fully active Extension
} eDeviceState;

/***
 *  Virtual baseclass for legacy and NAT extensions and devices
 ***/
class LoxExtension {
public:
  const uint32_t serial;                        // 24 bit serial number of the device.
  const uint16_t /*eDeviceType_t*/ device_type; // what kind of extension is this device
  const uint8_t hardware_version;               // some extensions have more than one hardware revision, but typically this is 0.
  const uint32_t version;                       // version number of the software in the extension. Automatically updated by the Miniserver.
protected:
  LoxCANBaseDriver &driver;
  eDeviceState state;

  virtual void SetState(eDeviceState state);
  virtual void ReceiveDirect(LoxCanMessage &message){};
  virtual void ReceiveBroadcast(LoxCanMessage &message){};

public:
  LoxExtension(LoxCANBaseDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version);

  // Need to be called by the main
  virtual void Startup(void){};
  virtual void Timer10ms(void){};
  virtual void ReceiveMessage(LoxCanMessage &message){};
};

#endif /* LoxExtension_hpp */