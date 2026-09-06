#include <stdbool.h>
#include <string.h>
#include "mscanner.h"

typedef struct
{
    int line;
    const char *start;
    const char *cur;
}Scanner;

static Scanner __scanner;

static Token token_make(Tokentype type);
static Token token_string();
static Token token_number();
static Token token_identifier();
static Token token_error(const char*message);

static Tokentype tokentype_keyword_check(int start, int length, const char*rest,Tokentype type);
static Tokentype tokentype_identify_type();

static bool match(char ex);
static void white_space_skip();

static bool isAtEnd(){return (*__scanner.cur == '\0');}
static bool isDigit(char c){return c >= '0' && c <= '9';}
static bool isAlpha(char c){ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';}
static char peek(){return *__scanner.cur;}
static char peek_next(){if(isAtEnd())return '\0'; return __scanner.cur[1];}
static inline char advance(){__scanner.cur++; return __scanner.cur[-1];}

# pragma mark - APIs -

void mscanner_init(const char *source)
{
    __scanner.start = source;
    __scanner.cur = source;
    __scanner.line = 1;
}

Token mtoken_scan()
{
    white_space_skip();
    __scanner.start = __scanner.cur;
    if(isAtEnd()) return token_make(TOKEN_EOF);

    char c = advance();
    if(isAlpha(c))return token_identifier();
    if(isDigit(c))return token_number();

    switch (c) 
    {
        case '(': return token_make(TOKEN_LEFT_PAREN);
        case ')': return token_make(TOKEN_RIGHT_PAREN);
        case '{': return token_make(TOKEN_LEFT_BRACE);
        case '}': return token_make(TOKEN_RIGHT_BRACE);
        case ';': return token_make(TOKEN_SEMICOLON);
        case ',': return token_make(TOKEN_COMMA);
        case '.': return token_make(TOKEN_DOT);
        case '-': return token_make(TOKEN_MINUS);
        case '+': return token_make(TOKEN_PLUS);
        case '/': return token_make(TOKEN_SLASH);
        case '*': return token_make(TOKEN_STAR);

        case '!':
            return token_make(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return token_make(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return token_make(match('=') ? TOKEN_LESS_EQUAL: TOKEN_LESS);
        case '>':
            return token_make(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return token_string();
    }
    return token_error("Unexpected character.");
}

# pragma mark - PRIVATEs - 
static Token token_make(Tokentype type)
{
    Token token;
    token.type  = type;
    token.start = __scanner.start;
    token.length = (int)(__scanner.cur-__scanner.start);
    token.line = __scanner.line;
    return token;
}
static Token token_string()
{
    while(peek() != '"' && !isAtEnd())
    {
        if(peek()=='\n') __scanner.line++;
        advance();
    }
    if(isAtEnd())return token_error("Unterminated string.");
    advance();
    return token_make(TOKEN_STRING);
}
static Token token_number()
{
    while(isDigit(peek())) advance();
    if(peek() == '.' && isDigit(peek_next()))
    {
        advance();
        while(isDigit(peek())) advance();
    }
    return token_make(TOKEN_NUMBER);
}
static Token token_identifier()
{
    while(isAlpha(peek()) || isDigit(peek())) advance();
    return token_make(tokentype_identify_type());
}
static Token token_error(const char*message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = __scanner.line;
    return token;
}

static Tokentype tokentype_keyword_check(int start, int length, const char*rest,Tokentype type)
{
    if(__scanner.cur - __scanner.start == start + length && memcmp(__scanner.start+start, rest, length)==0)return type;
    return TOKEN_IDENTIFIER;
}
static Tokentype tokentype_identify_type()
{
    switch (__scanner.start[0]) 
    {
        case 'a': return tokentype_keyword_check(1,2,"nd",TOKEN_AND);
        case 'c': return tokentype_keyword_check(1,4,"lass",TOKEN_CLASS);
        case 'e': return tokentype_keyword_check(1,3,"lse",TOKEN_ELSE);
        case 'i': return tokentype_keyword_check(1,1,"f",TOKEN_IF);
        case 'n': return tokentype_keyword_check(1,2,"il",TOKEN_NIL);
        case 'o': return tokentype_keyword_check(1, 1, "r", TOKEN_OR);
        case 'p': return tokentype_keyword_check(1, 4, "rint", TOKEN_PRINT);
        case 'r': return tokentype_keyword_check(1, 5, "eturn", TOKEN_RETURN);
        case 's': return tokentype_keyword_check(1, 4, "uper", TOKEN_SUPER);
        case 'v': return tokentype_keyword_check(1, 2, "ar", TOKEN_VAR);
        case 'w': return tokentype_keyword_check(1, 4, "hile", TOKEN_WHILE);
        case 'f': if(__scanner.cur - __scanner.start > 1)
        {
            switch (__scanner.start[1]) 
            {
                case 'a': return  tokentype_keyword_check(2, 3, "lse", TOKEN_FALSE);
                case 'o': return  tokentype_keyword_check(2, 1, "r", TOKEN_FOR);
                case 'u': return  tokentype_keyword_check(2, 1, "n", TOKEN_FUN);
            }
        }break;
        case 't':
        {
            if(__scanner.cur - __scanner.start > 1)
            {
                switch (__scanner.start[1]) 
                {
                    case 'h': return tokentype_keyword_check(2, 2, "is", TOKEN_THIS);
                    case 'r': return tokentype_keyword_check(2, 2, "ue", TOKEN_TRUE);
                }
            }
        }break;
    }
    return TOKEN_IDENTIFIER;
}

static bool match(char ex)
{
    if(isAtEnd())return false;
    if(*__scanner.cur != ex)return false;
    __scanner.cur++;
    return true;
}
static void white_space_skip()
{
    for(;;)
    {
        char c = peek();
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                __scanner.line++;
                advance();
                break;
            case '/':
                if(peek_next() == '/')
                {
                    while(peek() != '\n' && !isAtEnd()) advance();
                }else{return;}
                break;
            default:
                return;
        }
    }
}