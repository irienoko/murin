#ifndef mcompiler_h
#define mcompiler_h

#include <stdbool.h>
#include "mchunk.h"
#include "mvm.h"

bool compile(const char*source,Chunk*chunk,Vm*vm);

#endif