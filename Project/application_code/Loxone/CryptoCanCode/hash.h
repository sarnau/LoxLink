/**
 * A collection of various hash functions
 */
#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>

// Robert Sedgwick's "Algorithms in C" hash function.
uint32_t RSHash(const uint8_t *buffer, uint32_t size);

// A bitwise hash function written by Justin Sobel. Ignores the seed when 0.
uint32_t JSHash(const uint8_t *buffer, uint32_t size);

// An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published. Substitutes the algorithm's initial value when the seed is non-zero.
uint32_t DJBHash(const uint8_t *buffer, uint32_t size);

// An algorithm proposed by Donald E. Knuth in "The Art Of Computer Programming, Volume 3", under the topic of sorting and search chapter 6.4. Substitutes the algorithm's initial value when the seed is non-zero.
uint32_t DEKHash(const uint8_t *buffer, uint32_t size);

// BPHash is a hashing library for C++. It is designed to hash complex C++ data types, such as containers, pointers, and smart pointers.
uint32_t BPHash(const uint8_t *buffer, uint32_t size);

#endif
