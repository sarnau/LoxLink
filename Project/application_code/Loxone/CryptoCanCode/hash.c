#include "hash.h"

// Robert Sedgwick's "Algorithms in C" hash function.
uint32_t RSHash(const uint8_t *buffer, uint32_t size)
{
  uint32_t result = 0;
  uint32_t multFactor = 63689;
  for(uint32_t i=0; i<size; ++i) {
    result = buffer[i] + multFactor * result;
    multFactor *= 378551;
  }
  return result;
}

// A bitwise hash function written by Justin Sobel. Ignores the seed when 0.
uint32_t JSHash(const uint8_t *buffer, uint32_t size)
{
  uint32_t result = 1315423911;
  for(uint32_t i=0; i<size; ++i)
    result ^= (result >> 2) + (result << 5) + buffer[i];
  return result;
}

// An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published. Substitutes the algorithm's initial value when the seed is non-zero.
uint32_t DJBHash(const uint8_t *buffer, uint32_t size)
{
  uint32_t result = 5381;
  for(uint32_t i=0; i<size; ++i)
    result += buffer[i] + (result << 5);
  return result;
}

// An algorithm proposed by Donald E. Knuth in "The Art Of Computer Programming, Volume 3", under the topic of sorting and search chapter 6.4. Substitutes the algorithm's initial value when the seed is non-zero.
uint32_t DEKHash(const uint8_t *buffer, uint32_t size)
{
  uint32_t result = size;
  for(uint32_t i=0; i<size; ++i)
    result = buffer[i] ^ ((result >> 27) | (result << (32 - 27)));
  return result;
}

// BPHash is a hashing library for C++. It is designed to hash complex C++ data types, such as containers, pointers, and smart pointers.
uint32_t BPHash(const uint8_t *buffer, uint32_t size)
{
  uint32_t result = 0;
  for(uint32_t i=0; i<size; ++i)
    result = buffer[i] ^ (result << 7);
  return result;
}
