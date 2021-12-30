#ifndef BLOCK_PRODUCER_C
#define BLOCK_PRODUCER_C

#include <stdlib.h>

#define NOTNULL(x) __nonnull (x)
#define MAX_RETRY 10

void serialiseBlock(char *ser_block, const unsigned char *ser_block_header, struct block_t block);
void submitBlockHeader(unsigned char block[80]);
void submitBlock(unsigned char *block);
void createMerkleTree(int size, unsigned char transactions[][32], unsigned char *out);
void destroyBlock(struct block_t *block);
struct block_t createBlock();
void serialiseBlock(char *ser_block, const unsigned char *ser_block_header, struct block_t block);

#endif