#ifndef TRANSACTION_H
#define TRANSACTION_H
struct input_t {
  unsigned char previousOutHash[32];
  unsigned int prevOutIndex;
  unsigned char scriptLength;
  unsigned char *sigScript;
  unsigned int nSequence;
};

struct output_t {
  unsigned long value;
  unsigned char spkLength;
  unsigned char *spk; 
};


struct coinbase_t
{
  unsigned int version;
  unsigned char nInputs;
  struct input_t inputs;
  unsigned int nOutputs;
  struct output_t *outputs;
  unsigned int locktime;
};
int serializeCoinbase(char *acc, struct coinbase_t coinbase);
struct coinbase_t createCoinbase();
void fillTransaction( struct coinbase_t *coinbase,
                      const unsigned char segwit_default_commit[76],
                      const unsigned int height,
                      const unsigned int value,
                      const unsigned char *spk,
                      const unsigned int spkLen,
                      const unsigned char *coinbase_data
                    );
void addNewOutput(struct coinbase_t *coinbase, struct output_t output);
void destroyTransaction(const struct coinbase_t *transaction);
void getTransactionId(unsigned char txId[32], const struct coinbase_t transaction);
int getSerSize(struct coinbase_t coinbase);

#endif