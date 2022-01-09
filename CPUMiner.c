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
#include "CPUMiner.h"

int flag;
typedef struct thread_opt_s {
  int *flag;
  int height;
  miner_options_t *opt;
} thread_opt_t;
enum PORTS{
  PORT_MAINNET = 8333,
  PORT_TESTNET = 18332,
  PORT_REGTEST = 18443,
  PORT_SIGNET = 8333 //TODO
};

enum NETWORKS {
  MAINNET = 1 << 1,
  TESTNET = 1 << 2,
  REGTEST = 1 << 3,
  SIGNET =  1 << 4
};
void dumpOpts(miner_options_t *opt) {
  printf("  rpcport = %d\n", opt->port);
  printf("  coinbasevalue = %s\n", opt->coinbaseValue);
  printf("  cookie = %s\n", opt->cookie);
  printf("  datadir = %s\n", opt->datadir);
  printf("  spk = %s\n", opt->spk);
}
miner_options_t parseArgs(int argc, char **argv) {
  miner_options_t opt = {
    .spk = "0014546a43c83cc73cb785ed722ad613f6f3c4a6b3e2",
    .coinbaseValue = "discord.bitcoinheiros.com",
    .network = TESTNET,
    .port = PORT_TESTNET,
    .url = "%s@localhost:%d",
  };

  for (unsigned register int i = 1; i < argc; ++i) {
    if ((argc - i) == 1 || argv[i][0] != '-') {
      printf("Invalid option %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    if(strcmp("-datadir", argv[i]) == 0) {
      if(strlen(argv[++i]) > 100) {
        puts("-datadir too big!");
        exit(EXIT_FAILURE);
      }
      strcpy(opt.datadir, argv[i]);
    }
    else if(strcmp("-spk", argv[i]) == 0) {
      if(strlen(argv[++i]) > 70) {
        puts("-spk too big!");
        exit(EXIT_FAILURE);
      }
      strcpy(opt.spk, argv[i]);
    }
    else if(strcmp("-network", argv[i]) == 0) {
      if(strcmp(argv[++i], "mainnet") == 0) {
        opt.network = MAINNET;
        opt.port = PORT_MAINNET;
      }
      else if(strcmp(argv[i], "testnet") == 0) {
        opt.network = TESTNET;
        opt.port = PORT_TESTNET;
      }
      else if(strcmp(argv[i], "signet")  == 0) {
        opt.network = SIGNET;
        opt.port = PORT_SIGNET;
      }
      else if(strcmp(argv[i], "regtest") == 0) {
        opt.network = REGTEST;
        opt.port = PORT_REGTEST;
      }
      else {
        puts("Unknow network");
        exit(EXIT_FAILURE);
      }
    }
    else if (strcmp("-coinbasevalue", argv[i]) == 0) {
      if(strlen(argv[++i]) > 100) {
        puts("Coinbase value too big!");
        exit(EXIT_FAILURE);
      }
      strcpy(opt.coinbaseValue, argv[i]);
    }
    else {
      puts("Unrecognized option");
      exit(EXIT_FAILURE);
    }
  }
  return opt;
}
static void getCookie(miner_options_t *opt) {
  FILE *fcookie;
  fcookie = fopen(opt->datadir, "r");
  if(!fcookie) {
    puts("Error reading cookie!");
    exit(EXIT_FAILURE);
  }
  fread(opt->cookie, sizeof(char), 75, fcookie);
}

NOTNULL((1)) void *scheduler(void *attr) {
  struct memory readBuffer;
  thread_opt_t *attrs = (thread_opt_t *) attr;
  time_t nextUpdate = time(NULL) + 10 * 60;
  attrs->height = 0;
  *attrs->flag = 0;
  callRPC(&readBuffer, getBlockchaininfo, attrs->opt);
  readBuffer.response[35 + 7] = '\0';

  if(readBuffer.response == NULL) return (void *) EXIT_FAILURE;
  
  sscanf(readBuffer.response + 35, "%d", &attrs->height);
  printf("Tip: %d\n", attrs->height);
  free(readBuffer.response);

  int newHeight;

  do {
    callRPC(&readBuffer, getBlockchaininfo, attrs->opt);
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
    sleep(3);

  } while(*(attrs->flag) == 0);
}

void handle_sigint() {
  printf("Shutdown requested\n");
  flag = 3;
}

int main(int argc, char **argv) {
  puts("Wellcome to my CPU miner");
  puts("(C) Davidson Souza - This software is distributed under MIT licence");
  puts("Starting...");

  miner_options_t opt = parseArgs(argc, argv);
  
  opt.headers = curl_slist_append(opt.headers, "Content-Type: application/json");
  signal(SIGINT, handle_sigint);

  struct block_t block;  
  thread_opt_t thopt;
  
  flag = 0;
  thopt.height = 14;
  thopt.flag = &flag;
  thopt.opt = &opt;
  pthread_t th;
  if(opt.datadir[0] == 0) {
    puts("Required datadir");
    exit(EXIT_FAILURE);
  }
  //Get the authentication log from bitcoind
  getCookie(&opt);

  //Log
  dumpOpts(&opt);
  puts("Creating a onchain watcher");

  //Create a thread that will monitor the blockchain
  pthread_create(&th, NULL, scheduler, &thopt);
  puts("Done!");
  mine(&flag, &opt);
  return EXIT_SUCCESS;
}
