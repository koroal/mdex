#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "util.h"

void msleep(long ms)
{
	struct timespec ts = {0};
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = ms % 1000 * 1000;
	while (nanosleep(&ts, &ts));
}

buffer_t buffer_make(size_t size)
{
	buffer_t buf = {0};
	buf.size = size;
	return buf;
}

void buffer_free(buffer_t *buf)
{
	free(buf->data);
	buf->data = NULL;
	buf->size = 0;
}

int buffer_shrink(buffer_t *buf)
{
	return TRY_REALLOC(&buf->data, &buf->size, buf->n + 1);
}

int buffer_reserve(buffer_t *buf, size_t size)
{
	if (size > ((size_t)-1) - 1)
		return ERROR;
	if (buf->size < size + 1)
		return TRY_REALLOC(&buf->data, &buf->size, size + 1);
	return OK;
}

void buffer_rewind(buffer_t *buf, size_t nmemb)
{
	if (buf->n <= nmemb)
		return;
	buf->n = nmemb;
	buf->data[buf->n] = '\0';
}

size_t buffer_write(buffer_t *buf, const char *ptr, size_t nmemb)
{
	if (nmemb > ((size_t)-1) - 1) {
		return 0;
	} else if (!buf->data) {
		size_t new_size = buf->size;
		if (new_size < nmemb + 1)
			new_size = nmemb + 1;
		if (TRY_REALLOC(&buf->data, &buf->size, new_size))
			return 0;
		buf->n = 0;
	} else if (buf->size - buf->n < nmemb + 1) {
		size_t new_size = buf->size;
		do {
			if (new_size > ((size_t)-1) / 2)
				return 0;
			new_size *= 2;
		} while (new_size - buf->n < nmemb + 1);
		if (TRY_REALLOC(&buf->data, &buf->size, new_size))
			nmemb = buf->size - buf->n;
	}
	memcpy(buf->data + buf->n, ptr, nmemb);
	buf->n += nmemb;
	buf->data[buf->n] = '\0';
	return nmemb;
}

int buffer_strcpy(buffer_t *buf, const char *ptr, size_t size)
{
	return size == buffer_write(buf, ptr, size) ? OK : ERROR;
}

int buffer_append(buffer_t *buf, const char *ptr)
{
	size_t size = strlen(ptr);
	return size == buffer_write(buf, ptr, size) ? OK : ERROR;
}

int buffer_append_double(buffer_t *buf, double num, int iwidth, int fwidth)
{
	int digits;
	num = fabs(num);
	iwidth = abs(iwidth);
	fwidth = abs(fwidth);
	digits = num < 10.0 ? 1 : 1 + (int)(floor(log10(num)));
	if (iwidth < digits)
		iwidth = digits;
	if (buffer_reserve(buf, buf->n + (size_t)(iwidth + fwidth + 1)))
		return ERROR;
	if (iwidth > digits)
		buf->n += (size_t)sprintf(buf->data + buf->n, "%0*u", iwidth - digits, 0);
	buf->n += (size_t)sprintf(buf->data + buf->n, "%.*g", digits + fwidth, num);
	return OK;
}

int buffer_append_ulong(buffer_t *buf, unsigned long num, int iwidth)
{
	if (buffer_reserve(buf, buf->n + (size_t)(iwidth > 20 ? iwidth : 20)))
		return ERROR;
	buf->n += (size_t)sprintf(buf->data + buf->n, "%0*lu", iwidth, num);
	return OK;
}

int try_realloc(void *pptr, size_t *out_n, size_t new_n, size_t size)
{
	void **ptr = pptr, *new_ptr;
	if (!(new_ptr = realloc(*ptr, new_n * size)))
		return ERROR;
	*ptr = new_ptr;
	*out_n = new_n;
	return OK;
}
