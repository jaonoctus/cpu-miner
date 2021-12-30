#ifndef BLOCK_H
#define BLOCK_H

//This is an internal representation of a block
struct block_t {
  int version;
  unsigned char prevBlockHash[32];
  unsigned char merkleRoot[32];
  unsigned int timestamp;
  unsigned int bits;
  unsigned int nonce;
  unsigned int tx_count;
  unsigned char **tx;
  unsigned int bytes;
};

#endif //BLOCK_H