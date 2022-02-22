#include <stdlib.h>
#include "util.h"

#if !(defined(VECT_NAME) && defined(VECT_ELEM))
#error "You must define at least: VECT_NAME, VECT_ELEM"
#else

#if defined(VECT_PASS_VALUE) && !defined(VECT_PUSH_VALUE)
#define VECT_PUSH_VALUE
#endif
#if defined(VECT_PASS_VALUE) && !defined(VECT_ITER_VALUE)
#define VECT_ITER_VALUE
#endif
#if defined(VECT_PASS_VALUE) && !defined(VECT_FREE_VALUE)
#define VECT_FREE_VALUE
#endif

#if defined(VECT_EXTERN) || defined(VECT_HEADER)
#undef VECT_EXTERN
#define VECT_EXTERN extern
#else
#define VECT_EXTERN static
#endif

#define VECT_(X) CAT(CAT(VECT_NAME, _), X)

typedef VECT_ELEM(VECT_(elem_t));
#undef VECT_ELEM

#define VECT VECT_(t)
#define VECT_ELEM VECT_(elem_t)
#define VECT_ITER VECT_(iter_t)

typedef struct VECT_NAME {
	size_t size, n;
	VECT_ELEM *data;
} VECT;

typedef struct VECT_(iter) {
	VECT_ELEM *next, *end;
} VECT_ITER;

static VECT VECT_(make)(size_t size)
{
	VECT vect = {0};
	vect.size = size;
	return vect;
}

static VECT_ITER VECT_(iter)(const VECT *vect)
{
	VECT_ITER it = {0};
	it.next = vect->data;
	it.end = vect->data + vect->n;
	return it;
}

#ifdef VECT_ITER_VALUE
static int VECT_(next)(VECT_ELEM *elem, VECT_ITER *it)
#else
static int VECT_(next)(VECT_ELEM **elem, VECT_ITER *it)
#endif
{
	if (it->next == it->end)
		return 0;
#ifdef VECT_ITER_VALUE
	*elem = *it->next++;
#else
	*elem = it->next++;
#endif
	return 1;
}

VECT_EXTERN int VECT_(reserve)(VECT *vect, size_t size);
VECT_EXTERN void VECT_(free)(VECT *vect);

#ifdef VECT_PUSH_VALUE
VECT_EXTERN int VECT_(push)(VECT *vect, VECT_ELEM elem);
#else
VECT_EXTERN int VECT_(push)(VECT *vect, VECT_ELEM *elem);
#endif

#ifndef VECT_HEADER

VECT_EXTERN int VECT_(reserve)(VECT *vect, size_t size)
{
	if (vect->size < size)
		return TRY_REALLOC(&vect->data, &vect->size, size);
	return OK;
}

VECT_EXTERN void VECT_(free)(VECT *vect)
{
	if (!vect->data)
		return;
#ifdef VECT_FREE
	{
		size_t i = vect->n;
		while (i--) {
#ifdef VECT_FREE_VALUE
			VECT_FREE(vect->data[i]);
#else
			VECT_FREE(&vect->data[i]);
#endif
		}
	}
#endif
	free(vect->data);
	vect->data = NULL;
	vect->size = 0;
}

#ifdef VECT_PUSH_VALUE
VECT_EXTERN int VECT_(push)(VECT *vect, VECT_ELEM elem)
#else
VECT_EXTERN int VECT_(push)(VECT *vect, VECT_ELEM *elem)
#endif
{
	if (!vect->data) {
		size_t new_size = vect->size ? vect->size : 1;
		if (TRY_REALLOC(&vect->data, &vect->size, new_size))
			return ERROR;
		vect->n = 0;
	} else if (vect->n == vect->size) {
		size_t new_size = 2 * vect->size;
		if (TRY_REALLOC(&vect->data, &vect->size, new_size))
			return ERROR;
	}
#ifdef VECT_PUSH_VALUE
	vect->data[vect->n++] = elem;
#else
	vect->data[vect->n++] = *elem;
#endif
	return OK;
}

#endif
#endif

#undef VECT_NAME
#undef VECT_ELEM
#undef VECT_FREE
#undef VECT_PUSH_VALUE
#undef VECT_ITER_VALUE
#undef VECT_FREE_VALUE
#undef VECT_PASS_VALUE
#undef VECT_HEADER
#undef VECT_EXTERN
#undef VECT_ITER
#undef VECT_
#undef VECT
