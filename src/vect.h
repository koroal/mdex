#include "util.h"

#if !(defined(NAME) && defined(ELEM))
#error "You must define at least: NAME, ELEM"
#else

#if defined(ELEM_VALUE) && !defined(PUSH_VALUE)
#define PUSH_VALUE
#endif
#if defined(ELEM_VALUE) && !defined(ITER_VALUE)
#define ITER_VALUE
#endif
#if defined(ELEM_VALUE) && !defined(FREE_VALUE)
#define FREE_VALUE
#endif

#if defined(EXTERN) || defined(HEADER)
#undef EXTERN
#define EXTERN extern
#else
#define EXTERN static
#endif

#define VECT_(X) CONCAT(CONCAT(NAME, _), X)

typedef ELEM(VECT_(elem_t));
#undef ELEM

#define VECT VECT_(t)
#define ELEM VECT_(elem_t)
#define ITER VECT_(iter_t)

typedef struct NAME {
	size_t size, n;
	ELEM *data;
} VECT;

typedef struct VECT_(iter) {
	ELEM *next, *end;
} ITER;

static VECT VECT_(make)(size_t size)
{
	VECT vect = {0};
	vect.size = size;
	return vect;
}

static ITER VECT_(iter)(const VECT *vect)
{
	ITER it = {0};
	it.next = vect->data;
	it.end = vect->data + vect->n;
	return it;
}

#ifdef ITER_VALUE
static int VECT_(next)(ELEM *elem, ITER *it)
#else
static int VECT_(next)(ELEM **elem, ITER *it)
#endif
{
	if (it->next == it->end)
		return 0;
#ifdef ITER_VALUE
	*elem = *it->next++;
#else
	*elem = it->next++;
#endif
	return 1;
}

EXTERN int VECT_(reserve)(VECT *vect, size_t size);
EXTERN void VECT_(free)(VECT *vect);

#ifdef PUSH_VALUE
EXTERN int VECT_(push)(VECT *vect, ELEM elem);
#else
EXTERN int VECT_(push)(VECT *vect, ELEM *elem);
#endif

#ifndef HEADER

EXTERN int VECT_(reserve)(VECT *vect, size_t size)
{
	if (vect->size < size)
		return TRY_REALLOC(&vect->data, &vect->size, size);
	return OK;
}

EXTERN void VECT_(free)(VECT *vect)
{
	if (!vect->data)
		return;
#ifdef FREE
	{
		size_t i = vect->n;
		while (i--) {
#ifdef FREE_VALUE
			FREE(vect->data[i]);
#else
			FREE(&vect->data[i]);
#endif
		}
	}
#endif
	free(vect->data);
	vect->data = NULL;
	vect->size = 0;
}

#ifdef PUSH_VALUE
EXTERN int VECT_(push)(VECT *vect, ELEM elem)
#else
EXTERN int VECT_(push)(VECT *vect, ELEM *elem)
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
#ifdef PUSH_VALUE
	vect->data[vect->n++] = elem;
#else
	vect->data[vect->n++] = *elem;
#endif
	return OK;
}

#endif
#endif

#undef VECT_
#undef VECT
#undef FREE
#undef ITER
#undef ELEM
#undef NAME
#undef HEADER
#undef EXTERN
#undef PUSH_VALUE
#undef ITER_VALUE
#undef FREE_VALUE
#undef ELEM_VALUE
