#include "debug.h"
#include "mchunk.h"
#include "mvalue.h"
#include <stdint.h>
#include <stdio.h>

#pragma mark - PROTOTYPEs -
static int const_instruction(const char*name,Chunk*chunk,int offset);
static int const_16instruction(const char*name,Chunk*chunk,int offset);
static int const_32instruction(const char*name,Chunk*chunk,int offset);
static int simple_instruction(const char *message, int offset);

int disassemble_instructions(Chunk*chunk,int offset)
{
    
    printf("%04d ",offset);

    uint8_t instruction = chunk->code.items[offset];
    switch(instruction)
    {
        case OP_CONSTANT:
            return const_instruction("OP_CONSTANT",chunk,offset);
        case OP_RETURN:
            simple_instruction("OP_RETURN", offset);
        case OP_NEGATE:
            simple_instruction("OP_NEGATE", offset);
        case OP_NIL:
            simple_instruction("OP_NIL", offset);
        case OP_FALSE:
            simple_instruction("OP_FALSE", offset);
        case OP_TRUE:
            simple_instruction("OP_TRUE", offset);
        case OP_ADD:
            simple_instruction("OP_ADD", offset);
        case OP_NOT:
            simple_instruction("OP_NOT", offset);
        case OP_PRINT:
            simple_instruction("OP_PRINT", offset);
        case OP_POP:
            simple_instruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:
            simple_instruction("OP_DEFINE_GLOBAL", offset);
        case OP_GET_GLOBAL:
            simple_instruction("OP_GET_GLOBAL", offset);
        case OP_SET_GLOBAL:
            return const_instruction("OP_SET_GLOBAL",chunk,offset);
        case OP_GREATER:
            simple_instruction("OP_GREATER", offset);
        case OP_LESS:
            simple_instruction("OP_GREATER", offset);
        case OP_EQUAL:
            simple_instruction("OP_EQUAL", offset);
        case OP_SUBTRACT:
            simple_instruction("OP_SUBTRACT", offset);
        case OP_DIVIDE:
            simple_instruction("OP_DIVIDE", offset);
        case OP_MULTIPLY:
            simple_instruction("OP_MULTIPLY", offset);
        case OP_CONSTANT_16:
            return const_16instruction("OP_CONSTANT_16",chunk,offset);
        case OP_CONSTANT_32:
            return const_32instruction("OP_CONSTANT_32",chunk,offset);
        default:
            printf("Unrecognised opcode :( \"%d\".\n",instruction);
            return offset + 1;
    }
}

#pragma mark - PROTOTYPE IMPLEMENTATIONs-
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

static int simple_instruction(const char *message, int offset)
{
    printf("%s\n", message);
    return offset+1;
}

