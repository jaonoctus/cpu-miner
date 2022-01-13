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

#include "primitives/block.h"
#include "miner.h"
#include "primitives/transaction.h"
#include "sha2.h"
#include "blockproducer.h"
#include "rpc.h"

#define CREATE_BLOCK_CHECK(var)   if (var == NULL) {  \
    struct block_t block = {  \
      .version = 0x00 , \
      .prevBlockHash = {0},\
      .merkleRoot = {0},\
      .timestamp = time(NULL) + 1,\
      .bits = 0x207fffff,\
      .nonce = 0,\
      .tx_count = 0,\
      .tx = NULL,\
      .bytes = 0\
    };\
    assert(json_object_put(root) == 1);\
    \
    return block;\
  }

#define GET_I(root,i) json_object_array_get_idx(root, i)
#define ADD(root, data) json_object_array_add(root, data)

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
//Creates a block if we have a running bitcoind
__attribute__((__warn_unused_result__)) struct block_t createBlock(miner_options_t *opt) {
  struct memory readBuffer;
  
  callRPC(&readBuffer, getBlockTemplateCmd, opt);
  if (readBuffer.size == 0 || readBuffer.response == NULL) {
    printf("Couldn't connect to bitcoin\n");
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
    
    return block;
  }


  json_object *root = json_tokener_parse(readBuffer.response);
  
  CREATE_BLOCK_CHECK(root)

  free(readBuffer.response);

  //Take everithing we need from a block template
  const json_object *result             = json_object_object_get(root, "result");
  CREATE_BLOCK_CHECK(result)
  
  const json_object *transactions       = json_object_object_get(result, "transactions");
  const unsigned char *bits             = json_object_get_string(json_object_object_get(result, "bits"));
  const unsigned int coinbaseValue      = json_object_get_uint64(json_object_object_get(result, "coinbasevalue"));
  const unsigned int version            = json_object_get_int(json_object_object_get(result, "version"));
  const unsigned int height             = json_object_get_int(json_object_object_get(result, "height"));
  const unsigned int minTime            = json_object_get_int(json_object_object_get(result, "mintime"));
  const unsigned char *segwitCommit     = json_object_get_string(json_object_object_get(result, "default_witness_commitment"));
  const unsigned char *prevBlockHashStr = json_object_to_json_string_ext(json_object_object_get(result, "previousblockhash"), 0);
  
  //None of theese can be NULL
  CREATE_BLOCK_CHECK(result);
  CREATE_BLOCK_CHECK(transactions);
  CREATE_BLOCK_CHECK(bits);
  CREATE_BLOCK_CHECK(segwitCommit);

  // +1 because coinbase
  const unsigned int count = json_object_array_length(transactions) + 1;
  unsigned char hashes[count][32], aux[32];
  struct coinbase_t coinbase =  createCoinbase();
  /* We need all txIds, for calculating the Merkle Tree */
  //Starting with the coinbase

  fillTransaction(&coinbase, segwitCommit, height, coinbaseValue, opt->spk, strlen(opt->spk), opt->coinbaseValue);
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
    .timestamp = minTime + (30 *  60),
    .bits = 0x1d00ffff,
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
  
  /*if (!(opt->flags & USE_MIN_DIFF)) {
    unsigned int nBits;
    str2bytes((unsigned char *) &nBits, bits, 8);
    block.bits = __builtin_bswap32(nBits);
  }
  else {
    block.bits = __builtin_bswap32(block.bits);
  }*/

  assert(json_object_put(root) == 1);
  return block;
}

//Send a header with "submitheader"
NOTNULL((1)) void submitBlockHeader(unsigned char block[80], miner_options_t *opt) {
  unsigned char cmd[10000], ser_block[(80 * 2) + 1];
  struct memory ret;
  for (unsigned int i = 0; i < 80; ++i)
    sprintf(ser_block + (2 * i), "%02x", block[i]);
  sprintf(cmd, submitHeader, ser_block);
  callRPC(&ret, cmd, opt);

  if(ret.response == NULL || ret.size <=0) return;
  if(strncmp(ret.response + 23, "null", 4))
    printf("%s\n", ret);
  free(ret.response);
}
//Submit a block with "submitblock"
NOTNULL((1)) void submitBlock(unsigned char *block, miner_options_t *opt) {
  int cmdLen = strlen(block) + strlen(submitBlockBase) + 500;
  char cmd[cmdLen];
  
  struct memory ret;

  sprintf(cmd, submitBlockBase, block);
  callRPC(&ret, cmd, opt);
  free(ret.response);
}

NOTNULL((1, 2)) void serialiseBlock(char *ser_block, const unsigned char *ser_block_header, struct block_t block) {
  int flag = 0;

  for (unsigned register int i = 0; i < 80; ++i) {
    sprintf(ser_block + (2 * i), "%02x", ser_block_header[i]);
  }

  assert(block.tx_count < 255);

  if (block.tx_count < 0xFD) {
    sprintf(ser_block + 160, "%02x", block.tx_count);
    ser_block[162] = 0;
  } else if (block.tx_count <= 0xFFFF) {
    sprintf(ser_block + 160, "0xfd%04x", (uint32_t) __builtin_bswap16(block.tx_count));
    ser_block[164] = 0;
  }
  else if (block.tx_count <= 0xFFFFFFFF) {
    sprintf(ser_block + 160, "0xfe%08x", (uint32_t) __builtin_bswap32(block.tx_count));
    ser_block[168] = 0;
  }

  for (unsigned register int i = 0; i < block.tx_count; ++i) {
    strcat(ser_block, block.tx[i]);
  }

  destroyBlock(&block);
}

