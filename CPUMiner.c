#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <curl/curl.h>

#include "primitives/block.h"
#include "miner.h"
#include "sha2.h"
#include "primitives/transaction.h"
#include "blockproducer.h"
#include "rpc.h"

struct curl_slist *headers;

//Bitcoind's auth cookie
char cookie[76] = {0}; 

typedef struct thread_opt_s {
  int *flag;
  int height;
} thread_opt_t;

static void getCookie(int testNet) {
  FILE *fcookie;
  if(testNet)
    fcookie = fopen("/home/erik/.bitcoin/testnet3/.cookie", "r");
  else
    fcookie = fopen("/home/erik/.bitcoin/.cookie", "r");
  if(fcookie == NULL) {
    puts("Error: Reading cookie file");
    exit(1);
  }
  fread(cookie, sizeof(char), 75, fcookie);
}

NOTNULL((1)) void *handleTipUpdate(void *attr) {
  struct memory readBuffer;
  thread_opt_t *attrs = (thread_opt_t *) attr;
  time_t nextUpdate = time(NULL) + 10 * 60;

  callRPC(&readBuffer, getBlockchaininfo, headers, cookie);
  readBuffer.response[35 + 7] = '\0';

  if(readBuffer.response == NULL) return (void *) EXIT_FAILURE;
  
  sscanf(readBuffer.response + 35, "%d", &attrs->height);
  printf("Tip: %d\n", attrs->height);
  free(readBuffer.response);

  int newHeight;

  do {
    callRPC(&readBuffer, getBlockchaininfo, headers, cookie);
    if(readBuffer.size > 50) {
      readBuffer.response[35 + 7] = '\0';

      sscanf(readBuffer.response + 35, "%d", &newHeight);

      if (attrs->height != newHeight) {
        *(attrs->flag) = 2;
        attrs->height = newHeight;
        nextUpdate = time(NULL) + 10 * 60;
        printf("Tip updated %d\n", attrs->height);
      }
    }
    
    if(nextUpdate <= time(NULL)) {
      *(attrs->flag) = 2;
      nextUpdate += 10 * 60;
    }
    if(readBuffer.response != NULL)
      free(readBuffer.response);

    sleep(10);

  } while(*(attrs->flag) == 0);
}

int main() {

  headers = curl_slist_append(headers, "Content-Type: application/json");

  struct block_t block;

  int flag = 0;
  thread_opt_t thopt;
  thopt.height = 14;
  thopt.flag = &flag;
  pthread_t th;
  //Get the authentication log from bitcoind
  getCookie(1);
  //Log
  puts("Creating a onchain watcher");
  //Create a thread that will monitor the blockchain
  pthread_create(&th, NULL, handleTipUpdate, &thopt);
  puts("Done!");

  //A serialized block header
  unsigned char ser_block_header[80];
  int tryals = 0; //If we fail more than MAX_RETRY, give up
start: {
  do {
    block = createBlock();
    if(block.version == 0) {
      puts("Something went wrong! Trying again...");
      tryals += 1;
      if (tryals > MAX_RETRY) { return 0; }
    }
  } while (block.version == 0);

  flag = 0;
  tryals = 0;

  //Serialize the block header as raw data. We only hex-encode for submiting
  printf("Creating a block with %d txs\n", block.tx_count);
  memcpy(ser_block_header, &block.version, sizeof(int));
  memcpy(ser_block_header + 4, block.prevBlockHash, 32);
  memcpy(ser_block_header + 36, block.merkleRoot, 32);
  memcpy(ser_block_header + 68, &block.timestamp, sizeof(int));
  memcpy(ser_block_header + 72, &block.bits, sizeof(int));
  memcpy(ser_block_header + 76, &block.nonce, sizeof(int));

  //This function does the magic of mining
  mine(ser_block_header, 0, 100, &flag, submitBlockHeader);

  //1 - We found a block, 2 - Someone else found a block
  if(flag == 1) {
    //A block serialized, this is only used if we find a the block
    unsigned char ser_block[160 + (block.bytes * 2)];
    thopt.height++;
    serialiseBlock(ser_block, ser_block_header, block);
    submitBlock(ser_block);
    /*for (unsigned int i = 0; i < 80; ++i) {
      printf("%02x", ser_block_header[i]);
    }
    printf("\n");*/
  }
  else {
    destroyBlock(&block);
  }

}
//Loop forever
goto start;
  return EXIT_SUCCESS;
}
