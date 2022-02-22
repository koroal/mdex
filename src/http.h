#ifndef HTTP_H
#define HTTP_H

#include "util.h"

typedef struct http_headers {
	struct curl_slist *list;
} http_headers_t;

http_headers_t http_headers_make(void);
int http_headers_push(http_headers_t *headers, const char *string);
void http_headers_free(http_headers_t *headers);

int http_init(void);
void http_free(void);
int http_get(const char *url, const http_headers_t *headers, const char *payload, buffer_t *response);

#endif
