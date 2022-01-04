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
#include <signal.h>


#include "primitives/block.h"
#include "miner.h"
#include "sha2.h"
#include "primitives/transaction.h"
#include "blockproducer.h"
#include "rpc.h"

struct curl_slist *headers;

//Bitcoind's auth cookie
char cookie[76] = {0}; 
int flag;
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

NOTNULL((1)) void *scheduler(void *attr) {
  struct memory readBuffer;
  thread_opt_t *attrs = (thread_opt_t *) attr;
  time_t nextUpdate = time(NULL) + 10 * 60;
  attrs->height = 0;
  *attrs->flag = 0;
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
    if(*attrs->flag == 3) return NULL;
    
    if(nextUpdate <= time(NULL)) {
      *(attrs->flag) = 2;
      nextUpdate += 10 * 60;
    }
    if(readBuffer.response != NULL)
      free(readBuffer.response);
    sleep(10);

  } while(*(attrs->flag) == 0);
}

void handle_sigint() {
  printf("Shutdown requested\n");
  flag = 3;
}

int main() {
  headers = curl_slist_append(headers, "Content-Type: application/json");
  signal(SIGINT, handle_sigint);

  struct block_t block;  
  thread_opt_t thopt;
  
  flag = 0;
  thopt.height = 14;
  thopt.flag = &flag;
  pthread_t th;

  //Get the authentication log from bitcoind
  getCookie(1);

  //Log
  puts("Creating a onchain watcher");

  //Create a thread that will monitor the blockchain
  pthread_create(&th, NULL, scheduler, &thopt);
  puts("Done!");

  mine(&flag);
  return EXIT_SUCCESS;
}
