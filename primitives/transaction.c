#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "transaction.h"
#include "../sha2.h"

const void str2bytes(unsigned char *out, const unsigned char *str, int size);
#define NOTNULL(x) __nonnull (x)
#define INIT_SER_METHOD(ser)    const unsigned int size = getSerSize(ser);\
                                unsigned int offset = 0;

#define END_SER_METHODS(ser)    memcpy(ser, acc, size)
static const char *coinbase_data = "discord.bitcoinheiros.com";

#define SER_INT(x) offset += serInt(acc + offset, x);
#define SER_BYTE(x) offset += serByte(acc + offset, x);
#define SER_BYTEARRAY(x, len) offset += setByteArray(acc + offset, x, len);
#define SER_LONG(x) offset += serLong(acc + offset, x);

int getSerSize(struct coinbase_t coinbase) {
  unsigned int size = 0;
  size += 4;  // version
  size += 1;  // nInputs
  for (unsigned int i = 0; i < coinbase.nInputs; ++i) {
    size += 4;  // nSequence
    size += 36; // previousOut
    size += 1;  // scriptSigLength
    size += coinbase.inputs.scriptLength; //scriptSig
  }
  size += 1;   // nOutputs
  for (unsigned register int i = 0; i < coinbase.nOutputs; ++i) {
    /* Something really strange happens when I try add +8 from the value, it actually only works without then */
    size += 1; // spkLength
    size += coinbase.outputs[i].spkLength; //spk
  }
  size += 4;   // nlocktime

  return size * 2;  // each byte takes up 2 chars when hex-encoded
}
/** Some serialization methods
 * @todo: Maybe create a separate file for this
 */
int serByteArray(char *out, const unsigned char *bytes, int len) {
    for (unsigned register int i = 0; i < len; ++i) {
      sprintf(out + (2 * i), "%02x", (__builtin_bswap16(bytes[i]) >> 8));
    } 
    return len * 2;
}

int serInt(char *out, const unsigned int in) {
  sprintf(out, "%08x", __builtin_bswap32(in));
  return 4 * 2;
}
int serByte(char *out, const unsigned char in) {
  sprintf(out, "%02x", in);
  return 1 * 2;
} 

int serLong(char *out, const unsigned long in) {
  sprintf(out, "%08x%08x", ((unsigned  long) __builtin_bswap64(in) >> 32), ((unsigned  long) __builtin_bswap64(in)));
  return 8 * 2;
}

NOTNULL((1)) int serInput(char *serInput, struct input_t input) {
  unsigned int offset = 0;
  offset += serByteArray(serInput + offset, input.previousOutHash, 32);
  offset += serInt(serInput + offset, input.prevOutIndex);
  offset += serByte(serInput + offset, input.scriptLength);
  offset += serByteArray(serInput + offset, input.sigScript, input.scriptLength);
  offset += serInt(serInput + offset, input.nSequence);
  return offset;
}

NOTNULL((1)) int serOutput(char *serOutput, struct output_t output) {
  unsigned int offset = 0;
  offset += serLong(serOutput, output.value);
  offset += serByte(serOutput + offset, output.spkLength);
  offset += serByteArray(serOutput + offset, output.spk, output.spkLength);
  return offset;
}

NOTNULL((1)) int serializeCoinbase(char *acc, struct coinbase_t coinbase) {
  INIT_SER_METHOD(coinbase)

  SER_INT(coinbase.version)

  SER_BYTE(coinbase.nInputs)

  for (unsigned int i = 0; i < coinbase.nInputs; ++i) {
    offset += serInput(acc + offset, coinbase.inputs);
  }

  SER_BYTE(coinbase.nOutputs)
  
  for (unsigned int i = 0; i < coinbase.nOutputs; ++i) {
    offset += serOutput(acc + offset, coinbase.outputs[i]);
  }
  SER_INT(coinbase.locktime)

  return (size * 2);
}
//Creates an empty (invalid) coinbase transaction. DO NOT serialize it before filling up
struct coinbase_t createCoinbase() {
  const struct coinbase_t coinbase = {
    .version = 0,
    .nInputs = 0,
    .inputs = {
      .nSequence = 0,
      .previousOutHash = {},
      .prevOutIndex = 0,
      .scriptLength = 00,
      .sigScript = NULL
    },
    .nOutputs = 0,
    .outputs = NULL,
    .locktime = 0
  };

  return coinbase;
}
//Fill up a new coinbase with proper data
//segwit_default_commitment SHOULD be provided, if no spk, OP_TRUE is used
NOTNULL((1, 2)) void fillTransaction( struct coinbase_t *coinbase,
                      const unsigned char segwit_default_commit[76],
                      const unsigned int height,
                      const unsigned int value,
                      const unsigned char *spk,
                      const unsigned int spkLen
                    ) {
  coinbase->version = 1;
  coinbase->nInputs = 0x01;
  //Inputs
  
  coinbase->inputs.nSequence = 0xffffff;
  memset(coinbase->inputs.previousOutHash, 0x00, 32);
  coinbase->inputs.prevOutIndex = 0xffffffff;
  coinbase->inputs.scriptLength = 64;
  coinbase->inputs.sigScript = malloc(coinbase->inputs.scriptLength);
  memset(coinbase->inputs.sigScript, 0, coinbase->inputs.scriptLength);
  coinbase->inputs.sigScript[0] = height > 65535 ? 0x03 : 0x02;
  *(int *)(coinbase->inputs.sigScript + 1) = height;
  memcpy(coinbase->inputs.sigScript + 10, coinbase_data, strlen(coinbase_data));
  
  //Outputs -- Only 2 outputs, the actual payout and  the segwit default commitment
  coinbase->nOutputs = 0x02;
  if(spkLen > 0 && spk != NULL) {
    coinbase->outputs = malloc(sizeof (struct output_t) * coinbase->nOutputs);
    coinbase->outputs[0].value = value;
    coinbase->outputs[0].spkLength = spkLen/2;
    coinbase->outputs[0].spk = malloc(spkLen/2);
    assert(coinbase->outputs[0].spk != NULL);
    str2bytes(coinbase->outputs[0].spk, spk, spkLen);
  }
  else {
    coinbase->outputs = malloc(sizeof (struct output_t) * coinbase->nOutputs);
    coinbase->outputs[0].value = value;
    coinbase->outputs[0].spkLength = 1;
    coinbase->outputs[0].spk = malloc(1);
    assert(coinbase->outputs[0].spk != NULL);
    coinbase->outputs[0].spk[0] = 51;
  }
  coinbase->outputs[1].value = 0;
  coinbase->outputs[1].spkLength = 76 / 2;
  coinbase->outputs[1].spk = (unsigned char *) malloc(coinbase->outputs[1].spkLength);
  str2bytes(coinbase->outputs[1].spk, (unsigned char *) segwit_default_commit, 76);
}

NOTNULL((1)) void addNewOutput(struct coinbase_t *coinbase, struct output_t output) {
  unsigned int ser_size = 0;

  struct output_t outs[coinbase->nOutputs + 1];
  
  for (int i = 0; i < coinbase->nOutputs; ++i) {
    outs[i].value = coinbase->outputs[i].value;
    outs[i].spkLength = coinbase->outputs[i].spkLength;
    outs[i].spk = coinbase->outputs[i].spk;
  }
  outs[coinbase->nOutputs].value = output.value;
  outs[coinbase->nOutputs].spkLength = output.spkLength;
  outs[coinbase->nOutputs].spk = output.spk;
  
  coinbase->outputs = realloc(coinbase->outputs, sizeof(struct input_t) * coinbase->nInputs);
  
  coinbase->nOutputs += 1;
  for (int i = 0; i < coinbase->nOutputs; ++i) {
    coinbase->outputs[i].value = outs[i].value;
    coinbase->outputs[i].spkLength = outs[i].spkLength;
    coinbase->outputs[i].spk = outs[i].spk;
  }
}

NOTNULL((1)) void destroyTransaction(const struct coinbase_t *transaction) {
  assert(transaction != NULL && transaction->inputs.sigScript != NULL);

  free(transaction->inputs.sigScript);
  for (unsigned register int i = 0; i < transaction->nOutputs; ++i) {
    free(transaction->outputs[i].spk);
  }
  free(transaction->outputs);
}

NOTNULL((1)) void getTransactionId(unsigned char txId[32], const struct coinbase_t transaction) {
  sha_ctx ctx;
  sha_init(&ctx);
  const unsigned int size = getSerSize(transaction);
  unsigned char buffer[size], ser_tx[size];
  memset(ser_tx, 0, size);
  memset(buffer, 0, size);
  serializeCoinbase(buffer, transaction);
  str2bytes(ser_tx, buffer, size + 32);
  
  sha_update(&ctx, ser_tx, (size + 32)/2);
  sha_finalize(&ctx, txId);

  sha_init(&ctx);
  sha_update(&ctx, txId, 32);
  sha_finalize(&ctx, txId);
}
