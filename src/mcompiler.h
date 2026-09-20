#ifndef mcompiler_h
#define mcompiler_h

#include <stdbool.h>
#include <stdint.h>
#include "mchunk.h"
#include "mobject.h"
#include "mvm.h"

#define UINT8_COUNT (UINT8_MAX + 1)

ObjFunction *compile(const char*source);

#endif