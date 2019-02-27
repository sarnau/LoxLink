#ifndef MMM_CAN_H
#define MMM_CAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MMM_CAN_Init();
void MMM_CAN_ConfigFilter_internal(uint32_t filterBank, uint32_t filterId, uint32_t filterMaskId, uint32_t filterFIFOAssignment);

#ifdef __cplusplus
}
#endif

#endif