#ifndef mobject_h
#define mobject_h

#include <stdbool.h>
#include "mvalue.h"
#include "mvm.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type) 
#define IS_STRING(value)    isObjType(value, OBJ_STRING) 

#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)
typedef enum
{
    OBJ_STRING
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
};
void free_objects(Vm*vm);

ObjString *take_string(char *chars, int length,Vm*vm);
ObjString *copy_string(const char *chars, int length,Vm*vm);
static inline bool isObjType(Value value, Objtype type){return IS_OBJ(value) && AS_OBJ(value)->type ==type;}


#endif