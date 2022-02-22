#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>

#define OK 0
#define ERROR -1

#define STR_(X) #X
#define STR(X) STR_(X)

#define CAT__(X, Y) X ## Y
#define CAT_(X, Y) CAT__(X, Y)
#define CAT(X, Y) CAT_(X, Y)

#define SIZEOF(A) (sizeof(A) / sizeof(*(A)))
#define BASEOF(P, T, M) ((T *)(((char *)(P)) - offsetof(T, M)))

#define SRCLOC __FILE__":"STR(__LINE__)

#ifndef TAG
#define TAG SRCLOC
#endif

#endif
