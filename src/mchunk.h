#ifndef mchunk_h
#define mchunk_h

#include "da_array.h"
#include <stdint.h>

#include "mvalue.h"

enum opcode
{
    OP_RETURN,
    OP_NEGATE,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_CONSTANT,
    OP_CONSTANT_16,
    OP_CONSTANT_32,
    OP_PRINT,
    OP_POP,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_DEFINE_GLOBAL,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP
};
typedef struct 
{
    int line;
    int count;
}Line;

typedef dynamic_array(uint8_t)dy_code;
typedef dynamic_array(Line)dy_line;
typedef dynamic_array(Value)dy_value;
typedef struct
{
    dy_code code;
    dy_line line;
    dy_value value;
    int count;
    int capacity;
}Chunk;

/// Initlise `chunk`
///
/// @param[in] chunk -> pointer to chunk struct
void mchunk_init(Chunk *chunk);

/// Write `byte` to `chunk` at `line`
///
/// @param[in] chunk    -> pointer to chunk struct
/// @param[in] byte     -> 8 byte instruction
/// @param[in] line     -> line number
void mchunk_write(Chunk *chunk, uint8_t byte, int line);

/// Write `value` to `chunk` at `line`
///
/// @param[in] chunk    -> pointer to chunk struct
/// @param[in] value    -> value to write to chunk
/// @param[in] line     -> line numb
void mchunk_write_constant(Chunk *chunk, Value value, int line);

/// Write `value` to `chunk` at `line` return index
///
/// @param[in] chunk    -> pointer to chunk struct
/// @param[in] value    -> value to write to chunk
/// @param[in] line     -> line numb
uint8_t mchunk_write_constant_return_index(Chunk *chunk, Value value, int line);

/// diassemble `chunk`
///
/// @param[in] chunk    -> pointer to chunk struct
/// @param[in] index    -> name
void mchunk_disassemble(Chunk*chunk,const char*name);

/// Get line number from `index`
///
/// @param[in] chunk    -> pointer to chunk struct
/// @param[in] index    -> instruction index
int mchunk_get_line(Chunk*chunk,int index);

int chunk_add_constant(Chunk*chunk,Value value);

/// Free chunk
///
/// @param[in] chunk    -> pointer to chunk struct
void mchunk_free(Chunk *chunk);






#endif