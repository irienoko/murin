#include <string.h>
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

typedef void(*ParseFn)(bool canAssign);
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence prece;
}Rule;

typedef struct
{
    Token name;
    int depth;
}Local;

typedef struct
{
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDeath;
}Compiler;

Parser  __parser;
Chunk   *__compiling_chunk;
Compiler *__cur_compiler = NULL;
Vm      *__vm;

# pragma mark - PROTOTYPEs - 
static void error_at(Token*token,const char*message);
static void advance();
static void consume(Tokentype type, const char*message);
static void emit_constant(Value value);
static void init_compiler(Compiler *compiler);
static void parse_precedence(Precedence prece);
static Rule *get_rule(Tokentype type);
static bool check(Tokentype type);
static bool match(Tokentype type);
static void declaration();
static void statement();
static void sync();
static uint8_t parse_variable(const char *message);
static void declare_variable();
static uint8_t identifier_constant(Token *name);
static bool identifier_equal(Token *a, Token *b);

static void block();
static void scope_begin(){__cur_compiler->scopeDeath++;};
static void scope_end();

# pragma mark - SIMPLE FUNCTIONs -
static Chunk    *current_chunk(){return __compiling_chunk;}
static Vm       *current_vm(){return __vm;}
static void     error(const char*message){error_at(&__parser.prev,message);}
static void     error_at_current(const char*message){error_at(&__parser.cur, message);}

static void emit_byte(uint8_t byte){mchunk_write(current_chunk(), byte, __parser.cur.line);}
static void emit_bytes(uint8_t byte1,uint8_t byte2){emit_byte(byte1);emit_byte(byte2);}
static void emit_return(){emit_byte(OP_RETURN);}
static void compiler_end(){emit_return(); if(__parser.had_error){mchunk_disassemble(current_chunk(), "==code==");}}
static void mark_initialised()
{
    __cur_compiler->locals[__cur_compiler->localCount - 1].depth = __cur_compiler->scopeDeath;
}
static void define_variable(uint8_t global)
{
    if(__cur_compiler->scopeDeath > 0)
    {
        mark_initialised();
        return;
    }
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

# pragma mark - PARSER FUNCTION RULES -
static void number(bool canAssign){double value = strtod(__parser.prev.start, NULL); emit_constant(NUMBER_VAL(value));}
static void binary(bool canAssign);
static void expression(){parse_precedence(PREC_ASSIGNMENT);}
static void grouping(bool canAssign){expression(); consume(TOKEN_RIGHT_PAREN,"Expect ')' after expression");}
static void unary(bool canAssign);
static void literal(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);

# pragma  mark - STATEMENTs -
static void print_statement();
static void expression_statement();

# pragma mark - DECLARATIONs -
static void var_declaration();

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
  [TOKEN_IDENTIFIER]    = {variable,     NULL,   PREC_NONE},
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
    Compiler compiler;
    init_compiler(&compiler);
    __compiling_chunk = chunk;
    __vm = vm;
    __parser.had_error = false;
    __parser.panic_mode = false;
    advance();
    while(!match(TOKEN_EOF)) declaration();
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
    __parser.had_error = true;
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
    bool canAssign = prece <= PREC_ASSIGNMENT;
    rule(canAssign);
    while (prece <= get_rule(__parser.cur.type)->prece) 
    {
        advance();
        ParseFn infix_rule = get_rule(__parser.prev.type)->infix;
        infix_rule(canAssign);
    }
    if(canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assigment target");
    }
}
static Rule *get_rule(Tokentype type)
{
    return &rules[type];
}

static bool check(Tokentype type)
{
    return __parser.cur.type == type;
}
static bool match(Tokentype type)
{
    if(!check(type)) return false;
    advance();
    return true;
}
static void declaration()
{
    if(match(TOKEN_VAR))
    {
        var_declaration();
    }else
    {
        statement();
    }
    if(__parser.panic_mode) sync();
}
static void statement()
{
    if(match(TOKEN_PRINT))
    {
        print_statement();
    }else if(match(TOKEN_LEFT_BRACE))
    {
        scope_begin();
        block();
        scope_end();
    }
    else
    {
        expression_statement();
    }
}
static void sync()
{
    __parser.panic_mode = false;
    while(__parser.cur.type != TOKEN_EOF)
    {
        if(__parser.prev.type == TOKEN_SEMICOLON) return;
        switch (__parser.cur.type) 
        {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:;
        }
        advance();
    }
}
static uint8_t parse_variable(const char *message)
{
    consume(TOKEN_IDENTIFIER, message);
    declare_variable();
    if(__cur_compiler->scopeDeath > 0) return 0;
    return identifier_constant(&__parser.prev);
}

static void add_local(Token name)
{
    if(__cur_compiler->localCount == UINT8_COUNT)
    {
        error("to many locals");
        return;
    }
    Local *local = &__cur_compiler->locals[__cur_compiler->localCount++];
    local->name = name;
    local->depth = -1;
}
static void declare_variable()
{
    if(__cur_compiler->scopeDeath == 0) return;;
    Token *name = &__parser.prev;
    for(int i = __cur_compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &__cur_compiler->locals[i];
        if(local->depth != -1 && local->depth < __cur_compiler->scopeDeath)break;
        if(identifier_equal(name, &local->name)) error("variable already defined");
    }
    add_local(*name);
}
static uint8_t make_constant(Value value)
{
    int constant = chunk_add_constant(current_chunk(), value);
    return(uint8_t)constant;
}
static uint8_t identifier_constant(Token *name)
{
    return make_constant(OBJ_VAL(copy_string(name->start, name->length, current_vm())));
}
static bool identifier_equal(Token *a, Token *b)
{
    if(a->length != b->length)return false;
    return memcmp(a->start, b->start, a->length)==0;
}

static int resolve_local(Compiler *compiler, Token *name)
{
    for(int i = __cur_compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &__cur_compiler->locals[i];
        if(identifier_equal(name, &local->name))
        {
            if (local->depth == -1)
            {
                error("Can use undeclared variable.");
            }
            return i;
        }
    }
    return -1;
}

static void emit_constant(Value value)
{
    mchunk_write_constant(current_chunk(), value, __parser.cur.line);
}
static void init_compiler(Compiler *compiler)
{
    compiler->localCount =0;
    compiler->scopeDeath = 0;
    __cur_compiler = compiler;
}

# pragma mark - PARSER FUNCTION RULES IMPLEMENTATIONS-
static void binary(bool canAssign)
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
static void literal(bool canAssign)
{
    switch (__parser.prev.type) 
    {
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_TRUE: emit_byte(OP_TRUE); break;
        case TOKEN_NIL: emit_byte(OP_NIL); break;
        default: return;
    }
}
static void unary(bool canAssign)
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
static void string(bool canAssign)
{
    emit_constant(OBJ_VAL(copy_string(__parser.prev.start+1, __parser.prev.length-2,current_vm())));
}
static void name_variable(Token name,bool canAssign)
{
    uint8_t getOP, setOP;
    int arg = resolve_local( __cur_compiler, &name);

    if(arg != -1)
    {
        getOP = OP_GET_LOCAL;
        getOP = OP_SET_LOCAL;
    }else
    {
        arg = identifier_constant(&name);
        getOP = OP_GET_GLOBAL;
        setOP = OP_SET_GLOBAL;
    }
    if(canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emit_bytes(setOP, (uint8_t)arg);
    }else{emit_bytes(getOP, (uint8_t)arg);}
}
static void variable(bool canAssign)
{
    name_variable(__parser.prev, canAssign);
}

static void block()
{
    while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void scope_end()
{
    __cur_compiler->scopeDeath--;
    while(__cur_compiler->localCount > 0 && __cur_compiler->locals[__cur_compiler->localCount - 1].depth > __cur_compiler->scopeDeath)
    {
        emit_byte(OP_POP);
        __cur_compiler->localCount--;
    }
}


# pragma  mark - STATEMENT IMPLEMENTATIONs-
static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' for end line.");
    emit_byte(OP_PRINT);
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' for end line.");
    emit_byte(OP_POP);
}

# pragma  mark - DECLARATION IMPLEMENTATIONs-
static void var_declaration()
{
    uint8_t global = parse_variable("Expect variable name");
    if(match(TOKEN_EQUAL))
    {
        expression();
    }else
    {
        emit_byte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' for end line.");
    define_variable(global);
}