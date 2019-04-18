//
//  LoxLegacyRelayExtension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyRelayExtension_hpp
#define LoxLegacyRelayExtension_hpp

#include "LoxLegacyExtension.hpp"

#define RELAY_EXTENSION_OUTPUTS 14

class LoxLegacyRelayExtension : public LoxLegacyExtension {
  uint16_t harewareDigitalOutBitmask; // 14 possible bits
  bool temperatureForceSend;
  bool temperatureOverheatingFlag; // emergency shutdown, if relays/dimmers got too hot
  float temperature;
  uint32_t temperatureMsTimer;

  void update_relays(uint16_t bitmask);
  virtual void PacketToExtension(LoxCanMessage &message);

public:
  LoxLegacyRelayExtension(LoxCANBaseDriver &driver, uint32_t serial);

  virtual void Startup(void);
  virtual void Timer10ms(void);
};

#endif /* LoxLegacyRelayExtension_hpp */