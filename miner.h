#ifndef MINER_H
#define MINER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "sha2.h"
#include "primitives/block.h"

#define THREADS 7
#define TARGET 0x0000000
#define WORKER_ATTR_MAGIC 0x10fe9030
typedef struct worker_attr_s {
    unsigned int magic;
    unsigned char block[80];
    unsigned int *flag; //We need stop working
    int ret;    //If I just found a block
} worker_attr_t;

#ifndef memcpy
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
		  size_t __n) __THROW __nonnull ((1, 2));
#endif

#ifndef sleep
    extern unsigned int sleep (unsigned int __seconds);
#endif

extern void sha256d(unsigned char *out, const unsigned char *src, 
      unsigned int size) __THROW __nonnull ((1, 2));

void be2le(unsigned char *out, const unsigned char *in) 
    __THROW __nonnull ((1, 2));

void mine(const unsigned char blockBinIn[80], unsigned int start, unsigned int end, int *flag, void (*submit) (unsigned char *));

#endif //MINER_H
