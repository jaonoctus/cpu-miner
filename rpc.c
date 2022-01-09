#include <curl/curl.h>

#include "rpc.h"
#include "CPUMiner.h"

NOTNULL((1, 4)) static size_t writeCallback(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;
  char *ptr;
  
  if(mem->size == 0) {
    ptr = malloc(mem->size + realsize + 1);
  } else {
    ptr = realloc(mem->response, mem->size + realsize + 1);
  }
  if(ptr == NULL) {
    printf("Error!\n");
    return 0;  /* out of memory! */
  }
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}
NOTNULL((1, 2)) void callRPC(
                              struct memory *out,
                              const char *data,
                              miner_options_t *opt
                            ) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  out->size = 0;
  out->response = NULL;

  if(curl) {
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    char url[1000] = {0};
    sprintf(url, opt->url, opt->cookie, opt->port);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, opt->headers);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
} 