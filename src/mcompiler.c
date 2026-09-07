#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mcompiler.h"
#include "mchunk.h"
#include "mobject.h"
#include "mscanner.h"
#include "mvalue.h"
#include "mvm.h"


typedef struct
{
    Token cur;
    Token prev;
    bool had_error;
    bool panic_mode;
}Parser;
typedef enum 
{
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
}Precedence;

typedef void(*ParseFn)();
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence prece;
}Rule;

Parser  __parser;
Chunk   *__compiling_chunk;
Vm      *__vm;

# pragma mark - PROTOTYPEs - 
static void error_at(Token*token,const char*message);
static void advance();
static void consume(Tokentype type, const char*message);
static void compiler_end();
static void emit_constant(Value value);
static void parse_precedence(Precedence prece);
static Rule *get_rule(Tokentype type);

# pragma mark - SIMPLE FUNCTIONs -
static Chunk    *current_chunk(){return __compiling_chunk;}
static Vm       *current_vm(){return __vm;}
static void     error(const char*message){error_at(&__parser.prev,message);}
static void     error_at_current(const char*message){error_at(&__parser.cur, message);}

static void emit_byte(uint8_t byte){mchunk_write(current_chunk(), byte, __parser.prev.line);}
static void emit_bytes(uint8_t byte1,uint8_t byte2){emit_byte(byte1);emit_byte(byte2);}
static void emit_return(){emit_byte(OP_RETURN);}
static void compiler_end(){emit_return(); if(__parser.had_error){mchunk_disassemble(current_chunk(), "==code==");}}

# pragma mark - PARSER FUNCTION RULES -
static void number(){double value = strtod(__parser.prev.start, NULL); emit_constant(NUMBER_VAL(value));}
static void binary();
static void expression(){parse_precedence(PREC_ASSIGNMENT);}
static void grouping(){expression(); consume(TOKEN_RIGHT_PAREN,"Expect ')' after expression");}
static void unary();
static void literal();
static void string();

# pragma mark - MASSIVE PARSR RULES LIST -
static Rule rules[] = 
{
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

# pragma mark - APIs - 
bool compile(const char*source,Chunk*chunk,Vm*vm)
{
    mscanner_init(source);
    __compiling_chunk = chunk;
    __vm = vm;
    __parser.had_error = false;
    __parser.panic_mode = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    compiler_end();
    return !__parser.had_error;
}

# pragma mark - PROTOTYPEs IMPLEMENTATIONS - 
static void error_at(Token*token,const char*message)
{
    if(__parser.panic_mode) return;
    __parser.panic_mode = true;
    fprintf(stderr, "[Line %d] Error", token->line);
    if(token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }else if(token->type == TOKEN_ERROR)
    {

    }else 
    {
        fprintf(stderr, " at '%.*s'",token->length,token->start);
    }
    fprintf(stderr, ": %s\n",message);
    __parser.had_error = false;
}
static void advance()
{
    __parser.prev = __parser.cur;
    for(;;)
    {
        __parser.cur = mtoken_scan();
        if(__parser.cur.type != TOKEN_ERROR)break;
        error_at_current(__parser.cur.start);
    }
}
static void consume(Tokentype type, const char*message)
{
    if(__parser.cur.type == type)
    {
        advance();
        return;
    }
    error_at_current(message);
}
static void parse_precedence(Precedence prece)
{
    advance();
    ParseFn rule = get_rule(__parser.prev.type)->prefix;
    if(rule==NULL)
    {
        error("Expect expression. ");
        return;
    }
    rule();
    while (prece <= get_rule(__parser.cur.type)->prece) 
    {
        advance();
        ParseFn infix_rule = get_rule(__parser.prev.type)->infix;
        infix_rule();
    }
}
static Rule *get_rule(Tokentype type)
{
    return &rules[type];
}
static void emit_constant(Value value)
{
    mchunk_write_constant(current_chunk(), value, __parser.prev.line);
}
static void binary()
{
    Tokentype operator_type = __parser.prev.type;
    Rule *rule = get_rule(operator_type);
    parse_precedence((Precedence)(rule->prece++));
    switch (operator_type) 
    {
        case TOKEN_BANG_EQUAL:      emit_bytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:     emit_byte(OP_EQUAL);break;
        case TOKEN_GREATER:         emit_byte(OP_GREATER);break;
        case TOKEN_GREATER_EQUAL:   emit_bytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emit_byte(OP_LESS);break;
        case TOKEN_LESS_EQUAL:      emit_bytes(OP_GREATER, OP_NOT);break;
        case TOKEN_PLUS:            emit_byte(OP_ADD); break;
        case TOKEN_MINUS:           emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR:            emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH:           emit_byte(OP_DIVIDE); break;
        default: return;
    }
}
static void literal()
{
    switch (__parser.prev.type) 
    {
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_TRUE: emit_byte(OP_TRUE); break;
        case TOKEN_NIL: emit_byte(OP_NIL); break;
        default: return;
    }
}
static void unary()
{
    Tokentype operatorType = __parser.prev.type;
    parse_precedence(PREC_UNARY);
    switch (operatorType) 
    {
        case TOKEN_BANG: emit_byte(OP_NOT); break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        default: return; // Unreachable
    }
}
static void string()
{
    emit_constant(OBJ_VAL(copy_string(__parser.prev.start+1, __parser.prev.length-2,current_vm())));
}
