#include "miner.h"

#define NOTNULL(x) __nonnull (x)

NOTNULL((1)) void *worker(void *attr) {
  assert(attr != NULL);
  worker_attr_t *attrs = (worker_attr_t *) attr;
  unsigned int midstate_hash1[8];
  unsigned char hash[32] = {0};
  unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;

  *attrs->flag = 0;
  //Let's take more processing time, we're greedy :)
  const int pid = gettid();
  affine_to_cpu(pid, attrs->cpu);
mine:
  assert(attrs->magic == WORKER_ATTR_MAGIC);
  do{
    ((struct block_t *) attrs->block)->nonce++;
    sha256d(hash, attrs->block, 80);
  } while( (*((unsigned int *)(hash + 28))) > TARGET && *attrs->flag == 0 );
  //Keep asserting it, in case of overflow we can catch it easily
  assert(attrs->magic == WORKER_ATTR_MAGIC);

  if (*attrs->flag == 0) {
    printf("Found!: ");
    for (unsigned int i = 0; i < 32; ++i)
      printf("%02x", hash[i]);
    printf("\n");
    *attrs->flag = 1;
    attrs->ret = 1;
  }

  if(*attrs->flag == 3) {
    attrs->ret = 0;
    return NULL;
  }

  do {
    sleep(0.1);
  } while(*attrs->flag != 0);

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
struct block_t mineCreateBlock() {
  int tryals = 0;
  struct block_t block;
  do {
    block = createBlock();
    if(block.version == 0) {
      puts("Something went wrong! Trying again...");
      tryals += 1;
      if (tryals > MAX_RETRY) { exit(EXIT_FAILURE); }
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
void mineSubmitBlock(unsigned char *blockBin, struct block_t block) {
  submitBlockHeader(blockBin);  //Block header
  unsigned char serBlock[block.bytes * 2 + 100];

  serialiseBlock(serBlock, blockBin, block);
  submitBlock(serBlock);
}
void mine(int *flag) {
  pthread_t threads[THREADS];
  worker_attr_t threadAttr[THREADS];

  for(unsigned register int i = 0; i < THREADS; ++i) {
    //This struct is passed for all threads, this magic number is just
    //a random int that is unlikely to colide, and is verified agaist corruption
    threadAttr[i].magic = WORKER_ATTR_MAGIC;
    threadAttr[i].cpu = i;
    threadAttr[i].flag = flag;
    // Each worker has is own thread, with that we can extract more power from the CPU
    pthread_create(&threads[i], NULL, worker, &threadAttr[i]);
  }

start:
  unsigned char hash[32], blockHeader[80];
  time_t lastDump = time(NULL);

  struct block_t block = mineCreateBlock();;
  mineSerBlockHeader(blockHeader, block);

  for(unsigned register int i = 0; i < THREADS; ++i) {

    memcpy(threadAttr[i].block, blockHeader, 80);

    //Each block's timestamps are shifted by one, so all workers can try all nonces
    ++((struct block_t *) threadAttr[i].block)->timestamp;
    threadAttr[i].flag = flag;
  }
  *flag = 0;
  //Relax, let your workers do the job
  do {
    sleep(0.1);
  } while(*flag == 0);
  //Something happend, either us or someone else found a block
  //Dump how many hashes we've done

  unsigned long int hashes = 0; 
  for (unsigned register int i = 0; i < THREADS; ++i) {
    hashes += (*(struct block_t *) threadAttr[i].block).nonce;
  }
  
  printf("We done %ld hashes, or %d h/s\n", hashes, hashes/(time(NULL) - lastDump));
  lastDump = time(NULL);
  //Did we find?
  if (*flag == 1) {
    //Let's broadcast it!
    for(unsigned register int i = 0; i < THREADS; ++i) {
      if(threadAttr[i].ret == 1) {
        printf("Found by: %d\n", i); //Witch thread found it?
        threadAttr[i].ret = 0;
        mineSubmitBlock(threadAttr[i].block, block);
      }
    }
  }
  else {
    destroyBlock(&block);
  }
  if (*flag == 3) { //Shutdown requested
    for(unsigned register int i = 0; i < THREADS; ++i) {
      pthread_join(threads[i], NULL);
    }
    return ;    
  }

goto start;

  return;
};