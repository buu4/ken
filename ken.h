#ifndef _KEN_COMPILER_H
#define _KEN_COMPILER_H

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define noreturn _Noreturn
#define public // Indicate extern

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

noreturn void error(const char *fmt, ...);
extern char *ken_progname;

//
// file.c
//

typedef struct {
    FILE *d;
    char *name;
    char *content;
    size_t length;
} File;

void file_init(File *file, char *path);
void file_close(File *file);

//
// lex.c
//

typedef enum {
    // Literals
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STR_LIT,
    TOK_IDENT,

    // Keyword
    TOK_FUNC,
    TOK_LET,
    TOK_MUT,
    
    // Data type
    TOK_UINT,   // defaults to U64
    TOK_INT,    // defaults to I64
    TOK_FLOAT,
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
    
    TOK_EOF,
    TOK_ERR,
} TokenType;

typedef struct {
    File *source;       // Source information
    TokenType type;     // Token kind
    const char *loc;    // Where the token located 
    size_t length;      // How much length on location 
    int line, col;      // Line:column
} Token;

typedef struct {
    File *source;
    size_t pos;
    int line, col;
} Lexer;

void error_tok(Token *tok, const char *fmt, ...);
void lex_init(Lexer *l, File *file);
Token lex_next(Lexer *l);

// Tokenize Lexer and return list of token & token count
Token *lex_tokenize(Lexer *l, int *count);

const char *token_type_name(TokenType type);

// will not removed, use macro instead
#ifndef NDEBUG
void print_tokens(Token *tokens, int count);
# else
#  define print_tokens(x, y)
#endif

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
