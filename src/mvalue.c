#include "mvalue.h"
#include "mobject.h"
#include <stdio.h>

static void print_object(Value value);

void mvalue_print(Value value)
{
    switch(value.type)
    {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
        break;
        case VAL_NIL:
            printf("nil");
        break;
        case VAL_NUMBER:
            printf("%g",AS_NUMBER(value));
        break;
        case VAL_OBJ:
            print_object(value);
        break;
    }
}

static void print_object(Value value)
{
    switch (OBJ_TYPE(value)) 
    {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
        break;
    }
}