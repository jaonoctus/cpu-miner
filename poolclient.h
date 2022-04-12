#ifndef POOL_CLIENT_H
#define POOL_CLIENT_H
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "CPUMiner.h"
#include "curl.h"
#include "poolclient.h"

void poolgetblock( struct memory *out, miner_options_t *opt );
int poolverifyCurrentBlock(struct memory *out, miner_options_t *opt);
void poolsubmitShare(struct memory *out, miner_options_t *opt, const char *data);
void poolsubmitShare(struct memory *out, miner_options_t *opt, const char *data);

#endif