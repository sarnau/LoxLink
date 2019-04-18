//
//  global_functions.hpp
//
//  Created by Markus Fritze on 05.03.19.
//  Copyright (c) 2019 Markus Fritze. All rights reserved.
//

#ifndef crc_hpp
#define crc_hpp

#include <stdint.h>
#include <stdlib.h>

// a 16-bit unsigned integer random number within a given range
uint16_t random_range(uint16_t minimum, uint16_t maximum);

// setup a random seed
void random_init(uint32_t seed);

// These are the 3 commonly used CRC algorithms for the Loxone hardware:

// simple CRC8 with a Polynome of 0x85
uint8_t crc8_default(const void *data, size_t len);

// Used for Maxim 1-Wire hardware to calculate the CRC over the serialnumber
uint8_t crc8_OneWire(const void *data, size_t size);

// Used for Modbus, a CRC16
uint16_t crc16_Modus(const void *data, size_t size);

// STM32 CRC32 algorithm as available in the STM32 hardware. Used as CRC32 over packages, etc
uint32_t crc32_stm32_aligned(const void *data, size_t size);

// Print a hexdump
#if DEBUG
void debug_print_buffer(const void *data, size_t size, const char *header = 0);
#endif

#endif /* crc_hpp */