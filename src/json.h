#ifndef JSON_H
#define JSON_H
#define JSMN_PARENT_LINKS
#define JSMN_STRICT
#define JSMN_HEADER
#include "jsmn.h"

#define JSON_UNDEFINED JSMN_UNDEFINED
#define JSON_OBJECT JSMN_OBJECT
#define JSON_ARRAY JSMN_ARRAY
#define JSON_STRING JSMN_STRING
#define JSON_PRIMITIVE JSMN_PRIMITIVE

typedef jsmntok_t json_t;

typedef struct json_iter {
	int count;
	const json_t *token;
} json_iter_t;

static json_iter_t json_iter(const json_t *token)
{
	json_iter_t it = {0};
	it.count = token->size;
	it.token = ++token;
	return it;
}

static int json_next(const json_t **elem, json_iter_t *it)
{
	int i = 0;
	const json_t *token = it->token;
	if (!it->count)
		return 0;
	do i += it->token++->size; while (i--);
	--it->count;
	*elem = token;
	return 1;
}

static const json_t *json_value(const json_t *token)
{
	if (token->size && token->type == JSON_STRING)
		++token;
	return token;
}

static size_t json_size(const json_t *token)
{
	return (size_t)(token->end - token->start);
}

static size_t json_count(const json_t *token)
{
	return (size_t)token->size;
}

json_t *json_parse(const char *data, size_t size);
int json_eql(const char *data, const json_t *token, const char *str, size_t strlen);
int json_eq(const char *data, const json_t *token, const char *str);
const json_t *json_find(const char *data, const json_t *token, const char *name);
const json_t *json_find_object(const char *data, const json_t *token, const char *name);
const json_t *json_find_array(const char *data, const json_t *token, const char *name);
const json_t *json_find_string(const char *data, const json_t *token, const char *name);
const json_t *json_find_number(const char *data, const json_t *token, const char *name);
const json_t *json_find_boolean(const char *data, const json_t *token, const char *name);
char *json_strcpy(const char *data, const json_t *token, char *dest, size_t size);
char *json_strdup(const char *data, const json_t *token);
double json_double(const char *data, const json_t *token);
unsigned long json_ulong(const char *data, const json_t *token);
unsigned json_uint(const char *data, const json_t *token);
long json_long(const char *data, const json_t *token);
int json_int(const char *data, const json_t *token);
int json_bool(const char *data, const json_t *token);
char *json_unescape(const char *ptr, size_t size);

#endif
