#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifndef SHA2_C
#define SHA2_C

#define ROTR(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define Ch() (E & F) ^ (~E & G)
#define Ma() (A & B) ^ (A & C) ^ (B & C)
#define S0() (ROTR(A, 2)) ^ (ROTR(A, 13)) ^ (ROTR(A, 22))

#define FOR(k) for(unsigned int i = 0;i<k; ++i)
#define S1() (ROTR(E, 6)) ^ (ROTR(E, 11)) ^ (ROTR(E, 25))
typedef struct sha_ctx_s
{
    unsigned int h[8];
    unsigned char block[64];
    unsigned int size;
} sha_ctx;

const static unsigned int h[8] = 
{
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};
const static unsigned int k[64] =
{
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
/**
 * @brief: Consumes one block
*/
void sha_compress(sha_ctx *shaCtx)
{
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, shaCtx->block, sizeof(unsigned int ) * 16);
    
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    FOR(16)
    {
        w[i] = __builtin_bswap32(w[i]);
    }
    #endif
    for (unsigned int i=16; i<=63; ++i)
    {
        s0 = ROTR(w[i-15], 7) ^ ROTR(w[i-15], 18) ^ (w[i-15] >>  3);
        s1 = ROTR(w[i- 2],17) ^ ROTR(w[i- 2], 19) ^ (w[i- 2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    
    A = shaCtx->h[0];
    B = shaCtx->h[1];
    C = shaCtx->h[2];
    D = shaCtx->h[3];
    E = shaCtx->h[4];
    F = shaCtx->h[5];
    G = shaCtx->h[6];
    H = shaCtx->h[7];

    /**
     * @todo: Unroll this loop 
     **/
    for(unsigned int i = 0; i<=63; ++i)
    {
        s1 = S1();
        ch = Ch();
        temp1 = H + s1 + ch + k[i] + w[i];
        s0 = S0();
        maj = Ma();
        temp2 = s0 + maj;

        H = G;
        G = F;
        F = E;
        E = D + temp1;
        D = C;
        C = B;
        B = A;
        A = temp1 + temp2;
    }
    shaCtx->h[0] += A;
    shaCtx->h[1] += B;
    shaCtx->h[2] += C;
    shaCtx->h[3] += D;
    shaCtx->h[4] += E;
    shaCtx->h[5] += F;
    shaCtx->h[6] += G;
    shaCtx->h[7] += H;
    memset(shaCtx->block, 0x00, 64);
}
/**
 * @brief: Initiates a new context
*/
void sha_init(sha_ctx *shaCtx)
{
    memcpy(shaCtx->h, h, 8 * sizeof(unsigned int));
    memset(shaCtx->block, 0, 64);
    shaCtx->size = 0;
}
/**
 * @brief: Inserts more data to the digest
*/
void sha_update(sha_ctx *shaCtx, const unsigned char *block, int size)
{
    if(((shaCtx->size % 64) + size) == 64)
    {
        memcpy(shaCtx->block + (shaCtx->size % 64), block, size);
        shaCtx->size += size;

        return sha_compress(shaCtx);
    }
    if(((shaCtx->size % 64) + size) > 64)
    {
        int count = 64 - (shaCtx->size % 64);
        memcpy(shaCtx->block + (shaCtx->size % 64), block, count);
        sha_compress(shaCtx);
        shaCtx->size += count;
        return sha_update(shaCtx, block + count, size - count);
    }
    
    memcpy(shaCtx->block + (shaCtx->size % 64), block, size);
    shaCtx->size += size;
}
void sha_finalize(sha_ctx *ctx, unsigned char ret[32])
{
    ctx->block[(ctx->size) % 64] = 0x80;
    if((ctx->size % 64) >= 56)
    {
        sha_compress(ctx);
    }    
    unsigned long _size = ctx->size * 8;

#if __BYTE_ORDER == __LITTLE_ENDIAN
    _size = __builtin_bswap32(_size);
    memcpy(ctx->block + 60, &_size, sizeof(unsigned long));
    sha_compress(ctx);
    FOR(8)
    {
        ctx->h[i] = __builtin_bswap32(ctx->h[i]);
    }
    memcpy(ret, ctx->h, 32);

#else
    memcpy(ctx->block + 60, &_size, sizeof(unsigned long));
    sha_compress(ctx);

    memcpy(ret, ctx->h, 32); 
#endif


}

#ifdef TEST

int main()
{
    char *str = (char *) malloc(1000000);
    for(unsigned register int i = 0; i<1000000; ++i)
    {
        str[i] = 'a';
    }
    unsigned char buf[64] = {0};
    struct sha_ctx ctx;
    sha_init(&ctx);
    sha_update(&ctx, str, 1000000);
    sha_finalize(&ctx, buf);
    FOR(32)
    {
        printf("%02x", buf[i]);
    }
    printf("\n");
    return 0;
}
#endif  //test
#endif  //sha2_c