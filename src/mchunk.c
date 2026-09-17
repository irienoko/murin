#include <stdio.h>
#include <string.h>
#include "da_array.h"
#include "debug.h"
#include "mchunk.h"

static int chunk_add_constant(Chunk*chunk,Value value);

#pragma mark - APIs -

void mchunk_init(Chunk *chunk)
{
    da_init(&chunk->code);
    da_init(&chunk->line);
    da_init(&chunk->value);
    chunk->capacity = 0;
    chunk->count = 0;
}

void mchunk_write(Chunk *chunk, uint8_t byte, int line)
{
    da_push(&chunk->code,byte);
    if(chunk->line.count <= 0){Line __line = {.line=line,.count=1}; da_push(&chunk->line, __line); return;}
    if(da_get_element(&chunk->line, chunk->line.count-1).line == line)
    {
        da_get_element(&chunk->line, chunk->line.count-1).count++;
        return;
    }else{Line __line = {.line=line,.count=1};da_push(&chunk->line, __line);}
}

uint8_t mchunk_write_constant_return_index(Chunk *chunk, Value value, int line)
{
    int index = chunk_add_constant(chunk,value);
    if(index<256)
    {
        mchunk_write(chunk,OP_CONSTANT,line);
        mchunk_write(chunk,index,line);
    }
    if(index>65535)
    {
        mchunk_write(chunk,OP_CONSTANT_32,line);
        mchunk_write(chunk,(uint8_t)(index & 0xff),line);
        mchunk_write(chunk,(uint8_t)((index >> 8))&0xff,line);
        mchunk_write(chunk,(uint8_t)((index >> 16))&0xff,line);
    }else if(index > 256)
    {
        mchunk_write(chunk,OP_CONSTANT_16,line);
        mchunk_write(chunk,(uint8_t)(index & 0xff),line);
        mchunk_write(chunk,(uint8_t)((index >> 8))&0xff,line);
    }
    return (uint8_t)index;
}

void mchunk_write_constant(Chunk *chunk, Value value, int line)
{
    int index = chunk_add_constant(chunk,value);
    if(index<256)
    {
        mchunk_write(chunk,OP_CONSTANT,line);
        mchunk_write(chunk,index,line);
        return;
    }
    if(index>65535)
    {
        mchunk_write(chunk,OP_CONSTANT_32,line);
        mchunk_write(chunk,(uint8_t)(index & 0xff),line);
        mchunk_write(chunk,(uint8_t)((index >> 8))&0xff,line);
        mchunk_write(chunk,(uint8_t)((index >> 16))&0xff,line);
        return;
    }else if(index > 256)
    {
        mchunk_write(chunk,OP_CONSTANT_16,line);
        mchunk_write(chunk,(uint8_t)(index & 0xff),line);
        mchunk_write(chunk,(uint8_t)((index >> 8))&0xff,line);
    }
}

int mchunk_get_line(Chunk*chunk,int index)
{
    int c_line = 0;
    dynamic_array(int) line_decode;
    da_init(&line_decode);

    //decode
    for(int i=0; i < chunk->line.count; i++)
    {
        for(int j=0; j < chunk->line.items[i].count; j++)
        {
            da_push(&line_decode, i+1);
        }
    }
    
    if(da_get_element(&line_decode, index))
    {
        memcpy(&c_line, &da_get_element(&line_decode, index), sizeof(int));
        da_free(&line_decode);
        return c_line;
    }
    da_free(&line_decode);
    return 0;
}

void mchunk_disassemble(Chunk*chunk,const char*name)
{
    printf("==%s==\n",name);
    for(int offset=0;offset<(int)chunk->code.count;)offset=disassemble_instructions(chunk, offset);
}

void mchunk_free(Chunk *chunk)
{
    da_free(&chunk->code);
    da_free(&chunk->line);
    da_free(&chunk->value);
    mchunk_init(chunk);
}

#pragma mark - PRIVATEs -

static int chunk_add_constant(Chunk*chunk,Value value)
{
    da_push(&chunk->value, value);
    return (chunk->value.count-1);
}
