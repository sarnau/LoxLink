//
//  global_functions.hpp
//
//  Created by Markus Fritze on 05.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#ifndef crc_hpp
#define crc_hpp

#include <stdint.h>

// a 16-bit unsigned integer random number within a given range
uint16_t random_range(uint16_t minimum, uint16_t maximum);

// setup a random seed
void random_init(uint32_t seed);

// These are the 3 commonly used CRC algorithms for the Loxone hardware:

// simple CRC8 with a Polynome of 0x85
uint8_t crc8_default(const void* data, int len);

// Used for Maxim 1-Wire hardware to calculate the CRC over the serialnumber
uint8_t crc8_OneWire(const uint8_t* data, int size);

// STM32 CRC32 algorithm as available in the STM32 hardware. Used as CRC32 over packages, etc
uint32_t crc32_stm32_aligned(const void* data, int size);

#endif /* crc_hpp */
