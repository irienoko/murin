#ifndef  mtabel_h
#define  mtabel_h

#include <stdbool.h>
#include <stdint.h>
#include "mvalue.h"

typedef struct
{
    ObjString *key;
    Value value;
}Entry;

typedef struct
{
    int count;
    int capacity;
    Entry *entries;
}Tabel;

void mtabel_init(Tabel *tabel);
bool mtabel_add(Tabel *tabel, ObjString *key, Value value);
bool mtabel_get(Tabel *tabel, ObjString *key, Value *value);
bool mtabel_delete(Tabel *tabel, ObjString *key);
void mtabel_copy(Tabel *from, Tabel *to);
void mtabel_free(Tabel *tabel);
ObjString *mtabel_findString(Tabel *tabel, const char *cahrs, int length, uint32_t hash);




#endif