#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mobject.h"
#include "mchunk.h"
#include "mtabel.h"
#include "mvalue.h"
#include "mmemory.h"
#include "mvm.h"

#define ALLOCATE_OBJ(type, objtype, vm)\
    (type*)allocate_obj(sizeof(type), objtype, vm)
   
static Obj       *allocate_obj(size_t size, Objtype type, Vm *vm);
static ObjString *allocate_string(char *chars, int length,uint32_t hash);
static uint32_t hash_string(const char* key, int length) ;
static void free_object(Obj *object);

void free_objects()
{
    Obj *object = get_current_vm()->objects;
    while (object !=NULL) 
    {
        Obj*next = object->next;
        free_object(object);
        object = next;
    }
}

ObjFunction *new_funciton()
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION, get_current_vm());
    function->arity = 0;
    function->name = NULL;
    mchunk_init(&function->chunk);
    return function;
}

ObjNative *new_native(NativeFn function)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE, get_current_vm());
    native->function = function;
    return native;
}

ObjString *copy_string(const char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = mtabel_findString(&get_current_vm()->strings,chars,length,hash);
    if(interned !=NULL)return interned;
    char *heapchar = allocate(char, length+1);
    memcpy(heapchar, chars, length);
    heapchar[length] ='\0';
    return allocate_string(heapchar,length,hash);
}

ObjString *take_string(char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = mtabel_findString(&get_current_vm()->strings,chars,length,hash);
    if(interned !=NULL)
    {
        free_array(char, chars, length+1);
        return interned;
    }
    return allocate_string(chars,length,hash);
}

static ObjString *allocate_string(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING, get_current_vm());
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    mtabel_add(&get_current_vm()->strings, string, NIL_VAL);
    return string;
}

static Obj *allocate_obj(size_t size, Objtype type, Vm *vm)
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
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction *function = (ObjFunction*)object;
            mchunk_free(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(ObjNative, object);
            break;
        }
    }
}

//FNV-1a hash
static uint32_t hash_string(const char* key, int length) 
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}