//
//  LoxBusTreeExtension.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusTreeExtension_hpp
#define LoxBusTreeExtension_hpp

#include "LoxNATExtension.hpp"

class tTreeExtensionConfig : public tConfigHeader {
public:
  // no custom configuration
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeExtension : public LoxNATExtension {
public:
  tTreeExtensionConfig config;

  void ReceiveDirect(LoxCanMessage &message);

public:
  LoxBusTreeExtension(LoxCANDriver &driver, uint32_t serial, eAliveReason_t alive);
};

#endif /* LoxBusTreeExtension_hpp */