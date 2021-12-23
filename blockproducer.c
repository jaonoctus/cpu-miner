#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <json-c/json.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include "block.h"
#include "miner.h"
#include "transaction.c"
#include "sha2.h"

#define GET_I(root,i) json_object_array_get_idx(root, i)
#define ADD(root, data) json_object_array_add(root, data)
#define MAX_RETRY 10


//Bitcoind's auth cookie
char cookie[76] = {0}; 
//Some useful constants that will be used during rpc stuff
static const char *getBlockchaininfo   =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblockchaininfo\",\"params\":[]}";
static const char *getBlockTemplateCmd =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblocktemplate\",\"params\":[{\"rules\":[\"segwit\"]}]}";
static const char *getBlock            =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblock\",\"params\":[\"%s\"]}";
static const char *submitHeader        =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"submitheader\",\"params\":[\"%s\"]}";
static const char *submitBlockBase     =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"submitblock\",\"params\":[\"%s\"]}";
//Payout address
static const char *address             =  (char *) "0014546a43c83cc73cb785ed722ad613f6f3c4a6b3e2";  //A spk, actually
struct curl_slist *headers;

static const unsigned char mask[] = {
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                                0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C,
                                0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            }; 
                
const void str2bytes(unsigned char *out, const unsigned char *str, int size) {
    if(out == NULL){
        printf("out must not be null");
        exit(1) ;
    }
    for(unsigned int i = 0; i < size/2; i ++) {
        out[i] = (mask[str[(2*i)]] << 4) | (mask[str[ ((2*i)) + 1]]);
    }
}
//This will be used for store RPC call's result
struct memory {
  char *response;
  size_t size;
};
 
NOTNULL((1, 4)) static size_t writeCallback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;
  char *ptr;
  if(mem->size == 0) {
    ptr = malloc(mem->size + realsize + 1);
  } else {
    ptr = realloc(mem->response, mem->size + realsize + 1);
  }
  if(ptr == NULL) {
    printf("Error!\n");
    return 0;  /* out of memory! */
  }
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}

static void getCookie(int testNet)
{
  FILE *fcookie;
  if(testNet)
    fcookie = fopen("/home/erik/.bitcoin/testnet3/.cookie", "r");
  else
    fcookie = fopen("/home/erik/.bitcoin/.cookie", "r");
  if(fcookie == NULL)
  {
    puts("Error: Reading cookie file");
    exit(1);
  }
  fread(cookie, sizeof(char), 75, fcookie);
}

NOTNULL((1, 2)) static void callRPC(struct memory *out, const char *data) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  out->size = 0;
  out->response = NULL;

  if(curl) {
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    char url[1000] = {0};
    sprintf(url, "http://%s@127.0.0.1:18332", cookie);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
} 
NOTNULL((1, 2)) void sha256d(unsigned char *out, const unsigned char *src, unsigned int size) {
  sha_ctx ctx;
  sha_init(&ctx);
  sha_update(&ctx, src, size);
  sha_finalize(&ctx, out);
  
  sha_init(&ctx);
  sha_update(&ctx, out, 32);
  sha_finalize(&ctx, out);
}

NOTNULL((1, 2, 3)) void pair_sha256(unsigned char *out, unsigned char *e1, unsigned char *e2) {
  unsigned char content[64];
  memcpy(content, e1, 32);
  memcpy(content + 32, e2, 32);

  sha_ctx ctx;
  sha_init(&ctx);
  sha_update(&ctx, content, 64);
  sha_finalize(&ctx, out);

  sha_init(&ctx);
  sha_update(&ctx, out, 32);
  sha_finalize(&ctx, out);
}
NOTNULL((1, 2)) void be2le(unsigned char *out, const unsigned char *in) {
  for(unsigned int i = 0; i < 32; ++i)
  {
    out[i] = in[31-i];
  }
}
//Send a header with "submitheader"
 NOTNULL((1)) void submitBlockHeader(unsigned char block[80]) {
  unsigned char cmd[10000], ser_block[(80 * 2) + 1];
  struct memory ret;
  for (unsigned int i = 0; i < 80; ++i)
    sprintf(ser_block + (2 * i), "%02x", block[i]);
  sprintf(cmd, submitHeader, ser_block);
  callRPC(&ret, cmd);

  if(ret.response == NULL || ret.size <=0) return;
  if(strncmp(ret.response + 23, "null", 4))
    printf("%s\n", ret);
  free(ret.response);
}
//Submit a block with "submitblock"
 NOTNULL((1)) void submitBlock(unsigned char *block) {
  int cmdLen = strlen(block) + strlen(submitBlockBase) + 500;
  char cmd[cmdLen];
  
  struct memory ret;

  sprintf(cmd, submitBlockBase, block);
  callRPC(&ret, cmd);
  free(ret.response);
}
//Create a Merkle Tree and returns the Merkle Root
NOTNULL((2, 3)) void createMerkleTree(int size, unsigned char transactions[][32], unsigned char *out) {
  unsigned char level[(size % 2) == 0 ? size /2 : (size + 1) / 2][32];
  if (size == 1) {
    memcpy(out, transactions[0], 32);
    return ;
  }

  if (size == 2) {
    pair_sha256(out, transactions[0], transactions[1]);
    return ;
  }

  if (size % 2 == 1) {
    size += 1;
    for(int i = 0; i < ((size ) / 2); ++i) {
      if(((2 * i) + 2) == size) {
        pair_sha256(level[i], transactions [2 * i], transactions[2 * i]);
      } else {
        pair_sha256(level[i], transactions [2 * i], transactions[(2 * i) + 1]);
      }
    }
  } else {
    for(int i = 0; i < size / 2; ++i) {
      pair_sha256(level[i], transactions [2 * i],transactions[(2 * i) + 1]);
    }
  }
  createMerkleTree(size / 2, level, out);
}
NOTNULL((1, 2)) void getRawTransactionHash(unsigned char out[32], unsigned char in[68]){
  str2bytes(out, in, 64);
}

NOTNULL((1)) void destroyBlock(struct block_t *block) {
  for(int i = 0; i < block->tx_count; ++i) {
    free(block->tx[i]);
  }
  free(block->tx);
}
__attribute__((__warn_unused_result__)) struct block_t createBlock() {
  struct memory readBuffer;
  
  callRPC(&readBuffer, getBlockTemplateCmd);
  assert(readBuffer.size > 0 && readBuffer.response != NULL);

  json_object *root = json_tokener_parse(readBuffer.response);

  assert(root != NULL);
  free(readBuffer.response);

  //Take everithing we need from a block template
  const json_object *result             = json_object_object_get(root, "result");
  assert(result != NULL);

  const json_object *transactions       = json_object_object_get(result, "transactions");
  const unsigned char *bits             = json_object_get_string(json_object_object_get(result, "bits"));
  const unsigned int coinbaseValue      = json_object_get_uint64(json_object_object_get(result, "coinbasevalue"));
  const unsigned int version            = json_object_get_int(json_object_object_get(result, "version"));
  const unsigned int height             = json_object_get_int(json_object_object_get(result, "height"));
  const unsigned char *segwitCommit     = json_object_get_string(json_object_object_get(result, "default_witness_commitment"));
  const unsigned char *prevBlockHashStr = json_object_to_json_string_ext(json_object_object_get(result, "previousblockhash"), 0);
  
  //None of theese can be NULL
  if (!result || !transactions || !bits || !segwitCommit ) {
    struct block_t block = {
      .version = 0x00 ,
      .prevBlockHash = {0},
      .merkleRoot = {0},
      .timestamp = time(NULL) + 1,
      .bits = 0x207fffff,
      .nonce = 0,
      .tx_count = 0,
      .tx = NULL,
      .bytes = 0
    };
    assert(json_object_put(root) == 1);

    return block;
  }

  // +1 because coinbase
  const unsigned int count = json_object_array_length(transactions) + 1;
  unsigned char hashes[count][32], aux[32];
  struct coinbase_t coinbase =  createCoinbase();
  /* We need all txIds, for calculating the Merkle Tree */
  //Starting with the coinbase

  fillTransaction(&coinbase, segwitCommit, height, coinbaseValue, address, strlen(address));
  getTransactionId(aux, coinbase);
  memcpy(hashes[0], aux, 32);

  //and fill up with more tx's
  for (unsigned register int i = 1; i < count; ++i) { 
    unsigned char *tx = (unsigned char *)json_object_get_string(json_object_object_get(json_object_array_get_idx((struct json_object *) transactions, i - 1), "txid"));
    assert(tx != NULL);
    getRawTransactionHash(aux, tx);
    be2le(hashes[i], aux);
  }

  unsigned char merkleRoot[32] = {0}, prevBlockHash[32] = {0};

  //Create a Merkle Tree
  createMerkleTree(count, hashes, merkleRoot);
  str2bytes(prevBlockHash, (unsigned char *) ++prevBlockHashStr, 64);
  
  //Now we can create a block
  struct block_t block = {
    .version = 0x02000000,
    .prevBlockHash = {0},
    .merkleRoot = {0},
    .timestamp = time(NULL) + (30 *  60),
    .bits = 0xffff001d,
    .nonce = 0,
    .tx_count = count,
    .tx = malloc(block.tx_count * sizeof(char *)),
    .bytes = 0
  };
  //Coinbase
  char serTx [getSerSize(coinbase)];

  const unsigned int ser_size = serializeCoinbase(serTx, coinbase);
  block.tx[0] = (char *) malloc(ser_size + 1 * sizeof(char *));
  block.bytes += ser_size;

  memcpy(block.tx[0], serTx, ser_size);
  block.tx[0][ser_size] = 0;

  //Since coinbase_t have free storage allocation, it's good to destroy it explicitly
  destroyTransaction(&coinbase);

  //Copy transaction data, will be used during serialization
  for (unsigned int i = 1; i < count; ++i) {
    json_object *tx = json_object_object_get (json_object_array_get_idx(transactions, i - 1), "data");
    if(tx != NULL) {
      const int size = json_object_get_string_len(tx) + 1;
      if (size == 0) {
        printf("Erro: Reading transaction %d\n", i);
        exit(1);
      }
      block.tx[i] = (char *) malloc(size * sizeof(char) + 1);
      block.bytes += size;
      memcpy(block.tx[i], (char *)json_object_get_string(tx), size);
      block.tx[i][size - 1] = 0;
    }
    else {
      printf("Erro: Reading transaction %d\n", i);
      exit(1);
    }
  }

  memcpy(block.merkleRoot, merkleRoot, 32);
  be2le(block.prevBlockHash, prevBlockHash);
  
  //unsigned int nBits;
  //str2bytes((unsigned char *) &nBits, bits, 8);
  block.bits = __builtin_bswap32(block.bits);  //Uncomment this line if you want the actual nBits
  assert(json_object_put(root) == 1);
  return block;
}

NOTNULL((1, 2)) void serialiseBlock(char *ser_block, const unsigned char *ser_block_header, struct block_t block) {
  int flag = 0;

  for (unsigned register int i = 0; i < 80; ++i) {
    sprintf(ser_block + (2 * i), "%02x", ser_block_header[i]);
  }

  assert(block.tx_count < 255);

  sprintf(ser_block + 160, "%02x", block.tx_count);

  ser_block[162] = 0;
  
  for (unsigned register int i = 0; i < block.tx_count; ++i) {
    strcat(ser_block, block.tx[i]);
  }

  destroyBlock(&block);
}

typedef struct thread_opt_s {
  int *flag;
  int height;
} thread_opt_t;

NOTNULL((1)) void *handleTipUpdate(void *attr) {
  struct memory readBuffer;
  thread_opt_t *attrs = (thread_opt_t *) attr;

  callRPC(&readBuffer, getBlockchaininfo);
  readBuffer.response[35 + 7] = '\0';

  if(readBuffer.response == NULL) return (void *) EXIT_FAILURE;
  
  sscanf(readBuffer.response + 35, "%d", &attrs->height);
  printf("Tip: %d\n", attrs->height);
  free(readBuffer.response);
  int newHeight;

  do {
    callRPC(&readBuffer, getBlockchaininfo);
    if(readBuffer.size > 50) {
      readBuffer.response[35 + 7] = '\0';

      sscanf(readBuffer.response + 35, "%d", &newHeight);

      if (attrs->height != newHeight) {
        *(attrs->flag) = 2;
        attrs->height = newHeight;
        printf("Tip updated %d\n", attrs->height);
      }
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
  thopt.height = 2131639;
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
  puts("Creating a block");
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
    for (unsigned int i = 0; i < 80; ++i) {
      printf("%02x", ser_block_header[i]);
    }
  }
  else {
    destroyBlock(&block);
  }

}
//Loop forever
goto start;
  return EXIT_SUCCESS;
}
