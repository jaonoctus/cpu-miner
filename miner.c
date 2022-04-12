#include "miner.h"

#define NOTNULL(x) __nonnull (x)

NOTNULL((1)) void *worker(void *attr) {
  assert(attr != NULL);
  unsigned char hash[32];
  worker_attr_t *attrs = (worker_attr_t *) attr;

  unsigned int hs[8], precomp[8];

  //Let's take more processing time, we're greedy :)
  const int pid = gettid();
  affine_to_cpu(pid, attrs->cpu);
  nice(1);
  
mine:
  do {
    sleep(0.1);
  } while(*attrs->flag != 0);

  assert(attrs->magic == WORKER_ATTR_MAGIC);
  attrs->block[80] = 0x80;
  attrs->block[126] = 0x02;
  attrs->block[127] = 0x80;
  sha_precompute(precomp, attrs->block);

  do {
    ((struct block_t *) attrs->block)->nonce++;

    sha_compress_block_header(hs, precomp, attrs->block + 64);
    sha_seccond_hash(hs);
  } while( __glibc_likely (__glibc_likely (hs[7] & 0xFFFFFFFF) && *attrs->flag == 0));

  //Keep asserting it, in case of overflow we can catch it easily
  assert(attrs->magic == WORKER_ATTR_MAGIC);

  if (*attrs->flag == 0) {
    printf("Found!: ");
    printf("%08x", __builtin_bswap32 (hs[7]));
    printf("%08x", __builtin_bswap32 (hs[6]));
    printf("%08x", __builtin_bswap32 (hs[5]));
    printf("%08x", __builtin_bswap32 (hs[4]));
    printf("%08x", __builtin_bswap32 (hs[3]));
    printf("%08x", __builtin_bswap32 (hs[2]));
    printf("%08x", __builtin_bswap32 (hs[1]));
    printf("%08x", __builtin_bswap32 (hs[0]));
    printf("\n");
    *attrs->flag = 1;
    attrs->ret = 1;
  }

  if(*attrs->flag == 3) {
    attrs->ret = 0;
    return NULL;
  }
  
goto mine;
}
void affine_to_cpu(int id, int cpu)
{
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	sched_setaffinity(0, sizeof(&set), &set);
  printf("Biding thread %d to cpu %d\n", id, cpu);
}
struct block_t mineCreateBlock(miner_options_t *opt) {
  int tryals = 0;
  struct block_t block;
  do {
    block = createBlock(opt);
    if(block.version == 0) {
      puts("Something went wrong! Trying again...");
      tryals += 1;
      if (tryals > MAX_RETRY) { return block; }
    }
  } while (block.version == 0);
  return block;
}

void mineSerBlockHeader(unsigned char *ser_block_header, struct block_t block) {
  //Serialize the block header as raw data. We only hex-encode for submiting
  printf("Creating a block with %d txs\n", block.tx_count);
  memcpy(ser_block_header, &block.version, sizeof(int));
  memcpy(ser_block_header + 4, block.prevBlockHash, 32);
  memcpy(ser_block_header + 36, block.merkleRoot, 32);
  memcpy(ser_block_header + 68, &block.timestamp, sizeof(int));
  memcpy(ser_block_header + 72, &block.bits, sizeof(int));
  memcpy(ser_block_header + 76, &block.nonce, sizeof(int));
}
void mineSubmitBlock(unsigned char *blockBin, struct block_t block, miner_options_t *opt) {
  submitBlockHeader(blockBin, opt);  //Block header
  unsigned char serBlock[block.bytes * 2 + 100];

  serialiseBlock(serBlock, blockBin, block);
  submitBlock(serBlock, opt);
}
void mine(int *flag, miner_options_t *opt) {
  pthread_t threads[opt->threads];
  worker_attr_t threadAttr[opt->threads];
  *flag = 4;  //4 means starting
  for(unsigned register int i = 0; i < opt->threads; ++i) {
    //This struct is passed for all threads, this magic number is just
    //a random int that is unlikely to colide, and is verified agaist corruption
    threadAttr[i].magic = WORKER_ATTR_MAGIC;
    threadAttr[i].cpu = i;
    threadAttr[i].flag = flag;
    // Each worker has is own thread, with that we can extract more power from the CPU
    pthread_create(&threads[i], NULL, worker, &threadAttr[i]);
  }

start:
  unsigned char blockHeader[80];
  time_t lastDump = time(NULL);

  struct block_t block = mineCreateBlock(opt);  //This function tryies to create a block 10 times
  if(block.version == 0) {
    *flag = 3;  //10 errors in a row? Not good! Request a shutdown
  }
  //Obtain a block header to mine
  mineSerBlockHeader(blockHeader, block);

  for(unsigned register int i = 0; i < opt->threads; ++i) {
    memcpy(threadAttr[i].block, blockHeader, 80);
    memset(threadAttr[i].block + 80, 0x00, 128 - 80);
    //Each block's timestamps are shifted by one, so all workers can try all nonces
    ++((struct block_t *) threadAttr[i].block)->timestamp;
  }

  *flag = 0;
  //Relax, take a cup of tea, and let your workers do the job
  do {
    sleep(0.1);
  } while(*flag == 0);

  //Something happend, either us or someone else found a block
  //Dump how many hashes we've done
  unsigned long int hashes = 0; 
  for (unsigned register int i = 0; i < opt->threads; ++i) {
    hashes += (*(struct block_t *) threadAttr[i].block).nonce;
  }

  //Catch some floating point exception
  if(time(NULL) != lastDump)
    printf("We made %ld hashes, or %d h/s\n", hashes, hashes/(time(NULL) - lastDump));
  lastDump = time(NULL);
  //Did we find?
  if (*flag == 1) {
    //Let's broadcast it!
    for(unsigned register int i = 0; i < opt->threads; ++i) {
      if(threadAttr[i].ret == 1) {
        printf("Found by: %d\n", i); //Log: Witch thread found it?
        threadAttr[i].ret = 0;
        mineSubmitBlock(threadAttr[i].block, block, opt);
     }
    }
  }
  else {
    destroyBlock(&block);
  }
  
  if (*flag == 3) { //Shutdown requested
    for(unsigned register int i = 0; i < opt->threads; ++i) {
      pthread_join(threads[i], NULL);
    }
    return ;    
  }

goto start;

  return;
};