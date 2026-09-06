#ifndef mvm_h
#define mvm_h

#include "mchunk.h"

typedef struct
{
    dy_value stack;
    Chunk *chunk;
    uint8_t *ip;
}Vm;

typedef enum
{
    RESULT_OK,
    RESULT_COMPILE_ERROR,
    RESULT_RUNTIME_ERROR
}Result;

void mvm_init(Vm*vm);
void mvm_free(Vm*vm);

Result mvm_interpret_result(const char*source,Vm*vm);


#endif