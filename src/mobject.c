#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mobject.h"
#include "mtabel.h"
#include "mvalue.h"
#include "mmemory.h"
#include "mvm.h"

#define ALLOCATE_OBJ(type, objtype,vm)\
    (type*)allocate_obj(sizeof(type), objtype,vm)
   
static Obj       *allocate_obj(size_t size, Objtype type,Vm*vm);
static ObjString *allocate_string(char *chars, int length,uint32_t hash, Vm*vm);
static uint32_t hash_string(const char* key, int length) ;
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
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = mtabel_findString(&vm->strings,chars,length,hash);
    if(interned !=NULL)return interned;
    memcpy(heapchar, chars, length);
    heapchar[length] ='\0';
    return allocate_string(heapchar,length,hash,vm);
}

ObjString *take_string(char *chars, int length,Vm*vm)
{
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = mtabel_findString(&vm->strings,chars,length,hash);
    if(interned !=NULL)
    {
        free_array(char, chars, length+1);
        return interned;
    }
    return allocate_string(chars,length,hash,vm);
}

static ObjString *allocate_string(char *chars, int length, uint32_t hash, Vm*vm)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING,vm);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    mtabel_add(&vm->strings, string, NIL_VAL);
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