#ifndef mvm_h
#define mvm_h

#include "mchunk.h"
#include "mtabel.h"
#include "mobject.h"

#define FRAMES_MAX 64

typedef struct
{
    ObjFunction *function;
    uint8_t *ip;
    Value *slots;
}CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    dy_value stack;
    Value *stack_top;
    Tabel globals;
    Tabel strings;
    Obj *objects;
}Vm;

typedef enum
{
    RESULT_OK,
    RESULT_COMPILE_ERROR,
    RESULT_RUNTIME_ERROR
}Result;

void mvm_init(Vm*vm);
void mvm_free(Vm*vm);
Vm *get_current_vm();

Result mvm_interpret_result(const char*source,Vm*vm);

void mvm_defineNative(const char*name, NativeFn function,Vm *vm);

Value peek(int dist, Vm*vm);


#endif