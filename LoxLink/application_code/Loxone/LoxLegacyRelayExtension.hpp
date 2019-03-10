//
//  LoxLegacyRelayExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyRelayExtension_hpp
#define LoxLegacyRelayExtension_hpp

#include "LoxLegacyExtension.hpp"

class LoxLegacyRelayExtension : public LoxLegacyExtension {
  uint16_t harewareDigitalOutBitmask; // 14 possible bits
  bool forceSendTemperature;
  bool shutdownFlag; // extension is overheating

  void update_relays(uint16_t bitmask);
  virtual void PacketToExtension(LoxCanMessage &message);

public:
  LoxLegacyRelayExtension(LoxCANDriver &driver, uint32_t serial);

  virtual void Timer10ms(void);
};

#endif /* LoxLegacyRelayExtension_hpp */