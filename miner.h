#ifndef MINER_H
#define MINER_H
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include <assert.h>
#include <sys/resource.h>

#include "sha2.h"
#include "primitives/block.h"
#include "CPUMiner.h"

#define THREADS 7
#define TARGET 0xFFFF0000
#define WORKER_ATTR_MAGIC 0x10fe9030
#define NOTNULL(x) __nonnull (x)
#define MAX_RETRY 10

typedef struct worker_attr_s {
    unsigned int magic;
    unsigned char block[128];
    unsigned int *flag; //We need stop working
    int ret;    //If I just found a block
    int cpu;    //Witch CPU should we bind to? 
} worker_attr_t;

#ifndef memcpy
    extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
		  size_t __n) __THROW __nonnull ((1, 2));
#endif
    void affine_to_cpu(int id, int cpu);
#ifndef sleep
    extern unsigned int sleep (unsigned int __seconds);
#endif

NOTNULL((1)) 
    extern void destroyBlock(struct block_t *block);

NOTNULL((1))
    void submitBlockHeader(unsigned char block[80], miner_options_t *opt);

__attribute__((__warn_unused_result__)) 
    extern struct block_t createBlock();
    
NOTNULL((1)) 
    extern void submitBlock(unsigned char *block, miner_options_t *opt);

NOTNULL((1, 2)) 
    extern void serialiseBlock(char *ser_block, const unsigned char *ser_block_header, struct block_t block);

extern void sha256d(unsigned char *out, const unsigned char *src, 
      unsigned int size) __THROW __nonnull ((1, 2));

void be2le(unsigned char *out, const unsigned char *in) 
    __THROW __nonnull ((1, 2));

void mine(int *flag, miner_options_t *opt);
#endif //MINER_H
