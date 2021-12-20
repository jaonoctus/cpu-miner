#ifndef SHA2_H
#define SHA2_H

typedef struct sha_ctx_s
{
    unsigned int h[8];
    unsigned char block[64];
    unsigned int size;
} sha_ctx;

void sha_compress(sha_ctx *shaCtx);
void sha_init(sha_ctx *shaCtx);
void sha_update(sha_ctx *shaCtx, const unsigned char *block, int size);
void sha_finalize(sha_ctx *ctx, unsigned char ret[32]);

#endif //SHA2_H