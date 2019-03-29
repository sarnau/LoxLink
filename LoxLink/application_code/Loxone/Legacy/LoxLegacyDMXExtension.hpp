//
//  LoxLegacyDMXExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyDMXExtension_hpp
#define LoxLegacyDMXExtension_hpp

#include "LoxLegacyExtension.hpp"

class LoxLegacyDMXExtension : public LoxLegacyExtension {
  uint8_t fragData[36]; // fragmented package
  virtual void PacketToExtension(LoxCanMessage &message);
  virtual void FragmentedPacketToExtension(LoxMsgLegacyFragmentedCommand_t fragCommand, const void *fragData, int size);
  virtual void StartRequest();

  void search_reply(uint16_t channel, uint16_t manufacturerID, uint32_t deviceID, bool isLast);
  const char *const DMX_DeviceTypeString(uint8_t type) const;

public:
  LoxLegacyDMXExtension(LoxCANBaseDriver &driver, uint32_t serial);
};

#endif /* LoxLegacyDMXExtension_hpp */