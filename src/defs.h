#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>

#define OK 0
#define ERROR -1

#define STRING_(X) #X
#define STRING(X) STRING_(X)

#define CONCAT__(X, Y) X ## Y
#define CONCAT_(X, Y) CONCAT__(X, Y)
#define CONCAT(X, Y) CONCAT_(X, Y)

#define SIZEOF(A) (sizeof(A) / sizeof(*(A)))
#define BASEOF(P, T, M) ((T *)(((char *)(P)) - offsetof(T, M)))

#define SRCLOC (__FILE__":"STRING(__LINE__))

#ifndef TAG
#define TAG SRCLOC
#endif

#endif
