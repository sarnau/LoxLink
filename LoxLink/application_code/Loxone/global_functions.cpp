//
//  global_functions.cpp
//
//  Created by Markus Fritze on 05.03.19.
//  Copyright ÃÂ© 2019 Markus Fritze. All rights reserved.
//

#include "global_functions.hpp"
#include <stdio.h>
#include <string.h>

uint32_t gRandomSeed = 1;

uint16_t random_range(uint16_t minimum, uint16_t maximum) {
  gRandomSeed = 1103515245 * gRandomSeed + 12345;
  uint16_t value = (gRandomSeed >> 16) & 0x7FFF;
  uint16_t range = maximum - minimum + 1;
  return value % range + minimum;
}

// setup a random seed
void random_init(uint32_t seed) {
  gRandomSeed = seed;
}

uint8_t crc8_default(const void *data, int len) {
  uint8_t crc = 0x00;
  for (int i = 0; i < len; i++) {
    crc ^= ((uint8_t *)data)[i];
    for (int j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0)
        crc = (crc << 1) ^ 0x85;
      else
        crc <<= 1;
    }
  }
  return crc;
}

uint8_t crc8_OneWire(const uint8_t *data, int size) {
  uint8_t crc = 0x00;
  for (int i = 0; i < size; i++) {
    uint8_t inbyte = ((uint8_t *)data)[i];
    for (int j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix)
        crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

uint32_t crc32_stm32_word(uint32_t crc, uint32_t data) {
  crc = crc ^ data;
  for (int i = 0; i < 32; i++) {
    if (crc & 0x80000000)
      crc = (crc << 1) ^ 0x04C11DB7; // STM32 CRC-32
    else
      crc = (crc << 1);
  }
  return crc;
}

uint32_t crc32_stm32_aligned(const void *data, int size) {
  uint32_t crc = -1;
  if (size >> 2) {
    for (int i = 0; i < size - (size & 3); i += 4) {
      crc = crc32_stm32_word(crc, *(uint32_t *)((uint8_t *)data + i));
    }
  }
  if (size & 3) { // the remainder is filled with zero bytes
    uint32_t value = 0;
    memcpy(&value, (uint8_t *)data + size - (size & 3), size & 3);
    crc = crc32_stm32_word(crc, value);
  }
  return crc;
}

void debug_print_buffer(const void *data, int size, const char *header) {
  const int LineLength = 16;
  const uint8_t *dp = (const uint8_t *)data;
  for (int loffset = 0; loffset < size; loffset += LineLength) {
    if(header)
        printf("%s ", header);
    printf("%04x : ", loffset);
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset < size) {
        printf("%02x ", dp[offset]);
      } else {
        printf("   ");
      }
    }
    putchar(' ');
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset >= size)
        break;
      uint8_t c = dp[offset];
      if (c < 0x20 || c >= 0x7f)
        c = '.';
      putchar(c);
    }
    printf("\n");
  }
}