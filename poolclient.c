#include "poolclient.h"

void poolgetblock( struct memory *out, miner_options_t *opt ) {
	const char *data = "getblocktemplate";
	CURL_PERFORM(opt->pool);
}


int poolverifyCurrentBlock(struct memory *out, miner_options_t *opt) {
	const char *data = "getbestblock";
	CURL_PERFORM(opt->pool);
}

void poolsubmitShare(struct memory *out, miner_options_t *opt, const char *data) {
	CURL_PERFORM(opt->pool);
}
