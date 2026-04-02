#ifndef _KEN_COMPILER_H
#define _KEN_COMPILER_H

#ifndef _WIN32
# define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#define noreturn _Noreturn

typedef struct {
    FILE *fd;               // file descriptor
    const char *name;       // origin name
    const char *content;    // origin
    size_t length;          // origin length
} File;

//
// lex.c
//

typedef enum {
    // Literals
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STR_LIT,
    TOK_IDENT,  // identifier

    // Keyword
    TOK_FUNC,
    TOK_LET,
    TOK_MUT,
    
    // Data type
    TOK_UINT,   // defaults to U64
    TOK_INT,    // defaults to I64
    TOK_FLOAT,
    TOK_VOID,
    TOK_BOOL,
    TOK_STR,

    // Operators
    TOK_PLUS, // +
    TOK_MINUS, // -
    TOK_STAR, // *
    TOK_SLASH, // /
    TOK_PERCENT, // %

    // Punctuation
    TOK_LPAREN, // (
    TOK_RPAREN, // )
    TOK_LBRACE, // {
    TOK_RBRACE, // }
    TOK_LBRACKET, // [
    TOK_RBRACKET, // ]
    TOK_COMMA, // ,
    
    // End of file
    TOK_EOF,
} TokenType;

typedef struct {
    File *source;       // Source information
    TokenType type;     // Token kind
    const char *start;  // Where the token starts
    size_t length;      // How much length on starts
    int line, col;      // Line:column
} Token;

typedef struct {
    File *source;
    size_t pos;
    int line, col;
} Lexer;

// Functions
noreturn void error(const char *fmt, ...);
void error_tok(Token *tok, const char *fmt, ...); // Multi, no die.

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

void lex_init(Lexer *l, const char *source, size_t len);
Token *lex_next(Lexer *l);
// Tokenize Lexer and return list of token & token count
Token *lex_tokenize(Lexer *l, int *count);
const char *token_type_name(TokenType type);

//
// hashmap.c
//

typedef struct {
  char *key;
  int keylen;
  void *val;
} HashEntry;

typedef struct {
  HashEntry *buckets;
  int capacity;
  int used;
} HashMap;

void *hashmap_get(HashMap *map, char *key);
void *hashmap_get2(HashMap *map, char *key, int keylen);
void hashmap_put(HashMap *map, char *key, void *val);
void hashmap_put2(HashMap *map, char *key, int keylen, void *val);
void hashmap_delete(HashMap *map, char *key);
void hashmap_delete2(HashMap *map, char *key, int keylen);
void hashmap_test(void);

#endif
