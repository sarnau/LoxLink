//
//  LoxLegacyExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyExtension_hpp
#define LoxLegacyExtension_hpp

#include "LoxExtension.hpp"

class LoxLegacyExtension : public LoxExtension {
protected:
  bool isMuted;
  bool isDeviceIdentified;
  bool forceStartMessage;
  int32_t aliveCountdown;

  // firmware update
  bool firmwareUpdateActive;
  uint32_t firmwareNewVersion;
  uint32_t firmwareUpdateCRCs[64];

  void sendCommandWithValues(LoxMsgLegacyCommand_t command, uint8_t val8, uint16_t val16, uint32_t val32);
  void sendCommandWithVersion(LoxMsgLegacyCommand_t command);

  virtual void PacketMulticastAll(LoxCanMessage &message);
  virtual void PacketMulticastExtension(LoxCanMessage &message);
  virtual void PacketToExtension(LoxCanMessage &message);
  virtual void PacketFromExtension(LoxCanMessage &message);
  virtual void PacketFirmwareUpdate(LoxCanMessage &message);

public:
  LoxLegacyExtension(LoxCANDriver &driver, uint32_t serial, eDeviceType_t device_type, uint8_t hardware_version, uint32_t version);

  virtual void Timer10ms(void);
  virtual void ReceiveMessage(LoxCanMessage &message);
};

#endif /* LoxLegacyExtension_hpp */