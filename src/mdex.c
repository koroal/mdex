#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <unistd.h>
#include <sys/stat.h>
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include "util.h"
#include "http.h"
#include "json.h"
#include "mdex.h"

#define URL "https://api.mangadex.org"
#define CHAPTERS_REQ_LIMIT 500
#define NO_GROUP_NAME "No Group"
#define NO_GROUP_ID 0

#define zipOpenNewFileInZip(Z, N) \
		zipOpenNewFileInZip((Z), (N), NULL, NULL, 0, NULL, 0, NULL, 0, 0)

typedef struct range {
	double from, to;
} range_t;

#define NAME ranges
#define ELEM range_t
#include "vect.h"

typedef struct group {
	char uuid[40];
	char *name;
	unsigned priority;
} group_t;

static void group_free(group_t *group)
{
	free(group->name);
}

#define NAME groups
#define ELEM group_t
#define FREE group_free
#include "vect.h"

#define NAME group_ids
#define ELEM size_t
#define ELEM_VALUE
#include "vect.h"

typedef struct chapter {
	char uuid[40];
	char *title;
	double number;
	unsigned pages;
	unsigned volume;
	unsigned version;
	unsigned priority;
	group_ids_t group_ids;
	int skip;
} chapter_t;

static void chapter_delete(chapter_t *chapter)
{
	group_ids_free(&chapter->group_ids);
	free(chapter->title);
	free(chapter);
}

static chapter_t *chapter_create(void)
{
	chapter_t *chapter = calloc(1, sizeof(*chapter));
	if (!chapter)
		return NULL;
	chapter->group_ids = group_ids_make(0);
	return chapter;
}

#define NAME chapters
#define ELEM chapter_t *
#define ELEM_VALUE
#define FREE chapter_delete
#include "vect.h"

typedef struct mdex {
	char uuid[40];
	char lang[8];
	char *title;
	unsigned flags;
	const char **prefs;
	groups_t groups;
	ranges_t ranges;
	chapters_t chapters;
} mdex_t;

static void mdex_delete(mdex_t *mdex)
{
	free(mdex->title);
	groups_free(&mdex->groups);
	ranges_free(&mdex->ranges);
	chapters_free(&mdex->chapters);
	free(mdex);
}

static size_t regmatch_size(const regmatch_t *match)
{
	return (size_t)(match->rm_eo - match->rm_so);
}

static int parse_uuid(mdex_t *mdex, const char *series)
{
	static const char pattern[] =
		"[[:xdigit:]]{8}-[[:xdigit:]]{4}-"
		"[[:xdigit:]]{4}-[[:xdigit:]]{4}-"
		"[[:xdigit:]]{12}";
	int result = ERROR;
	size_t size;
	regex_t regex;
	regmatch_t match;
	if (regcomp(&regex, pattern, REG_EXTENDED))
		goto finish;
	if (regexec(&regex, series, 1, &match, 0))
		goto cleanup_regex;
	size = regmatch_size(&match);
	if (size > SIZEOF(mdex->uuid) - 1)
		size = SIZEOF(mdex->uuid) - 1;
	strncat(mdex->uuid, series + match.rm_so, size);
	result = OK;
cleanup_regex:
	regfree(&regex);
finish:
	return result;
}

static int parse_ranges(mdex_t *mdex, const char *ranges)
{
	static const char pattern[] =
		"^[[:space:]]*"
		"([[:digit:]]+\\.?[[:digit:]]*|[[:digit:]]*\\.?[[:digit:]]+)?"
		"([[:space:]]*-[[:space:]]*)?"
		"([[:digit:]]+\\.?[[:digit:]]*|[[:digit:]]*\\.?[[:digit:]]+)?"
		"[^,]*,?";
	int result = ERROR;
	regex_t regex;
	regmatch_t match[4];
	if (!ranges) {
		range_t range = {0.0, HUGE_VAL};
		return ranges_push(&mdex->ranges, &range);
	}
	if (regcomp(&regex, pattern, REG_EXTENDED))
		goto finish;
	while (!regexec(&regex, ranges, SIZEOF(match), match, 0)) {
		int dash = !!regmatch_size(&match[2]),
			lhs = !!regmatch_size(&match[1]),
			rhs = !!regmatch_size(&match[3]);
		const char
			*lhs_ptr = ranges + match[1].rm_so,
			*rhs_ptr = ranges + match[3].rm_so;
		range_t range;
		if (!regmatch_size(&match[0]))
			break;
		if (dash) {
			range.from = lhs ? atof(lhs_ptr) : 0.0;
			range.to = rhs ? atof(rhs_ptr) : HUGE_VAL;
		} else {
			range.from = lhs ? atof(lhs_ptr) : HUGE_VAL;
			range.to = lhs ? range.from : 0.0;
		}
		if (range.to >= range.from && ranges_push(&mdex->ranges, &range))
			goto cleanup_regex;
		ranges += match->rm_eo;
	}
	if (!mdex->ranges.n)
		goto cleanup_regex;
	result = OK;
cleanup_regex:
	regfree(&regex);
finish:
	return result;
}

static unsigned get_group_priority(mdex_t *mdex, const char *name)
{
	const char **groups;
	unsigned priority = UINT_MAX;
	for (groups = mdex->prefs; *groups; ++groups, --priority)
		if (!strcmp(*groups, name))
			return priority;
	return 0;
}

static mdex_t *mdex_create(const mdex_args_t *args)
{
	static const char *null;
	group_t no_group = {0};
	const char *lang = args->lang ? args->lang : "en";
	mdex_t *mdex = calloc(1, sizeof(*mdex));
	if (!mdex)
		goto error;
	mdex->groups = groups_make(0);
	mdex->ranges = ranges_make(0);
	mdex->chapters = chapters_make(0);
	if (parse_uuid(mdex, args->series)) {
		puts("Failed to parse uuid");
		goto cleanup_mdex;
	} else if (parse_ranges(mdex, args->ranges)) {
		puts("Failed to parse chapter ranges");
		goto cleanup_mdex;
	}
	mdex->flags = args->flags;
	mdex->prefs = args->groups ? args->groups : &null;
	strncat(mdex->lang, lang, SIZEOF(mdex->lang) - 1);
	if (groups_reserve(&mdex->groups, 1) ||
		!(no_group.name = strdup(NO_GROUP_NAME))) {
		puts("Out of memory");
		goto cleanup_mdex;
	}
	no_group.priority = get_group_priority(mdex, no_group.name);
	groups_push(&mdex->groups, &no_group);
	return mdex;
cleanup_mdex:
	mdex_delete(mdex);
error:
	return NULL;
}

static int get_title(mdex_t *mdex)
{
	int result = ERROR;
	const json_t *token;
	json_t *json = NULL;
	buffer_t req = buffer_make(0);
	buffer_t resp = buffer_make(0);
	if (buffer_append(&req, URL) ||
		buffer_append(&req, "/manga/") ||
		buffer_append(&req, mdex->uuid) ||
		http_get(req.data, NULL, NULL, &resp) ||
		!(json = json_parse(resp.data, resp.n)) ||
		!(token = json_find(resp.data, json, "data.attributes.title.en")) ||
		!(mdex->title = json_strdup(resp.data, token)))
		goto finish;
	result = OK;
finish:
	free(json);
	buffer_free(&resp);
	buffer_free(&req);
	return result;
}

static int fetch_group_id(mdex_t *mdex, const char *data, const json_t *uuid, size_t *group_id)
{
	int result = ERROR;
	group_t group = {0};
	buffer_t req = buffer_make(0);
	buffer_t resp = buffer_make(0);
	json_t *json = NULL;
	const json_t *name;
	if (buffer_append(&req, URL) ||
		buffer_append(&req, "/group/") ||
		buffer_strcpy(&req, data + uuid->start, json_size(uuid)) ||
		http_get(req.data, NULL, NULL, &resp) ||
		!(json = json_parse(resp.data, resp.n)) ||
		!(name = json_find(resp.data, json, "data.attributes.name")) ||
		!(group.name = json_strdup(resp.data, name)))
		goto finish;
	group.priority = get_group_priority(mdex, group.name);
	json_strcpy(data, uuid, group.uuid, SIZEOF(group.uuid));
	if (groups_push(&mdex->groups, &group))
		goto finish;
	*group_id = mdex->groups.n - 1;
	result = OK;
finish:
	free(json);
	buffer_free(&resp);
	buffer_free(&req);
	return result;
}

static int get_group_id(mdex_t *mdex, const char *data, const json_t *uuid, size_t *group_id)
{
	groups_t *groups = &mdex->groups;
	size_t i, n;
	for (i = 0, n = groups->n; i < n; ++i) {
		if (json_eq(data, uuid, groups->data[i].uuid)) {
			*group_id = i;
			return OK;
		}
	}
	return fetch_group_id(mdex, data, uuid, group_id);
}

static int parse_chapter(mdex_t *mdex, const char *data, const json_t *json)
{
	size_t group_id;
	group_ids_iter_t group_ids;
	const json_t *field, *relationship;
	json_iter_t relationships;
	chapter_t *chapter;
	if (json_find_string(data, json, "attributes.externalUrl"))
		return OK;
	if (!(chapter = chapter_create()))
		return ERROR;
	if (!(field = json_find(data, json, "id")))
		goto error;
	json_strcpy(data, field, chapter->uuid, SIZEOF(chapter->uuid) - 1);
	if (!(field = json_find(data, json, "attributes.chapter")))
		goto error;
	chapter->number = json_double(data, field);
	if (!(field = json_find(data, json, "attributes.version")))
		goto error;
	chapter->version = json_uint(data, field);
	if (!(field = json_find(data, json, "attributes.volume")))
		goto error;
	chapter->volume = json_uint(data, field);
	if (!(field = json_find(data, json, "attributes.pages")))
		goto error;
	chapter->pages = json_uint(data, field);
	if (mdex->flags & MDEX_CHAPTITLE)
		if (!(field = json_find(data, json, "attributes.title")) ||
			!(chapter->title = json_strdup(data, field)))
			goto error;
	if (!(field = json_find(data, json, "relationships")))
		goto error;
	relationships = json_iter(field);
	while (json_next(&relationship, &relationships))
		if (!(field = json_find(data, relationship, "type")) ||
			!json_eq(data, field, "scanlation_group") ||
			!(field = json_find(data, relationship, "id")) ||
			get_group_id(mdex, data, field, &group_id) ||
			group_ids_push(&chapter->group_ids, group_id))
			continue;
	if (!chapter->group_ids.n)
		if (group_ids_push(&chapter->group_ids, NO_GROUP_ID))
			goto error;
	group_ids = group_ids_iter(&chapter->group_ids);
	while (group_ids_next(&group_id, &group_ids)) {
		group_t *group = &mdex->groups.data[group_id];
		if (chapter->priority < group->priority)
			chapter->priority = group->priority;
	}
	if (chapters_push(&mdex->chapters, chapter))
		goto error;
	return OK;
error:
	chapter_delete(chapter);
	return ERROR;
}

static int parse_chapters(mdex_t *mdex, const char *data, const json_t *json)
{
	const json_t *chapter;
	json_iter_t chapters = json_iter(json);
	while (json_next(&chapter, &chapters))
		if (parse_chapter(mdex, data, chapter))
			return ERROR;
	return OK;
}

static int get_chapters(mdex_t *mdex)
{
	static const char req_params[] =
		"/feed?order[volume]=asc&order[chapter]=asc&limit=" STRING(CHAPTERS_REQ_LIMIT)
		"&contentRating[]=safe&contentRating[]=suggestive"
		"&contentRating[]=erotica&contentRating[]=pornographic"
		"&translatedLanguage[]=";
	int result = ERROR;
	json_t *json = NULL;
	const json_t *field;
	size_t req_buffer_state;
	size_t offset = 0, total = 0;
	buffer_t req = buffer_make(0);
	buffer_t resp = buffer_make(0);
	if (buffer_append(&req, URL) ||
		buffer_append(&req, "/manga/") ||
		buffer_append(&req, mdex->uuid) ||
		buffer_append(&req, req_params) ||
		buffer_append(&req, mdex->lang) ||
		buffer_append(&req, "&offset="))
		goto finish;
	req_buffer_state = req.n;
	do {
		if (buffer_append_ulong(&req, offset, 0) ||
			http_get(req.data, NULL, NULL, &resp) ||
			!(json = json_parse(resp.data, resp.n)))
			goto finish;
		if (!total)
			if (!(field = json_find_number(resp.data, json, "total")) ||
				!(total = json_uint(resp.data, field)) ||
				chapters_reserve(&mdex->chapters, total))
				goto finish;
		if (!(field = json_find(resp.data, json, "data")) ||
			parse_chapters(mdex, resp.data, field))
			goto finish;
		offset += CHAPTERS_REQ_LIMIT;
		buffer_rewind(&req, req_buffer_state);
		buffer_rewind(&resp, 0);
		free(json);
		json = NULL;
	} while (offset < total);
	result = OK;
finish:
	free(json);
	buffer_free(&resp);
	buffer_free(&req);
	return result;
}

static int order_chapters(const void *chapter1, const void *chapter2)
{
	const chapter_t *c1 = *(const chapter_t **)chapter1;
	const chapter_t *c2 = *(const chapter_t **)chapter2;
	if (c1->number < c2->number)
		return -1;
	if (c1->number > c2->number)
		return +1;
	if (c1->priority > c2->priority)
		return -1;
	if (c1->priority < c2->priority)
		return +1;
	if (c1->version > c2->version)
		return -1;
	if (c1->version < c2->version)
		return +1;
	return 0;
}

static int chapter_in_range(const chapter_t *chapter, const ranges_t *ranges)
{
	range_t *range;
	ranges_iter_t it = ranges_iter(ranges);
	while (ranges_next(&range, &it))
		if (chapter->number >= range->from &&
			chapter->number <= range->to)
			return 1;
	return 0;
}

static int filter_chapters(mdex_t *mdex)
{
	int result = OK, reporting = 0;
	int report = mdex->flags & MDEX_REPORTDUP;
	groups_t *groups = &mdex->groups;
	ranges_t *ranges = &mdex->ranges;
	chapters_t *chapters = &mdex->chapters;
	chapter_t *chapter, *last = NULL;
	chapters_iter_t it;
	qsort(chapters->data, chapters->n, sizeof(*chapters->data), order_chapters);
	it = chapters_iter(chapters);
	while (chapters_next(&chapter, &it)) {
		if (!chapter_in_range(chapter, ranges)) {
			chapter->skip = 1;
			continue;
		}
		if (last && last->number == chapter->number) {
			chapter->skip = 1;
			if (report && !last->priority) {
				size_t group_id, delimiter;
				group_ids_iter_t group_ids;
				if (!last->skip) {
					printf("%s", mdex->title);
					if (chapter->volume)
						printf(" v%02u", chapter->volume);
					printf(" c%03.5g: [", chapter->number);
					delimiter = 0;
					group_ids = group_ids_iter(&last->group_ids);
					while (group_ids_next(&group_id, &group_ids)) {
						if (!delimiter) delimiter = 1; else printf(" & ");
						printf("%s", groups->data[group_id].name);
					}
					printf("]");
					reporting = 1;
					result = ERROR;
				}
				if (reporting) {
					printf(", [");
					delimiter = 0;
					group_ids = group_ids_iter(&chapter->group_ids);
					while (group_ids_next(&group_id, &group_ids)) {
						if (!delimiter) delimiter = 1; else printf(" & ");
						printf("%s", groups->data[group_id].name);
					}
					printf("]");
				}
			}
		} else if (reporting) {
			printf("\n");
			reporting = 0;
		}
		last = chapter;
	}
	return result;
}

static int get_file_name(buffer_t *buf, const mdex_t *mdex, const chapter_t *chapter)
{
	if (mdex->flags & MDEX_USESUBDIR)
		if (buffer_append(buf, mdex->title) ||
			buffer_append(buf, "/"))
			return ERROR;
	if (buffer_append(buf, mdex->title))
		return ERROR;
	if (chapter->volume > 0)
		if (buffer_append(buf, " v") ||
			buffer_append_ulong(buf, chapter->volume, 2))
			return ERROR;
	if (buffer_append(buf, " c") ||
		buffer_append_double(buf, chapter->number, 3, 5))
		return ERROR;
	if (chapter->title && *chapter->title)
		if (buffer_append(buf, " (") ||
			buffer_append(buf, chapter->title) ||
			buffer_append(buf, ")"))
			return ERROR;
	if (chapter->group_ids.n > 0) {
		size_t group_id, delimiter = 0;
		group_ids_iter_t group_ids;
		if (buffer_append(buf, " ["))
			return ERROR;
		group_ids = group_ids_iter(&chapter->group_ids);
		while (group_ids_next(&group_id, &group_ids)) {
			if (!delimiter)
				delimiter = 1;
			else if (buffer_append(buf, " & "))
				return ERROR;
			if (buffer_append(buf, mdex->groups.data[group_id].name))
				return ERROR;
		}
		if (buffer_append(buf, "]"))
			return ERROR;
	}
	if (buffer_append(buf, ".cbz"))
		return ERROR;
	return OK;
}

static int get_pages_in_file(const char *archive, size_t *pages)
{
	int result = ERROR;
	unzFile unzip;
	unz_global_info info;
	if (!(unzip = unzOpen(archive)))
		goto finish;
	if (unzGetGlobalInfo(unzip, &info))
		goto cleanup_unzip;
	*pages = info.number_entry;
	result = OK;
cleanup_unzip:
	unzClose(unzip);
finish:
	return result;
}

static int save_page(const char *archive, const char *name, const char *data, size_t size)
{
	int result = ERROR;
	zipFile zip;
	if (!(zip = zipOpen(archive, APPEND_STATUS_ADDINZIP)))
		goto finish;
	if (zipOpenNewFileInZip(zip, name))
		goto cleanup_zip;
	if (zipWriteInFileInZip(zip, data, (unsigned)size))
		goto cleanup_file;
	result = OK;
cleanup_file:
		zipCloseFileInZip(zip);
cleanup_zip:
		zipClose(zip, NULL);
finish:
	return result;
}

static int save_pages(const char *archive, const chapter_t *chapter, size_t pages)
{
	int result = ERROR;
	char *base_url = NULL;
	const json_t *base, *hash, *data, *file;
	size_t page = 0, req_buffer_state, name_buffer_state;
	json_t *json = NULL;
	json_iter_t files;
	buffer_t buf = buffer_make(0);
	buffer_t req = buffer_make(0);
	buffer_t resp = buffer_make(0);
	buffer_t name = buffer_make(0);
	printf("\33[2K\r%s: %lu/%u", archive, pages, chapter->pages);
	fflush(stdout);
	if (buffer_append(&req, URL) ||
		buffer_append(&req, "/at-home/server/") ||
		buffer_append(&req, chapter->uuid) ||
		http_get(req.data, NULL, NULL, &resp) ||
		!(json = json_parse(resp.data, resp.n)) ||
		!(base = json_find_string(resp.data, json, "baseUrl")) ||
		!(hash = json_find_string(resp.data, json, "chapter.hash")) ||
		!(data = json_find_array(resp.data, json, "chapter.data")) ||
		!(base_url = json_strdup(resp.data, base)))
		goto finish;
	buffer_rewind(&req, 0);
	if (buffer_append(&req, base_url) ||
		buffer_append(&req, "/data/") ||
		buffer_strcpy(&req, resp.data + hash->start, json_size(hash)) ||
		buffer_append(&req, "/"))
		goto finish;
	req_buffer_state = req.n;
	if (buffer_append_double(&name, chapter->number, 3, 5) ||
		buffer_append(&name, "-"))
		goto finish;
	name_buffer_state = name.n;
	files = json_iter(data);
	while (json_next(&file, &files)) {
		int pos;
		const char *dot;
		if (page++ < pages)
			continue;
		if (buffer_strcpy(&req, resp.data + file->start, json_size(file)) ||
			http_get(req.data, NULL, NULL, &buf))
			goto finish;
		for (dot = NULL, pos = file->end; pos-- > file->start;) {
			if (resp.data[pos] == '.') {
				dot = resp.data + pos;
				break;
			}
		}
		if (buffer_append_ulong(&name, page, 3))
			goto finish;
		if (dot && buffer_strcpy(&name, dot, (size_t)(file->end - pos)))
			goto finish;
		if (save_page(archive, name.data, buf.data, buf.n))
			goto finish;
		printf("\33[2K\r%s: %lu/%lu", archive, page, json_count(data));
		fflush(stdout);
		buffer_rewind(&buf, 0);
		buffer_rewind(&req, req_buffer_state);
		buffer_rewind(&name, name_buffer_state);
	}
	result = OK;
finish:
	putchar('\n');
	free(json);
	free(base_url);
	buffer_free(&name);
	buffer_free(&resp);
	buffer_free(&req);
	buffer_free(&buf);
	return result;
}

static int save_chapter(const char *archive, const chapter_t *chapter, int resume, int checkonly)
{
	size_t pages = 0;
	zipFile zip;
	if (!resume || get_pages_in_file(archive, &pages)) {
		if (checkonly) {
			printf("Download: %s\n", archive);
			return OK;
		}
		if (!(zip = zipOpen(archive, APPEND_STATUS_CREATE)))
			return ERROR;
		zipClose(zip, NULL);
	}
	if (pages < chapter->pages) {
		if (checkonly) {
			printf("Resume:   %s\n", archive);
			return OK;
		}
		if (save_pages(archive, chapter, pages))
			return ERROR;
	}
	return OK;
}

static int save_chapters(const mdex_t *mdex)
{
	int result = ERROR, has_chapter;
	int overwrite = mdex->flags & MDEX_OVERWRITE;
	int checkonly = mdex->flags & MDEX_CHECKONLY;
	int usesubdir = mdex->flags & MDEX_USESUBDIR;
	chapter_t *chapter, *last = NULL;
	chapters_iter_t chapters = chapters_iter(&mdex->chapters);
	buffer_t name = buffer_make(0);
	buffer_t last_name = buffer_make(0);
	if (usesubdir && !checkonly)
		if (access(mdex->title, F_OK) && mkdir(mdex->title, 0750))
			goto finish;
	while ((has_chapter = chapters_next(&chapter, &chapters)) || last) {
		if (!has_chapter)
			goto check_last;
		if (chapter->skip)
			continue;
		if (get_file_name(&name, mdex, chapter))
			goto finish;
		if (!overwrite) {
			if (!access(name.data, F_OK)) {
				last = chapter;
				goto next;
			} else if (last) check_last: {
				if (get_file_name(&last_name, mdex, last) ||
					save_chapter(last_name.data, last, 1, checkonly))
					goto finish;
				last = NULL;
				buffer_rewind(&last_name, 0);
			}
		}
		if (has_chapter && save_chapter(name.data, chapter, 0, checkonly))
			goto finish;
next:
		buffer_rewind(&name, 0);
	}
	result = OK;
finish:
	buffer_free(&last_name);
	buffer_free(&name);
	return result;
}

int mdex_download(const mdex_args_t *args)
{
	int result = ERROR;
	mdex_t *mdex = mdex_create(args);
	if (!mdex)
		goto finish;
	if (get_title(mdex)) {
		puts("Failed to fetch series title");
		goto cleanup_mdex;
	} else if (get_chapters(mdex)) {
		puts("Failed to fetch chapters list");
		goto cleanup_mdex;
	} else if (!filter_chapters(mdex) && save_chapters(mdex)) {
		puts("Failed to download chapters");
		goto cleanup_mdex;
	}
	result = OK;
cleanup_mdex:
	mdex_delete(mdex);
finish:
	return result;
}
