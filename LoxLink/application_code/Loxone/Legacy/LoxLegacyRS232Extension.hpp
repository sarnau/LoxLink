//
//  LoxLegacyRS232Extension.hpp
//
//  Created by Markus Fritze on 03.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxLegacyRS232Extension_hpp
#define LoxLegacyRS232Extension_hpp

#include "FreeRTOS.h"
#include "LoxLegacyExtension.hpp"
#include "queue.h"

#define RS232_RX_BUFFERSIZE 512
#define RS232_TX_BUFFERSIZE 512

class LoxLegacyRS232Extension : public LoxLegacyExtension {
  uint8_t sendCount;
  uint8_t sendFill;
  uint8_t sendCRC;
  uint8_t sendData[256]; // max. size of one send buffer as received via several messages from the Miniserver
  StaticQueue_t txQueue;

  static void vRS232RXTask(void *pvParameters);
  static void vRS232TXTask(void *pvParameters);

  virtual void PacketToExtension(LoxCanMessage &message);

public:
  LoxLegacyRS232Extension(LoxCANBaseDriver &driver, uint32_t serial);

  virtual void Startup(void);
};

#endif /* LoxLegacyRS232Extension_hpp */