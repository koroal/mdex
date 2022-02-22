#include <curl/curl.h>
#include "util.h"
#include "http.h"

#define REQS_PER_SECOND 5
#define RETRY_DELAY 1000
#define RETRY_COUNT 2

static CURL *curl;

static size_t callback(char *ptr, size_t size, size_t nmemb, void *data)
{
	buffer_t *buf = data;
	return buffer_write(buf, ptr, nmemb);
}

int http_init(void)
{
	if (curl_global_init(CURL_GLOBAL_ALL))
		goto error;
	if (!(curl = curl_easy_init()))
		goto cleanup_global;
	if (curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1) != CURLE_OK ||
	    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1) != CURLE_OK ||
	    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK ||
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback) != CURLE_OK)
		goto cleanup_easy;
	return OK;
cleanup_easy:
	curl_easy_cleanup(curl);
	curl = NULL;
cleanup_global:
	curl_global_cleanup();
error:
	puts("Failed to init libcurl");
	return ERROR;
}

void http_free(void)
{
	if (curl) {
		curl_easy_cleanup(curl);
		curl = NULL;
	}
	curl_global_cleanup();
}

int http_get(const char *url, const http_headers_t *headers, const char *payload, buffer_t *response)
{
	size_t start = response->n;
	unsigned retries = RETRY_COUNT;
	if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK ||
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response) != CURLE_OK ||
	    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
	    curl_easy_setopt(curl, payload ? CURLOPT_POST : CURLOPT_HTTPGET, 1) != CURLE_OK ||
	    (payload && curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload) != CURLE_OK))
		return ERROR;
	while (curl_easy_perform(curl) != CURLE_OK) {
		buffer_rewind(response, start);
		if (!retries)
			return ERROR;
		--retries;
		msleep(RETRY_DELAY);
	}
	msleep(1000 / REQS_PER_SECOND);
	return OK;
}

int http_headers_push(http_headers_t **headers, const char *string)
{
	struct curl_slist *tmp = curl_slist_append(*headers, string);
	if (!tmp)
		return ERROR;
	*headers = tmp;
	return OK;
}

void http_headers_free(http_headers_t **headers)
{
	curl_slist_free_all(*headers);
	*headers = NULL;
}
