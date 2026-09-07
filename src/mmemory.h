#ifndef mmemory_h
#define mmemory_h

#include <stdlib.h>

#define allocate(type, count)\
    (type*)rellocate(NULL, 0, sizeof(type) *count)

#define free_array(type, pointer, oldCount) \
    rellocate(pointer, sizeof(type) * (oldCount), 0)
#define FREE(type, pointer) rellocate(pointer, sizeof(type), 0)

static inline void *rellocate(void *pointer, size_t olsize, size_t nsize)
{
    if(nsize==0){free(pointer);return NULL;}
    void *result = realloc(pointer, nsize);
    if(result==NULL)exit(1);
    return result;
}

#endif