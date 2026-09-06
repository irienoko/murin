#ifndef mobject_h
#define mobject_h

#include <stdbool.h>
#include "mvalue.h"

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
};
struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};
ObjString *copy_string(const char *chars, int length);
static inline bool isObjType(Value value, Objtype type){return IS_OBJ(value) && AS_OBJ(value)->type ==type;}


#endif