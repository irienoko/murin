#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mvm.h"
#include "mchunk.h"
#include "da_array.h"
#include "mcompiler.h"
#include "mmemory.h"
#include "mobject.h"
#include "mvalue.h"

# pragma mark - PROTOTYPEs - 
static Result run(Vm*vm);
static void error_at_runtime(Vm*vm,const char*format,...);
static void concatenate(Value a, Value b, Vm *vm);

# pragma mark - APIs - 
void mvm_init(Vm*vm)
{
    da_init(&vm->stack);
}
void mvm_free(Vm*vm)
{
    da_free(&vm->stack);
}

# pragma mark - Simple Functions - 
static void  vm_stack_push(Value v,Vm*vm){da_push(&vm->stack, v);}
static Value vm_stack_pop(Vm*vm)
{
    Value out;
    da_pop(&vm->stack, &out);
    return out;
}
static Value peek(int dist, Vm*vm)
{

    return da_get_element(&vm->stack, 1-dist);
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
        case VAL_OBJ:
        {
            ObjString *astring = AS_STRING(a);
            ObjString *bstring = AS_STRING(b);
            return astring->length == bstring->length && memcmp(astring->chars, bstring->chars, astring->length)==0;
        }
        default:            return false;
    }
}

# pragma mark - PROTOTYPE IMPLEMENTATIONS- 
static Result run(Vm*vm)
{
    #define READ_BYTE() (*vm->ip++)
    #define BINARY_OP(valuetype,op)\
        do{\
            Value outb = vm_stack_pop(vm);\
            Value outa = vm_stack_pop(vm);\
            if(!IS_NUMBER(outa) || !IS_NUMBER(outb))\
            {\
                error_at_runtime(vm,"Operands must be number.");\
                return RESULT_RUNTIME_ERROR;\
            }\
            double b = AS_NUMBER(outb);\
            double a = AS_NUMBER(outa);\
            vm_stack_push(valuetype(a op b),vm);\
        }while(false)
    //#define READ_CONSTANT_16() (vm->c->code[READ_BYTE()] | vm->c->code[READ_BYTE()+1] <<8)
    //#define READ_CONSTANT_32() (vm->c->code[READ_BYTE()] | vm->c->code[READ_BYTE()+1] <<8 | vm->c->code[READ_BYTE()+2] <<16)
    #define READ_CONSTANT(read) (vm->chunk->value.items[read])
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
                printf("op_code:%d\n", inst);
                Value constant = READ_CONSTANT(READ_BYTE());
                vm_stack_push(constant,vm);
                break;
            }

            case OP_ADD:
            {
                Value a = vm_stack_pop(vm);
                Value b = vm_stack_pop(vm);
                if(IS_STRING(a) && IS_STRING(b))
                {
                    concatenate(a,b,vm);
                }else if (IS_NUMBER(a) && IS_NUMBER(b))
                {
                    vm_stack_push(NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b)), vm);
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
            case OP_RETURN:
            {
                printf("op_code:%d\n", inst);
                mvalue_print(vm_stack_pop(vm));
                printf("\n");
                return RESULT_OK;
            }
        }
    }
    #undef READ_BYTE
    #undef BINARY_OP
    #undef READ_CONSTANT
}
static void error_at_runtime(Vm*vm,const char*format,...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format,args);
    va_end(args);
    fputs("\n", stderr);

    size_t inst = vm->ip - vm->chunk->code.items - 1;
    printf("%04d\n",(int)inst);
    int line = mchunk_get_line(vm->chunk, inst);
    fprintf(stderr, "[line %d] in script\n",line);
    mvm_free(vm);
}

Result mvm_interpret_result(const char*source,Vm*vm)
{
    Chunk chunk;
    mchunk_init(&chunk);
    if(!compile(source,&chunk))
    {
        mchunk_free(&chunk);
        return RESULT_COMPILE_ERROR;
    }

    
    vm->chunk = &chunk;
    vm->ip = vm->chunk->code.items;

    Result result = run(vm);
    mchunk_free(&chunk);

    return result;
}

static void concatenate(Value a, Value b, Vm *vm)
{
    ObjString *b1 = AS_STRING(b);
    ObjString *a1 = AS_STRING(a);

    int length = a1->length + b1->length;
    char *chars = allocate(char, length+1);
    memcpy(chars, a1->chars, a1->length);
    memcpy(chars + a1->length, b1->chars, b1->length);
    chars[length] = '\0';
    ObjString *result = take_string(chars, length);
    vm_stack_push(OBJ_VAL(result), vm);
}