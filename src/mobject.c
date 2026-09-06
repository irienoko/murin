#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "mobject.h"
#include "mvalue.h"
#include "mmemory.h"

#define ALLOCATE_OBJ(type, objtype)\
    (type*)allocate_obj(sizeof(type), objtype)
   
static Obj       *allocate_obj(size_t size, Objtype type);
static ObjString *allocate_string(char *chars, int length);

ObjString *copy_string(const char *chars, int length)
{
    char *heapchar = allocate(char, length+1);
    memcpy(heapchar, chars, length);
    heapchar[length] ='\0';
    return allocate_string(heapchar, length);
}

static ObjString *allocate_string(char *chars, int length)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

static Obj *allocate_obj(size_t size, Objtype type)
{
    Obj *object = (Obj*)rellocate(NULL, 0, size);
    object->type = type;
    return  object;
}