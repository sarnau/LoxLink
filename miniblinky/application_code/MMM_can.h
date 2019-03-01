#ifndef MMM_CAN_H
#define MMM_CAN_H

#include "stm32f1xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CAN_HandleTypeDef gCan;

void MMM_CAN_Init();

#ifdef __cplusplus
}
#endif

#endif