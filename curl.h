#ifndef CURL_H
#define CURL_H
struct memory {
  char *response;
  size_t size;
};
size_t writeCallback(void *data, size_t size, size_t nmemb, void *userp);

#define CURL_PERFORM(url)   CURL *curl;\
  CURLcode res;\
\
  curl = curl_easy_init();\
  out->size = 0;\
  out->response = NULL;\
\
  if(curl) {\
    /*curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);*/\
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));\
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);\
\
    curl_easy_setopt(curl, CURLOPT_URL, url);\
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);\
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);\
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, opt->headers);\
    res = curl_easy_perform(curl);\
    curl_easy_cleanup(curl);\
  }

#endif