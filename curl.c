#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "curl.h"

size_t writeCallback(void *data, size_t size, size_t nmemb, void *userp) {
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