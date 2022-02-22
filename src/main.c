#include <stdio.h>
#include "defs.h"
#include "http.h"
#include "mdex.h"

static const char help[] =
"Usage: mdex [-wsdnto] [-l lang] [-c list] series [group...]\n\n"
"The first argument w/o dash must be a series link or uuid\n\n"
"-w      Overwrite existing files\n"
"-s      Save into a subdirectory\n"
"-d      Report duplicate chapters\n"
"-n      Only check and do not download\n"
"-t      Include chapter title in filename\n"
"-o      Override series title\n"
"-l lang Choose language by code (default is 'en')\n"
"-c list Choose chapters by ranges list (default is '-')\n\n"
"The rest are scanlation groups in the order of preference";

static char *get_optval(int argc, char **argv, int *i, int j)
{
	if (argv[*i][++j])
		return &argv[*i][j];
	if (argc - *i > 1)
		return argv[++*i];
	return NULL;
}

static int get_args(int argc, char **argv, mdex_args_t *out)
{
	int i, j;
	mdex_args_t args = {0};
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			if (!args.series) {
				args.series = argv[i];
				continue;
			} else {
				args.groups = (const char **)&argv[i];
				break;
			}
		}
		for (j = 1; argv[i][j]; ++j) {
			switch (argv[i][j]) {
			case 'w': args.flags |= MDEX_OVERWRITE; continue;
			case 's': args.flags |= MDEX_USESUBDIR; continue;
			case 'd': args.flags |= MDEX_REPORTDUP; continue;
			case 'n': args.flags |= MDEX_CHECKONLY; continue;
			case 't': args.flags |= MDEX_CHAPTITLE; continue;
			case 'l': args.lang = get_optval(argc, argv, &i, j); goto next;
			case 'o': args.title = get_optval(argc, argv, &i, j); goto next;
			case 'c': args.ranges = get_optval(argc, argv, &i, j); goto next;
			default: printf("Unknown option: -%c\n", argv[i][j]); goto error;
			}
		}
next:;
	}
	if (!args.series)
		goto error;
	*out = args;
	return OK;
error:
	puts(help);
	return ERROR;
}

static int global_init(void)
{
	if (http_init())
		return ERROR;
	return OK;
}

static void global_free(void)
{
	http_free();
}

int main(int argc, char **argv)
{
	int result;
	mdex_args_t args;
	if (get_args(argc, argv, &args) || global_init())
		return ERROR;
	result = mdex_download(&args);
	global_free();
	return result;
}
