#ifndef RTOS_CODE_H
#define RTOS_CODE_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PRIVILEGED_DATA volatile BaseType_t xFreeRTOSActive;

#ifdef __cplusplus
}
#endif

#endif