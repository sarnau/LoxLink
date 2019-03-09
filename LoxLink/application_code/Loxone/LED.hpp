//
//  LED.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LED_hpp
#define LED_hpp

#include <stdint.h>

class LED {
public:
  void blink_green(void) const;
  void blink_orange(void) const;
  void blink_red(void) const;
  void identify_on(void) const;
  void identify_off(void) const;
  void sync(uint8_t syncOffset, uint32_t timeInMs) const;
};

extern LED gLED;

#endif /* LED_hpp */