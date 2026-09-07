#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mobject.h"
#include "mvalue.h"
#include "mmemory.h"
#include "mvm.h"

#define ALLOCATE_OBJ(type, objtype,vm)\
    (type*)allocate_obj(sizeof(type), objtype,vm)
   
static Obj       *allocate_obj(size_t size, Objtype type,Vm*vm);
static ObjString *allocate_string(char *chars, int length,Vm*vm);
static void free_object(Obj *object);

void free_objects(Vm*vm)
{
    Obj *object = vm->objects;
    while (object !=NULL) 
    {
        Obj*next = object->next;
        free_object(object);
        object = next;
    }
}

ObjString *copy_string(const char *chars, int length,Vm*vm)
{
    char *heapchar = allocate(char, length+1);
    memcpy(heapchar, chars, length);
    heapchar[length] ='\0';
    return allocate_string(heapchar, length,vm);
}

ObjString *take_string(char *chars, int length,Vm*vm)
{
    return allocate_string(chars, length,vm);
}

static ObjString *allocate_string(char *chars, int length,Vm*vm)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING,vm);
    string->length = length;
    string->chars = chars;
    return string;
}

static Obj *allocate_obj(size_t size, Objtype type,Vm *vm)
{
    Obj *object = (Obj*)rellocate(NULL, 0, size);
    object->type = type;
    object->next = vm->objects;
    vm->objects = object;
    return  object;
}

static void free_object(Obj *object)
{
    switch (object->type) 
    {
        case OBJ_STRING:
        {
            ObjString *string = (ObjString*)object;
            free_array(char, string->chars, string->length+1);
            FREE(ObjString, object);
        }
    }
}