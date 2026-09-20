#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mvm.h"
#include "mchunk.h"
#include "da_array.h"
#include "mcompiler.h"
#include "mmemory.h"
#include "mobject.h"
#include "mtabel.h"
#include "mvalue.h"


# pragma mark - PROTOTYPEs - 
static Result run(Vm*vm);
static void error_at_runtime(Vm *vm,const char*format,...);
static void concatenate(Vm *vm);

static void defineNative(const char*name, NativeFn function,Vm *vm);
static Value clockNative(int argCount, Value *args);
static Vm *__cur_vm;

# pragma mark - APIs - 
void mvm_init(Vm*vm)
{
    da_init(&vm->stack);
    vm->stack.items = calloc(256, sizeof(Value));
    vm->stack_top = vm->stack.items;
    vm->stack.capacity = 256;
    mtabel_init(&vm->strings);
    vm->objects = NULL;
    __cur_vm = vm;
    vm->frameCount = 0;
    defineNative("clock", clockNative, vm);
}
void mvm_free(Vm*vm)
{
    mtabel_free(&vm->strings);
    mtabel_free(&vm->globals);
    da_free(&vm->stack);
    free_objects();
}

Vm *get_current_vm()
{
    return __cur_vm;
}

# pragma mark - Simple Functions - 
static void  vm_stack_push(Value value,Vm*vm)
{
    if(vm->stack.count >= vm->stack.capacity)
    {
        vm->stack.capacity = vm->stack.capacity<256 ? vm->stack.capacity * 2: 256;
        void *tmp = realloc((vm)->stack.items,(vm)->stack.capacity * sizeof(*(vm)->stack.items));
        if(!tmp){fprintf(stderr, "Segment fault\n"); exit(EXIT_FAILURE);}
        vm->stack.items = tmp;
    }
    *vm->stack_top = value;
    vm->stack_top++;
    vm->stack.count++;
}
static Value vm_stack_pop(Vm*vm)
{
    vm->stack_top--;
    vm->stack.count--;
    return *vm->stack_top;
}
static Value peek(int dist, Vm*vm)
{
    return vm->stack_top[-1-dist];
}

static bool call(ObjFunction *function, int argCount,Vm *vm)
{
    if(argCount != function->arity)
    {
        error_at_runtime(vm, "Expected %d argements but for %d.", function->arity, argCount);
        return false;
    }
    if(vm->frameCount == FRAMES_MAX)
    {
        error_at_runtime(vm, "Stack overflow.");
        return false;
    }
    CallFrame *frame = &vm->frames[vm->frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code.items;
    frame->slots = vm->stack_top - argCount - 1;
    return true;
}

static void defineNative(const char*name, NativeFn function,Vm *vm)
{
    vm_stack_push(OBJ_VAL(copy_string(name, (int)strlen(name))),vm);
    vm_stack_push((OBJ_VAL(new_native(function))), vm);
    mtabel_add(&vm->globals, AS_STRING(vm->stack.items[0]), vm->stack.items[1]);
    vm_stack_pop(vm);
    vm_stack_pop(vm);
} 

static Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}


static bool call_value(Value callee, int argCount,Vm *vm)
{
    if(IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee)) 
        {
            case OBJ_FUNCTION:
                return  call(AS_FUNCTION(callee),argCount,vm);
            case OBJ_NATIVE:
            {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm->stack_top - argCount);
                vm->stack_top -= argCount + 1;
                vm_stack_push(result, vm);
                return true;
            }
            default:break;
        }
    }
    error_at_runtime(vm, "can only call functions.");
    return false;
}
static bool is_falsey(Value value){return IS_NIL(value) || (IS_BOOL(value)&&!AS_BOOL(value));}
static bool value_equal(Value a, Value b)
{
    if(a.type != b.type)return false;
    switch (a.type) 
    {
        case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:       return true;
        case VAL_NUMBER:    return AS_NUMBER(a)==AS_NUMBER(b);
        case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b);
        default:            return false;
    }
}

# pragma mark - PROTOTYPE IMPLEMENTATIONS- 
static Result run(Vm*vm)
{
    CallFrame *frame = &vm->frames[vm->frameCount -1];
    #define READ_BYTE() (*frame->ip++)
    #define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2]<<8) | frame->ip[-1]))
    #define READ_STRING()  AS_STRING(READ_CONSTANT(READ_BYTE()))
    #define BINARY_OP(valuetype,op)\
        do{\
            if(!IS_NUMBER(peek(0,vm)) || !IS_NUMBER(peek(1,vm)))\
            {\
                error_at_runtime(vm,"Operands must be number.");\
                return RESULT_RUNTIME_ERROR;\
            }\
            double b = AS_NUMBER(vm_stack_pop(vm));\
            double a = AS_NUMBER(vm_stack_pop(vm));\
            vm_stack_push(valuetype(a op b),vm);\
        }while(false)
    //#define READ_CONSTANT_16() (vm->c->code[READ_BYTE()] | vm->c->code[READ_BYTE()+1] <<8)
    //#define READ_CONSTANT_32() (vm->c->code[READ_BYTE()] | vm->c->code[READ_BYTE()+1] <<8 | vm->c->code[READ_BYTE()+2] <<16)
    #define READ_CONSTANT(read) (frame->function->chunk.value.items[read])
    for(;;)
    {
        #ifdef DEBUG_TRACE_EXE
            printf("          ");
            for(Value *slot = vm->stack; slot < vm->stacktop; slot++)
            {
                printf("[ ");
                value_print(*slot);
                printf(" ]");
            }
            printf("\n");
            vm_disassemble_instruction(vm->c,(int)(vm->ip - vm->c->code));
        #endif  
        uint8_t inst;
        switch(inst=READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT(READ_BYTE());
                vm_stack_push(constant,vm);
                break;
            }

            case OP_PRINT:
            {
                mvalue_print(vm_stack_pop(vm));
                printf("\n");
                break;
            }

            case OP_POP:
            {
                vm_stack_pop(vm);
                break;
            }

            case OP_DEFINE_GLOBAL:
            {
                ObjString *name = READ_STRING();
                mtabel_add(&vm->globals, name, peek(0, vm));
                vm_stack_pop(vm);
                break;
            }

            case OP_GET_GLOBAL:
            {
                ObjString *name = READ_STRING();
                Value value;
                if(!mtabel_get(&vm->globals, name, &value))
                {
                    error_at_runtime(vm,"variable not defined '%s'.", name->chars);
                    return RESULT_RUNTIME_ERROR;
                }
                vm_stack_push(value, vm);
                break;
            }

            case OP_SET_GLOBAL:
            {
                ObjString *name = READ_STRING();
                if(mtabel_add(&vm->globals, name, peek(0, vm)))
                {
                    mtabel_delete(&vm->globals, name);
                    error_at_runtime(vm, "Undefined variable '%s'.",name->chars);
                    return RESULT_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                vm_stack_push(frame->slots[slot], vm);
                break;
            }

            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0, vm);
                break;
            }

            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                if(is_falsey(peek(0, vm))) frame->ip += offset;
                break;
            }

            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }

            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }

            case OP_ADD:
            {
                if(IS_STRING(peek(0, vm)) && IS_STRING(peek(1, vm)))
                {
                    concatenate(vm);
                }else if (IS_NUMBER(peek(0, vm)) && IS_NUMBER(peek(1, vm)))
                {
                    double a = AS_NUMBER(vm_stack_pop(vm));
                    double b = AS_NUMBER(vm_stack_pop(vm));
                    vm_stack_push(NUMBER_VAL(a + b), vm);
                }else{error_at_runtime(vm, "Operands must be two numbers or two strings.");return RESULT_RUNTIME_ERROR;}
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL,-); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL,*); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL,/); break;

            case OP_NIL:        vm_stack_push(NIL_VAL, vm); break;
            case OP_TRUE:       vm_stack_push(BOOL_VAL(true), vm); break;
            case OP_FALSE:      vm_stack_push(BOOL_VAL(false), vm); break;

            case OP_EQUAL:
            {
                Value a = vm_stack_pop(vm);
                Value b = vm_stack_pop(vm);
                vm_stack_push(BOOL_VAL(value_equal(a,b)), vm);
            }break;

            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            
            case OP_NOT:
                vm_stack_push(BOOL_VAL(is_falsey(vm_stack_pop(vm))), vm);
            break;

            case OP_NEGATE:
            {
                if(!IS_NUMBER(peek(1,vm)))
                {
                    error_at_runtime(vm,"Operand must be number.");
                    return RESULT_RUNTIME_ERROR;
                }
                vm_stack_push(NUMBER_VAL(-AS_NUMBER(vm_stack_pop(vm))), vm);
                break;
            }
            /*
            case OP_CONSTANT_16:
            {
                Value constant =  READ_CONSTANT(READ_BYTE() | ((READ_BYTE()+2)<<8));
                vm_stack_push(constant,vm);
                break;
            }
            case OP_CONSTANT_32:
            {
                Value constant =  READ_CONSTANT(READ_CONSTANT_32());
                vm_stack_push(constant,vm);
                break;
            }*/

            case OP_CALL:
            {
                int argCount = READ_BYTE();
                if(!call_value(peek(argCount, vm), argCount,vm))return RESULT_RUNTIME_ERROR;
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }
            case OP_RETURN:
            {
                Value result = vm_stack_pop(vm);
                vm->frameCount--;
                if(vm->frameCount == 0)
                {
                    vm_stack_pop(vm);
                    return RESULT_OK;
                }
                vm->stack_top = frame->slots;
                vm_stack_push(result, vm);
                frame = &vm->frames[vm->frameCount -1];
                break;
            }
        }
    }
    #undef READ_BYTE
    #undef READ_SHORT
    #undef BINARY_OP
    #undef READ_STRING
    #undef READ_CONSTANT
}
static void error_at_runtime(Vm *vm,const char*format,...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format,args);
    va_end(args);
    fputs("\n", stderr);

    for(int i = vm->frameCount - 1; i >= 0; i--)
    {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->function;
        size_t inst = frame->ip - frame->function->chunk.code.items - 1;

        fprintf(stderr, "[Line %d] in ", mchunk_get_line(&frame->function->chunk, inst));
        if(function->name == NULL)
        {
            fprintf(stderr, "sciprt\n");
        }else 
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    mvm_free(vm);
}

Result mvm_interpret_result(const char*source, Vm *vm)
{
    ObjFunction *function = compile(source);
    if(function == NULL) return RESULT_COMPILE_ERROR;

    vm_stack_push(OBJ_VAL(function), vm);

    call(function, 0, vm);

    return run(vm);
}

static void concatenate(Vm *vm)
{
    ObjString *b = AS_STRING(vm_stack_pop(vm));
    ObjString *a = AS_STRING(vm_stack_pop(vm));

    int length = a->length + b->length;
    char *chars = allocate(char, length+1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    ObjString *result = take_string(chars, length);
    vm_stack_push(OBJ_VAL(result), vm);
}