#include "debug.h"
#include "mvalue.h"
#include <stdint.h>

static int const_instruction(const char*name,Chunk*chunk,int offset);
static int const_16instruction(const char*name,Chunk*chunk,int offset);
static int const_32instruction(const char*name,Chunk*chunk,int offset);


int disassemble_instructions(Chunk*chunk,int offset)
{
    
    printf("%04d ",offset);

    uint8_t instruction = chunk->code.items[offset];
    switch(instruction)
    {
        case OP_CONSTANT:
            return const_instruction("OP_CONSTANT",chunk,offset);
        case OP_RETURN:
            printf("OP_RETURN\n");
            return + + 1;
        case OP_NEGATE:
            printf("OP_NEGATE\n");
            return  offset +1;
        case OP_NIL:
            printf("OP_NIL\n");
            return offset + 1;
        case OP_FALSE:
            printf("OP_FALSE\n");
            return offset + 1;
        case OP_TRUE:
            printf("OP_TRUE\n");
            return offset + 1;
        case OP_ADD:
            printf("OP_ADD\n");
            return  offset +1;
        case OP_NOT:
            printf("OP_NOT\n");
            return offset+1;
        case OP_GREATER:
            printf("OP_GREATER\n");
            return offset+1;
        case OP_LESS:
            printf("OP_LESS\n");
            return offset+1;
        case OP_EQUAL:
            printf("OP_EQUAL\n");
            return offset+1;
        case OP_SUBTRACT:
            printf("OP_SUBTRACT\n");
            return  offset +1;
        case OP_DIVIDE:
            printf("OP_DIVIDE\n");
            return  offset +1;
        case OP_MULTIPLY:
            printf("OP_MULTIPLY\n");
            return  offset +1;
        case OP_CONSTANT_16:
            return const_16instruction("OP_CONSTANT_16",chunk,offset);
        case OP_CONSTANT_32:
            return const_32instruction("OP_CONSTANT_32",chunk,offset);
        default:
            printf("Unrecognised opcode :( \"%d\".\n",instruction);
            return offset + 1;
    }
}

static int const_instruction(const char*name,Chunk*chunk,int offset)
{
    uint8_t constant = chunk->code.items[offset+1];
    printf("%-16s %4d", name, constant);
    mvalue_print(chunk->value.items[constant]);
    printf("\n");
    return offset +2;
}

static int const_16instruction(const char*name,Chunk*chunk,int offset)
{
    uint16_t constant = chunk->code.items[offset+1] | (chunk->code.items[offset +2] << 8);
    printf("%-16s %4d", name, constant);
    mvalue_print(chunk->value.items[constant]);
    printf("\n");
    return offset +3;
}


static int const_32instruction(const char*name,Chunk*chunk,int offset)
{
    uint32_t constant = chunk->code.items[offset+1] | (chunk->code.items[offset +2] << 8) | (chunk->code.items[offset +3] << 16);
    printf("%-16s %4d", name, constant);
    mvalue_print(chunk->value.items[constant]);
    printf("\n");
    return offset +4;
}

