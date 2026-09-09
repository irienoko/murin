#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mtabel.h"
#include "mmemory.h"
#include "mvalue.h"
#include "mobject.h"

#define TABLE_MAX_LOAD 0.75

static Entry *find_entry(Entry *entries, int capacity, ObjString *key);
static void adjust_capacity(Tabel *tabel, int capacity);

void mtabel_init(Tabel *tabel)
{
    tabel->capacity = 0;
    tabel->count = 0;
    tabel->entries = NULL;
}

bool mtabel_add(Tabel *tabel, ObjString *key, Value value)
{
    if(tabel->count +1 > tabel->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(tabel->capacity);
        adjust_capacity(tabel,capacity);
    }
    Entry *entry = find_entry(tabel->entries, tabel->capacity,key);
    bool isnewkey = entry->key==NULL;
    if(isnewkey && IS_NIL(entry->value))tabel->count++;
    entry->key = key;
    entry->value = value;
    return isnewkey;
}
bool mtabel_get(Tabel *tabel, ObjString *key, Value *value)
{
    if(tabel->count == 0) return false;
    Entry *entry = find_entry(tabel->entries, tabel->capacity, key);
    if(entry->key == NULL) return false;
    *value = entry->value;
    return true;
}
bool mtabel_delete(Tabel *tabel, ObjString *key)
{
    if(tabel->count == 0) return false;

    Entry *entry = find_entry(tabel->entries, tabel->capacity, key);
    if(entry->key == NULL)return false;
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void mtabel_copy(Tabel *from, Tabel *to)
{
    for(int i =0; i < from->capacity; i++)
    {
        Entry *entry = &from->entries[i];
        if(entry->key != NULL)
        {
            mtabel_add(to, entry->key, entry->value);
        }
    }
}

ObjString *mtabel_findString(Tabel *tabel, const char *chars, int length, uint32_t hash)
{
    if(tabel->count == 0) return NULL;
    uint32_t index = hash % tabel->capacity;
    for(;;)
    {
        Entry *entry = &tabel->entries[index];
        if(entry->key == NULL)
        {
            if(IS_NIL(entry->value))return NULL;
        }else if(entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length)==0)
        {
            return entry->key;
        }
        index = (index + 1)%tabel->capacity;
    }
}

void mtabel_free(Tabel *tabel)
{
    free_array(Entry, tabel->entries, tabel->capacity);
    mtabel_init(tabel);
}

static Entry *find_entry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;
    for(;;)
    {
        Entry *entry = &entries[index];
        if(entry->key == NULL)
        {
            if(IS_NIL(entry->value))
            {
                return tombstone != NULL ? tombstone : entry;
            }else 
            {
                if(tombstone == NULL) tombstone = entry;
            }
        }else if(entry->key == key)
        {
            return entry;
        }
        index = (index + 1)%capacity;
    }
}

static void adjust_capacity(Tabel *tabel, int capacity)
{
    Entry *entries = allocate(Entry, capacity);
    for(int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    tabel->count = 0;
    for(int i = 0; i < tabel->capacity; i++)
    {
        Entry *entry = &tabel->entries[i];
        if(entry->key == NULL) continue;
        
        Entry *dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        tabel->count++;
    }
    free_array(Entry, tabel->entries, tabel->capacity);
    tabel->entries = entries;
    tabel->capacity = capacity;
}