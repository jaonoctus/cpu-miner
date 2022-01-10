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
  printf("  actualdiff = %s\n", ! (opt->flags & USE_MIN_DIFF) ? "true" : "false");
}
void usage() {
  puts("./CPUMiner <name> <value> ... <name> <value>");
  puts("args:");
  puts(" -datadir <datadir>    Were I can find a .cookie file e.g: \n                        /home/alice/.bitcoin/testnet3/.cookie");
  puts(" -network <network>    Witch network we are mining on. mainnet, testnet\n                        signet testnet");
  puts(" -spk <spk>            Generated coins will be sent to spk\n");
  puts(" -rpchost <host>       A hostname for comunicating with bitcoind\n");
  puts(" -rpcuser <user>       An user for RPC authentication\n");
  puts(" -rpcpassword <pass>   A password for RPC authentication\n");
}

miner_options_t parseArgs(int argc, char **argv) {
  miner_options_t opt = {
    .spk = "0014546a43c83cc73cb785ed722ad613f6f3c4a6b3e2",
    .coinbaseValue = "discord.bitcoinheiros.com",
    .network = TESTNET,
    .port = PORT_TESTNET,
    .rpcHost = "localhost"
  };

  for (unsigned register int i = 1; i < argc; ++i) {
    if ((argc - i) == 1 || argv[i][0] != '-') {
      printf("Invalid option %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }

    if(strcmp("-actualdiff", argv[i]) == 0) {
      opt.flags |= argv[++i][0] == '1'?  0 : USE_MIN_DIFF;
    }
    else if (strcmp("-rpcpassword", argv[i]) == 0) {
      if(strlen(argv[++i]) > 100) {
        puts("-rpcpassword too big!");
        exit(EXIT_FAILURE);        
      }
      opt.flags |= USE_RPC_USER_AND_PASSWORD;
      strcpy(opt.rpcPassword, argv[i]);
    }
    else if (strcmp("-rpcuser", argv[i]) == 0) {
      if(strlen(argv[++i]) > 100) {
        puts("-rpcuser too big!");
        exit(EXIT_FAILURE);        
      }
      opt.flags |= USE_RPC_USER_AND_PASSWORD;
      strcpy(opt.rpcUser, argv[i]);
    }
    else if (strcmp("-rpchost", argv[i]) == 0) {
      if(strlen(argv[++i]) > 100) {
        puts("-rpchost too big!");
        exit(EXIT_FAILURE);        
      }
      strcpy(opt.rpcHost, argv[i]);
    }
    else if (strcmp("-rpccookie", argv[i]) == 0) {
      if (strlen(argv[++i]) > 100) {
        puts ("-rpccookie too big!");
        exit (EXIT_FAILURE);        
      }
      if (opt.flags & USE_RPC_USER_AND_PASSWORD) {
        puts ("Using -rpccookie and -rpcuser makes no sense!");
        exit (EXIT_FAILURE);
      }
      strcpy(opt.cookie, argv[i]);
    }
    else if (strcmp("-datadir", argv[i]) == 0) {
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
      printf("Unrecognized option %s\n", argv[i]);
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
  puts("Scheduler is now working");

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

void handle_signal() {
  printf("Shutdown requested\n");
  flag = 3;
}

int main(int argc, char **argv) {
  puts("Welcome to my CPU miner");
  puts("(C) Davidson Souza - This software is distributed under MIT licence");

  if(argc <= 1) {
    usage();
    exit(EXIT_FAILURE);
  }

  miner_options_t opt = parseArgs(argc, argv);
  
  opt.headers = curl_slist_append(opt.headers, "Content-Type: application/json");
  signal(SIGINT, handle_signal);
  signal(SIGABRT, handle_signal); //SIGABRT is called on abort() that is called in assert()

  struct block_t block;  
  thread_opt_t thopt;

  if(opt.datadir[0] == 0) {
    puts("ERROR: -datadir is required");
    exit(EXIT_FAILURE);
  }  

  puts("Starting...");

  //Get the authentication cookie from bitcoind
  if(!(opt.flags & USE_RPC_USER_AND_PASSWORD)) 
    getCookie(&opt);

  //Log
  dumpOpts(&opt);
  
  //Check if RPC is working
  struct memory readBuffer;
  callRPC(&readBuffer, getBlockchaininfo, &opt);
  if(readBuffer.response == NULL ||  readBuffer.size == 0) {
    puts("Failled connecting to RPC");
    exit(EXIT_FAILURE);
  }

  flag = 0;
  thopt.height = 14;
  thopt.flag = &flag;
  thopt.opt = &opt;

  pthread_t th;
  //Create a thread that will monitor the blockchain and reeschedule things
  pthread_create(&th, NULL, scheduler, &thopt);
  puts("Done!");
  mine(&flag, &opt);
  return EXIT_SUCCESS;
}
