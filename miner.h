#ifndef MINER_H
#define MINER_H
//How many lending 0's, if you want 4 0's, use 0x0000ffff
#define TARGET 0x00000000
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sha2.h"
#include "block.h"

#ifndef memcpy
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
		  size_t __n) __THROW __nonnull ((1, 2));
#endif
extern void sha256d(unsigned char *out, const unsigned char *src, 
      unsigned int size) __THROW __nonnull ((1, 2));

void be2le(unsigned char *out, const unsigned char *in) 
    __THROW __nonnull ((1, 2));

void mine(const unsigned char blockBinIn[80], unsigned int start, unsigned int end, int *flag, void (*submit) (unsigned char *));

#endif //MINER_H