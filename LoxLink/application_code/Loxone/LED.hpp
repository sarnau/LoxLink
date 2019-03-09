//
//  LED.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef LED_hpp
#define LED_hpp

#include <stdint.h>

typedef enum {
  eLED_off,
  eLED_green,
  eLED_orange,
  eLED_red,
} eLED_color;

typedef union {
  uint8_t state;
  struct {
    eLED_color color : 7;
    bool identify : 1;
  };
} eLED_state;

class LED {
  volatile eLED_state led_state;
  static void vLEDTask(void *pvParameters);

public:
  void Startup(void);

  void off(void);
  void blink_green(void);
  void blink_orange(void);
  void blink_red(void);
  void identify_on(void);
  void identify_off(void);
  void sync(uint8_t syncOffset, uint32_t timeInMs);
};

extern LED gLED;

#endif /* LED_hpp */