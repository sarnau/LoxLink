//
//  LoxBusTreeExtension.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeExtension_hpp
#define LoxBusTreeExtension_hpp

#include "LoxBusTreeDevice.hpp"
#include "LoxNATExtension.hpp"

#define MAX_TREE_DEVICECOUNT 62 // max. number per branch (6 bits, but 0 and 63 are reserved)

class tTreeExtensionConfig : public tConfigHeader {
public:
  // no custom configuration
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeExtension : public LoxNATExtension {
public:
  tTreeExtensionConfig config;

  int treeDevicesLeftCount;
  LoxBusTreeDevice *treeDevicesLeft[MAX_TREE_DEVICECOUNT];
  int treeDevicesRightCount;
  LoxBusTreeDevice *treeDevicesRight[MAX_TREE_DEVICECOUNT];

  virtual void SendValues(void);
  virtual void ReceiveDirect(LoxCanMessage &message);
  virtual void ReceiveBroadcast(LoxCanMessage &message);
  virtual void ReceiveDirectFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);
  virtual void ReceiveBroadcastFragment(LoxMsgNATCommand_t command, const uint8_t *data, uint16_t size);

public:
  LoxBusTreeExtension(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);

  // add an extension to the driver
  void AddExtension(LoxBusTreeDevice *ext, eTreeBranch branch);
};

#endif /* LoxBusTreeExtension_hpp */