#ifndef mobject_h
#define mobject_h

#include <stdbool.h>
#include <stdint.h>
#include "mvalue.h"
#include "mchunk.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type) 
#define IS_STRING(value)    isObjType(value, OBJ_STRING) 
#define IS_FUNCTION(value)  isObjType(value, OBJ_FUNCTION)

#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION
}Objtype;
struct Obj
{
    Objtype type;
    struct Obj *next;
};
struct ObjString
{
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};
typedef struct
{
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString *name;
}ObjFunction;

void free_objects();

ObjFunction *new_funciton();
ObjString *take_string(char *chars, int length);
ObjString *copy_string(const char *chars, int length);
static inline bool isObjType(Value value, Objtype type){return IS_OBJ(value) && AS_OBJ(value)->type ==type;}


#endif