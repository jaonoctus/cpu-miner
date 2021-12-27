#include "miner.h"

#define NOTNULL(x) __nonnull (x)

NOTNULL((1)) void *worker(void *attr) {
  assert(attr != NULL);
  worker_attr_t *attrs = (worker_attr_t *) attr;
  unsigned char hash[32];
  assert(attrs->magic == WORKER_ATTR_MAGIC);

  do{
    ((struct block_t *) attrs->block)->nonce++;
    sha256d(hash, attrs->block, 80);
  } while( (*((unsigned int *)(hash + 28))) > TARGET && *attrs->flag == 0 );
  
  assert(attrs->magic == WORKER_ATTR_MAGIC);

  attrs->ret = *attrs->flag == 0 ? 1 : 0;
  if (*attrs->flag == 0) {
    printf("Found!: ");
    for (unsigned int i = 0; i < 32; ++i)
      printf("%02x", hash[i]);
    printf("\n");
  }
  if (*attrs->flag == 0) 
    *attrs->flag = 1;
}

NOTNULL((1, 4)) void mine(const unsigned char blockBinIn[80], unsigned int start, unsigned int end, int *flag, void (*submit) (unsigned char *)) {
  unsigned char hash[32], blockBin[80], hashfinal[32];

  pthread_t threads[THREADS];
  worker_attr_t threadAttr[THREADS];
  *flag = 0;
  for(unsigned register int i = 0; i < THREADS; ++i) {
    memcpy(threadAttr[i].block, blockBinIn, 80);
    threadAttr[i].magic = WORKER_ATTR_MAGIC;
    //Each block's timestamps are shifted by one, so all workers can try all nonces
    ++((struct block_t *) threadAttr[i].block)->timestamp;
    threadAttr[i].flag = flag;
    pthread_create(&threads[i], NULL, worker, &threadAttr[i]);
  }
  //Relax, let your workers do the job
  do {
    sleep(0.1);
  } while(*flag == 0);
  //Something happend, either us or someone else found a block

  //Did we find?
  if (*flag == 1) {
    //Let's broadcast it!
    for(unsigned register int i = 0; i < THREADS; ++i) {
      if(threadAttr[i].ret == 1) {
        printf("Block found by %d\n", i);
        memcpy((unsigned char *) blockBinIn, threadAttr[i].block, 80);
        submit(threadAttr[i].block);
      }
    }
  }
  for(unsigned register int i = 0; i < THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }

  return;
};