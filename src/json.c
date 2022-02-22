#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#define JSMN_PARENT_LINKS
#define JSMN_STRICT
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "jsmn.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "util.h"
#include "json.h"

static int parse_code(const char *ptr, unsigned *code)
{
	const char *end = ptr + 4;
	unsigned num = 0;
	char ch;
	do {
		num <<= 4;
		ch = *ptr;
		if (ch >= '0' && ch <= '9') num |= (unsigned)(ch - '0');
		else if (ch >= 'A' && ch <= 'F') num |= (unsigned)(ch - 'A' + 0xa);
		else if (ch >= 'a' && ch <= 'f') num |= (unsigned)(ch - 'a' + 0xa);
		else return ERROR;
	} while (++ptr < end);
	*code = num;
	return OK;
}

static size_t utf8_encode(unsigned code, char *buf)
{
	unsigned char *out = (unsigned char *)buf;
	if (code <= 0x007f) {
		*out++ = (unsigned char)code;
		return 1;
	} else if (code <= 0x07ff) {
		*out++ = (unsigned char)(((code >> 6) & 0x1f) | 0xc0);
		*out++ = (unsigned char)(((code >> 0) & 0x3f) | 0x80);
		return 2;
	} else if (code <= 0xffff) {
		*out++ = (unsigned char)(((code >> 12) & 0x0f) | 0xe0);
		*out++ = (unsigned char)(((code >> 6) & 0x3f) | 0x80);
		*out++ = (unsigned char)(((code >> 0) & 0x3f) | 0x80);
		return 3;
	}
	return 0;
}

char *json_unescape(const char *ptr, size_t size)
{
	char ch;
	size_t n;
	char seq[3];
	unsigned code;
	const char *end;
	buffer_t buf;
	if (!size)
		return strdup("");
	end = ptr + size;
	buf = buffer_make(size);
	while (ptr < end) {
		ch = *ptr++;
		if (ch == '\\' && ptr < end) {
			ch = *ptr++;
			switch (ch) {
			case 'b': ch = '\b'; goto putchar;
			case 'f': ch = '\f'; goto putchar;
			case 'n': ch = '\n'; goto putchar;
			case 'r': ch = '\r'; goto putchar;
			case 't': ch = '\t'; goto putchar;
			case 'u':
			          if (end - ptr < 4 || parse_code(ptr, &code))
				          goto cleanup;
			          n = utf8_encode(code, seq);
			          if (buffer_strcpy(&buf, seq, n))
				          goto cleanup;
			          ptr += 4;
			          break;
			default:
			          goto putchar;
			}
		} else putchar: {
			if (buffer_strcpy(&buf, &ch, 1))
				goto cleanup;
		}
	}
	buffer_shrink(&buf);
	return buf.data;
cleanup:
	buffer_free(&buf);
	return NULL;
}

json_t *json_parse(const char *data, size_t size)
{
	int n;
	jsmntok_t *tokens;
	jsmn_parser parser;
	if (!size)
		return NULL;
	jsmn_init(&parser);
	if (((n = jsmn_parse(&parser, data, size, NULL, 0)) < 1) ||
	    (!(tokens = malloc((unsigned)n * sizeof(*tokens)))))
		return NULL;
	jsmn_init(&parser);
	if (jsmn_parse(&parser, data, size, tokens, (unsigned)n) < 1)
		goto cleanup;
	return tokens;
cleanup:
	free(tokens);
	return NULL;
}

int json_eql(const char *data, const json_t *token, const char *str, size_t strlen)
{
	size_t len = (size_t)(token->end - token->start);
	if (token->type != JSON_STRING || len != strlen)
		return 0;
	return !strncmp(data + token->start, str, len);
}

int json_eq(const char *data, const json_t *token, const char *str)
{
	return json_eql(data, token, str, strlen(str));
}

const json_t *json_find(const char *data, const json_t *token, const char *name)
{
	const char *next;
	for (next = name; *name; name = next) {
		while (*next && *next != '.')
			++next;
		if (token->type == JSON_ARRAY && isdigit(*name)) {
			long i = atol(name);
			if (i >= token->size)
				return NULL;
			for (++token; i--; i += token++->size);
			goto found;
		} else if (token->type == JSON_OBJECT) {
			long i = token++->size, j;
			while (i--) {
				if (!json_eql(data, token, name, (size_t)(next - name))) {
					for (j = token->size; j--; j += token++->size);
				} else {
					++token;
					goto found;
				}
			}
		}
		return NULL;
found:
		if (!*next)
			return token;
		++next;
	}
	return NULL;
}

const json_t *json_find_object(const char *data, const json_t *token, const char *name)
{
	token = json_find(data, token, name);
	if (!token || token->type != JSON_OBJECT)
		return NULL;
	return token;
}

const json_t *json_find_array(const char *data, const json_t *token, const char *name)
{
	token = json_find(data, token, name);
	if (!token || token->type != JSON_ARRAY)
		return NULL;
	return token;
}

const json_t *json_find_string(const char *data, const json_t *token, const char *name)
{
	token = json_find(data, token, name);
	if (!token || token->type != JSON_STRING)
		return NULL;
	return token;
}

const json_t *json_find_number(const char *data, const json_t *token, const char *name)
{
	token = json_find(data, token, name);
	if (!token || token->type != JSON_PRIMITIVE)
		return NULL;
	if (data[token->start] != '-' && !isdigit(data[token->start]))
		return NULL;
	return token;
}

const json_t *json_find_boolean(const char *data, const json_t *token, const char *name)
{
	token = json_find(data, token, name);
	if (!token || token->type != JSON_PRIMITIVE)
		return NULL;
	if (data[token->start] != 't' && data[token->start] != 'f')
		return NULL;
	return token;
}

char *json_strcpy(const char *data, const json_t *token, char *dest, size_t size)
{
	size_t tsize;
	if (!size)
		return NULL;
	tsize = json_size(token);
	size = tsize < size ? tsize : size - 1;
	dest[0] = '\0';
	return strncat(dest, data + token->start, size);
}

char *json_strdup(const char *data, const json_t *token)
{
	if (token->type == JSON_STRING)
		return json_unescape(data + token->start, json_size(token));
	return NULL;
}

double json_double(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return atof(data + token->start);
	return 0.0;
}

unsigned long json_ulong(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return strtoul(data + token->start, NULL, 10);
	return 0;
}

unsigned json_uint(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return (unsigned)strtoul(data + token->start, NULL, 10);
	return 0;
}

long json_long(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return atol(data + token->start);
	return 0;
}

int json_int(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return atoi(data + token->start);
	return 0;
}

int json_bool(const char *data, const json_t *token)
{
	if (token->type & (JSON_PRIMITIVE | JSON_STRING))
		return data[token->start] == 't';
	return 0;
}
