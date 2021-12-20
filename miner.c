#include "miner.h"

#define NOTNULL(x) __nonnull (x)

NOTNULL((1, 4)) void mine(const unsigned char blockBinIn[80], unsigned int start, unsigned int end, int *flag, void (*submit) (unsigned char *)) {

  unsigned char hash[32], blockBin[80], hashfinal[32];
  memcpy(blockBin, blockBinIn, 80);
  ((struct block_t *) blockBin)->nonce = start;
  
  do{
    ((struct block_t *) blockBin)->nonce++;
    sha256d(hash, blockBin, 80);
  } while((*((unsigned int *)(hash + 28))) > TARGET && *flag == 0 );


  ((struct block_t *) blockBinIn)->nonce = ((struct block_t *) blockBin)->nonce;
  be2le(hashfinal, hash);
  
  if ( *flag == 0  ) {
    submit(blockBin);

    printf("Found!: ");
    for (unsigned int i = 0; i < 32; ++i)
      printf("%02x", hashfinal[i]);
    printf("\n");
    *flag = 1;
  }

  return;
};