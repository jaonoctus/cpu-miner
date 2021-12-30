#ifndef UTILS_RPC_H
#define UTILS_RPC_H

#define NOTNULL(x) __nonnull (x)
#include <stdlib.h>
#include <string.h>

struct memory {
  char *response;
  size_t size;
};

//Some useful constants that will be used during rpc stuff
static const char *getBlockchaininfo   =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblockchaininfo\",\"params\":[]}";
static const char *getBlockTemplateCmd =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblocktemplate\",\"params\":[{\"rules\":[\"segwit\"]}]}";
static const char *getBlock            =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"getblock\",\"params\":[\"%s\"]}";
static const char *submitHeader        =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"submitheader\",\"params\":[\"%s\"]}";
static const char *submitBlockBase     =  (char *) "{\"jsonrpc\":\"1.0\",\"id\":\"curltext\",\"method\":\"submitblock\",\"params\":[\"%s\"]}";

NOTNULL((1, 2)) extern void callRPC(struct memory *out,
                    const char *data,
                    struct curl_slist *headers,
                    const char cookie[76]);
NOTNULL((1, 4)) static size_t writeCallback(void *data, size_t size, size_t nmemb, void *userp);

#endif