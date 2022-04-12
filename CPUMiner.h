#ifndef CPUMINER_H
#define CPUMINER_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <json-c/json.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
enum flags {
  USE_MIN_DIFF = 1 << 0,
  USE_RPC_USER_AND_PASSWORD = 1 << 1,
  VERBOSE = 1 << 2
};
typedef struct miner_options_s {
  char spk[70];
  char coinbaseValue[100];  //Like in "Chancellor on brink for seccond ballout for banks"
  char cookiefile[100];
  char url[100];
  char rpcPassword[100];
  char rpcUser[100];
  char rpcHost[100];
  char cookie[75];
  struct curl_slist *headers;
  unsigned int network;
  unsigned short port;
  unsigned int threads;
  unsigned long flags;
  
} miner_options_t;

#include "primitives/block.h"
#include "miner.h"
#include "primitives/transaction.h"
#include "sha2.h"
#include "blockproducer.h"
#include "rpc.h"

#endif //CPUMINER_H