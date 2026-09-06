#ifndef IRIE_DYNAMIC_ARRAYS_H
#define IRIE_DYNAMIC_ARRAYS_H

#include <stddef.h>
#include <stdlib.h>

#define dynamic_array(type)struct\
{\
    type *items;\
    size_t count;\
    size_t capacity;\
}

#define da_push(array, ...)\
do{\
    if((array)->count >= (array)->capacity)\
    {\
        (array)->capacity = (array)->capacity ? (array)->capacity *2 : 8;\
        void *tmp = realloc((array)->items,(array)->capacity * sizeof(*(array)->items));\
        if(!tmp){fprintf(stderr, "Out of memory\n"); exit(EXIT_FAILURE);}\
        (array)->items = tmp;\
    }\
    (array)->items[(array)->count++] =(__VA_ARGS__); \
}while (0)

#define da_push_noinc(array, ...)\
do{\
    if((array)->count >= (array)->capacity)\
    {\
        (array)->capacity = (array)->capacity ? (array)->capacity *2 : 8;\
        void *tmp = realloc((array)->items,(array)->capacity * sizeof(*(array)->items));\
        if(!tmp){fprintf(stderr, "Out of memory\n"); exit(EXIT_FAILURE);}\
        (array)->items = tmp;\
    }\
}while (0)

#define da_get_element(array, index) ((array)->items[(index)])

#define da_init(array)\
do{\
    (array)->items = NULL;\
    (array)->count = (array)->capacity = 0;\
}while(0)

#define da_free(array)\
do{\
    free((array)->items);\
    da_init(array);\
}while (0)

#define da_pop(array, out_ptr) \
    do { \
        if ((array)->count > 0) \
            *(out_ptr) = (array)->items[--(array)->count]; \
    } while (0)

#endif