#ifndef MAIN_H
#define MAIN_H

#include "FreeRTOS.h"
#include "event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t gEventGroup;

typedef enum {
  eMainEvents_none = 0,
  eMainEvents_buttonLeft = 0x0001,
  eMainEvents_buttonRight = 0x0002,
  eMainEvents_anyButtonPressed = 0x0080,
  eMainEvents_LoxCanMessageReceived = 0x0100,
  eMainEvents_1sTimer = 0x2000,
  eMainEvents_10msTimer = 0x4000,
} eMainEvents;

#ifdef __cplusplus
}
#endif

#endif