#ifndef MDEX_H
#define MDEX_H

#define MDEX_OVERWRITE (1 << 0)
#define MDEX_USESUBDIR (1 << 1)
#define MDEX_REPORTDUP (1 << 2)
#define MDEX_CHECKONLY (1 << 3)
#define MDEX_CHAPTITLE (1 << 4)

typedef struct mdex_args {
	const char *series;
	const char *ranges;
	const char *title;
	const char *lang;
	const char **groups;
	unsigned flags;
} mdex_args_t;

int mdex_download(const mdex_args_t *args);

#endif
