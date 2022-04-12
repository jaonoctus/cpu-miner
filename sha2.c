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
__always_inline sha_compress_block_header(unsigned int *h_out, unsigned int *h_in, char *block) {
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, block, 64);
    
    
    A = h_in[0];
    B = h_in[1];
    C = h_in[2];
    D = h_in[3];
    E = h_in[4];
    F = h_in[5];
    G = h_in[6];
    H = h_in[7];
    
    ROUND1(0) ROUND1(1) ROUND1(2) ROUND1(3) ROUND1(4) ROUND1(5) ROUND1(6) ROUND1(7) ROUND1(8) ROUND1(9) 
    ROUND1(10) ROUND1(11) ROUND1(12) ROUND1(13) ROUND1(14) ROUND1(15) ROUND2(16) ROUND2(17) ROUND2(18) ROUND2(19)
    ROUND2(20) ROUND2(21) ROUND2(22) ROUND2(23) ROUND2(24) ROUND2(25) ROUND2(26) ROUND2(27) ROUND2(28) ROUND2(29)
    ROUND2(30) ROUND2(31) ROUND2(32) ROUND2(33) ROUND2(34) ROUND2(35) ROUND2(36) ROUND2(37) ROUND2(38) ROUND2(39)
    ROUND2(40) ROUND2(41) ROUND2(42) ROUND2(43) ROUND2(44) ROUND2(45) ROUND2(46) ROUND2(47) ROUND2(48) ROUND2(49) 
    ROUND2(50) ROUND2(51) ROUND2(52) ROUND2(53) ROUND2(54) ROUND2(55) ROUND2(56) ROUND2(57) ROUND2(58) ROUND2(59)
    ROUND2(60) ROUND2(61) ROUND2(62) ROUND2(63)
    
    h_out[0] = h_in[0] + A;
    h_out[1] = h_in[1] + B;
    h_out[2] = h_in[2] + C;
    h_out[3] = h_in[3] + D;
    h_out[4] = h_in[4] + E;
    h_out[5] = h_in[5] + F;
    h_out[6] = h_in[6] + G;
    h_out[7] = h_in[7] + H;
}
__always_inline void sha_seccond_hash(unsigned int *h_out) {
    unsigned int w[64] = {0}, s0, s1,ch, temp1, maj, temp2, A, B, C, D, E, F, G, H;
    memcpy(w, h_out, 32);

    w[8] = __builtin_bswap32 (0x00000080);
    w[15] = __builtin_bswap32 (0x00010000);
    
    A = h[0];
    B = h[1];
    C = h[2];
    D = h[3];
    E = h[4];
    F = h[5];
    G = h[6];
    H = h[7];

    ROUND0(0) ROUND0(1) ROUND0(2) ROUND0(3) ROUND0(4) ROUND0(5) ROUND0(6) ROUND0(7) ROUND0(8) ROUND0(9) 
    ROUND0(10) ROUND0(11) ROUND0(12) ROUND0(13) ROUND0(14) ROUND0(15) ROUND2(16) ROUND2(17) ROUND2(18) ROUND2(19)
    ROUND2(20) ROUND2(21) ROUND2(22) ROUND2(23) ROUND2(24) ROUND2(25) ROUND2(26) ROUND2(27) ROUND2(28) ROUND2(29)
    ROUND2(30) ROUND2(31) ROUND2(32) ROUND2(33) ROUND2(34) ROUND2(35) ROUND2(36) ROUND2(37) ROUND2(38) ROUND2(39)
    ROUND2(40) ROUND2(41) ROUND2(42) ROUND2(43) ROUND2(44) ROUND2(45) ROUND2(46) ROUND2(47) ROUND2(48) ROUND2(49) 
    ROUND2(50) ROUND2(51) ROUND2(52) ROUND2(53) ROUND2(54) ROUND2(55) ROUND2(56) ROUND2(57) ROUND2(58) ROUND2(59)
    ROUND2(60) ROUND2(61) ROUND2(62) ROUND2(63)

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
