#ifndef mcompiler_h
#define mcompiler_h

#include <stdbool.h>
#include <stdint.h>
#include "mchunk.h"
#include "mvm.h"

#define UINT8_COUNT (UINT8_MAX + 1)

bool compile(const char*source,Chunk*chunk,Vm*vm);

#endif