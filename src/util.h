#ifndef UTIL_H
#define UTIL_H

#include "defs.h"

void msleep(long ms);
int try_realloc(void *pptr, size_t *out_n, size_t new_n, size_t size);
#define TRY_REALLOC(B, S, N) try_realloc((B), (S), (N), sizeof(**(B)))

typedef struct buffer {
	size_t size, n;
	char *data;
} buffer_t;

buffer_t buffer_make(size_t size);
void buffer_free(buffer_t *buf);
int buffer_shrink(buffer_t *buf);
int buffer_reserve(buffer_t *buf, size_t size);
void buffer_rewind(buffer_t *buf, size_t nmemb);
size_t buffer_write(buffer_t *buf, const char *ptr, size_t nmemb);
int buffer_strcpy(buffer_t *buf, const char *ptr, size_t size);
int buffer_append(buffer_t *buf, const char *ptr);
int buffer_append_double(buffer_t *buf, double num, int iwidth, int fwidth);
int buffer_append_ulong(buffer_t *buf, unsigned long num, int iwidth);

#endif
