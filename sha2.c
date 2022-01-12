#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "sha2.h"
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
void sha_precompute(unsigned int *h_out,  unsigned char *block) {
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, block, 64);
    
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    FOR(16) {
        w[i] = __builtin_bswap32(w[i]);
    }
    #endif
    for (unsigned int i=16; i<=63; ++i) {
        s0 = ROTR(w[i-15], 7) ^ ROTR(w[i-15], 18) ^ (w[i-15] >>  3);
        s1 = ROTR(w[i- 2],17) ^ ROTR(w[i- 2], 19) ^ (w[i- 2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    
    A = h[0];
    B = h[1];
    C = h[2];
    D = h[3];
    E = h[4];
    F = h[5];
    G = h[6];
    H = h[7];

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

    h_out[0] = h[0] + A;
    h_out[1] = h[1] + B;
    h_out[2] = h[2] + C;
    h_out[3] = h[3] + D;
    h_out[4] = h[4] + E;
    h_out[5] = h[5] + F;
    h_out[6] = h[6] + G;
    h_out[7] = h[7] + H;
}
void sha_compress_block_header(unsigned int *h_out, unsigned int *h_in, char *block) {
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, block, 64);
    
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    FOR(16) {
        w[i] = __builtin_bswap32(w[i]);
    }

    #endif
    for (unsigned int i=16; i<=63; ++i) {
        s0 = ROTR(w[i-15], 7) ^ ROTR(w[i-15], 18) ^ (w[i-15] >>  3);
        s1 = ROTR(w[i- 2],17) ^ ROTR(w[i- 2], 19) ^ (w[i- 2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    
    A = h_in[0];
    B = h_in[1];
    C = h_in[2];
    D = h_in[3];
    E = h_in[4];
    F = h_in[5];
    G = h_in[6];
    H = h_in[7];

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
    h_out[0] = h_in[0] + A;
    h_out[1] = h_in[1] + B;
    h_out[2] = h_in[2] + C;
    h_out[3] = h_in[3] + D;
    h_out[4] = h_in[4] + E;
    h_out[5] = h_in[5] + F;
    h_out[6] = h_in[6] + G;
    h_out[7] = h_in[7] + H;
}
void sha_seccond_hash(unsigned int *h_out) {
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, h_out, 32);
    FOR(8) {
        w[i] = __builtin_bswap32(w[i]);
    }

    w[8] = 0x00000080;
    w[15] = 0x00010000;

    #if __BYTE_ORDER == __LITTLE_ENDIAN
    FOR(16) {
        w[i] = __builtin_bswap32(w[i]);
    }
    #endif
    for (unsigned int i=16; i<=63; ++i) {
        s0 = ROTR(w[i-15], 7) ^ ROTR(w[i-15], 18) ^ (w[i-15] >>  3);
        s1 = ROTR(w[i- 2],17) ^ ROTR(w[i- 2], 19) ^ (w[i- 2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    
    A = h[0];
    B = h[1];
    C = h[2];
    D = h[3];
    E = h[4];
    F = h[5];
    G = h[6];
    H = h[7];

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

    h_out[0] = h[0] + A;
    h_out[1] = h[1] + B;
    h_out[2] = h[2] + C;
    h_out[3] = h[3] + D;
    h_out[4] = h[4] + E;
    h_out[5] = h[5] + F;
    h_out[6] = h[6] + G;
    h_out[7] = h[7] + H;
}
#ifdef TEST

int main()
{
    unsigned char block1[128] = {0}, block2[80] = {0};
    unsigned int hs[8];
    block1[80] = 0x80;
    block1[126] = 0x02;
    block1[127] = 0x80;

    sha_precompute(hs, block1);
    sha_compress_block_header(hs, hs, block1 + 64);
    sha_seccond_hash(hs);

    printf("%08x", hs[0]);
    printf("%08x", hs[1]);
    printf("%08x", hs[2]);
    printf("%08x", hs[3]);
    printf("%08x", hs[4]);
    printf("%08x", hs[5]);
    printf("%08x", hs[6]);
    printf("%08x", hs[7]);
    printf("\n");

    sha_ctx ctx;
    unsigned char hash[32];
    sha_init(&ctx);
    sha_update(&ctx, block2, 80);
    sha_finalize(&ctx, hash);

    sha_init(&ctx);
    sha_update(&ctx, hash, 32);
    sha_finalize(&ctx, hash);
        
    for (unsigned register int i = 0; i < 32; ++i) 
        printf("%02x", hash[i]);
    printf("\n");
}
#endif  //test
