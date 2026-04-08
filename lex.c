#include "ken.h"

static Token make_token(Lexer *l, TokenType type, const char *loc, size_t len)
{
    int line_no = 1;
    const char *p;
    const char *start = loc;

    while (l->source->content < start && start[-1] != '\n')
        start--;

    for (p = l->source->content; p < loc; p++)
        if (*p == '\n')
            line_no++;

    return (Token){
        .source = l->source,
        .type = type,
        .loc = loc,
        .length = len,
        .line = line_no,
        .col = (int)(loc - start) + 1,
     };
}

static char peek(Lexer *l)
{
    if (l->pos >= l->source->length)
        return '\0';
    return l->source->content[l->pos];
}

static char peek_at(Lexer *l, int n)
{
    if (l->pos + n >= l->source->length)
        return '\0';
    return l->source->content[l->pos + n];
}

static char peek_next(Lexer *l)
{
    return peek_at(l, 1);
}

static char advance(Lexer *l)
{
    assert(l->col > 0);
    assert(l->line > 0);

    char c = l->source->content[l->pos++];
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static void skip_whitespace(Lexer *l)
{
    while (l->pos < l->source->length) {
        char c = peek(l);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(l);
        } else if (c == '/' && peek_next(l) == '/') {
            while (l->pos < l->source->length && peek(l) != '\n')
                advance(l);
        } else if (c == '/' && peek_next(l) == '*') {
            // skip /*
            advance(l);
            advance(l);
            while (l->pos < l->source->length)
            {
                if (peek(l) == '*' && peek_next(l) == '/')
                    break;
                advance(l);
            }
            // skip */
            advance(l);
            advance(l);
        } else {
            break;
        }
    }
}

static void canonicalize_newline(char *p)
{ // Replaces \r or \r\n to \n
  // Swaping method; i = view, j = replacer
    int i = 0, j = 0;

    while (p[i])
    {
        if (p[i] == '\r') {
            if (p[i + 1] == '\n')
                i += 2;
            else
                i++;
        
            p[j++] = '\n';
        } else {
            p[j++] = p[i++];
        }
    }

    p[j] = '\0';
}

static Token lex_number(Lexer *l)
{
    const char *loc = l->source->content + l->pos;
    TokenType result_type = TOK_INT_LIT;

    while (l->pos < l->source->length) {
        // Allows 1_100
        while (isdigit(peek(l)) || (peek(l) == '_' && isdigit(peek_next(l)))) {
            advance(l);
        }
        // Check float
        if (peek(l) == '.') {
            advance(l); // skip . dot
            result_type = TOK_FLOAT_LIT;
            continue;
        }
        break;
    }

    return make_token(l, result_type, loc, (size_t)((l->source->content + l->pos) - loc));
}

static Token lex_string(Lexer *l)
{
    const char *loc = l->source->content + l->pos;
    advance(l); // skip opening "

    while (l->pos < l->source->length) {
        if (peek(l) == '"') {
            advance(l); // skip closing "
            return make_token(l, TOK_STR_LIT, loc, (size_t)((l->source->content + l->pos) - loc));
        }
        advance(l);
    }

    error_at(l, loc, "Unterminated string literal");
}

typedef struct {
    char *kw;
    size_t kw_len;
    TokenType type;
} kwMap;

static void *check_kw(char *name, size_t len)
{
    static HashMap map;

    if (map.capacity == 0) {
        static kwMap kw[] = {
            {"let", 3, TOK_LET},    {"mut", 3, TOK_MUT},
            {"func", 4, TOK_FUNC},  {"uint", 4, TOK_UINT},
            {"int", 3, TOK_INT},    {"float", 5, TOK_FLOAT},
            {"bool", 4, TOK_BOOL},  {"str", 3, TOK_STR},
        };

        for (size_t i = 0; i < sizeof(kw) / sizeof(*kw); i++)
            hashmap_put2(&map, kw[i].kw, kw[i].kw_len, &kw[i]);
    }

    return hashmap_get2(&map, name, len);
}

static Token lex_ident(Lexer *l)
{
    char *loc = l->source->content + l->pos;
    while (isalnum(peek(l)) || peek(l) == '_')
        advance(l);

    size_t len = (size_t)((l->source->content + l->pos) - loc);
    kwMap *kw = (kwMap*)check_kw(loc, len);
    TokenType type = kw ? kw->type : TOK_IDENT;

    return make_token(l, type, loc, len);
}

Token lex_next(Lexer *l)
{
    skip_whitespace(l);
    if (l->pos >= l->source->length)
        return make_token(l, TOK_EOF, l->source->content + l->pos, 0);

    char c = peek(l);

    if (isdigit(c) || (c == '-' && isdigit(peek_at(l, 1))))
        return lex_number(l);
    else if (c == '"')
        return lex_string(l);
    else if (isalpha(c) || c == '_')
        return lex_ident(l);

    const char *loc = l->source->content + l->pos;
    advance(l); // skip c

    switch (c) {
    case '(':
        return make_token(l, TOK_LPAREN, loc, 1);
    case ')':
        return make_token(l, TOK_RPAREN, loc, 1);
    case '{':
        return make_token(l, TOK_LBRACE, loc, 1);
    case '}':
        return make_token(l, TOK_RBRACE, loc, 1);
    case '[':
        return make_token(l, TOK_LBRACKET, loc, 1);
    case ']':
        return make_token(l, TOK_RBRACKET, loc, 1);
    case '.':
        return make_token(l, TOK_DOT, loc, 1);
    case ',':
        return make_token(l, TOK_COMMA, loc, 1);
    case '+':
        return make_token(l, TOK_PLUS, loc, 1);
    case '-':
        return make_token(l, TOK_MINUS, loc, 1);
    case '*':
        return make_token(l, TOK_STAR, loc, 1);
    case '/':
        return make_token(l, TOK_SLASH, loc, 1);
    case '%':
        return make_token(l, TOK_PERCENT, loc, 1);
    }

    error_at(l, loc, "Unexpected character");
}

void lex_init(Lexer *l, File *file)
{
    l->source = file;
    l->pos = 0;
    l->line = 1;
    l->col = 1;
}

Token *lex_tokenize(Lexer *l, int *count)
{
    assert(l->source);
    assert(l->source->content);
    assert(l->source->length);

// UTF-8 texts may start with a 3-byte "BOM" marker sequence.
// If exists, just skip them because they are useless bytes.
// (It is actually not recommended to add BOM markers to UTF-8
// texts, but it's not uncommon particularly on Windows.)
    if (!memcmp(l->source->content, "\xef\xbb\xbf", 3))
         l->source->content += 3;

    canonicalize_newline(l->source->content);
// update the source length including null terminator
// because we have been modified the pointer
    l->source->length = strlen(l->source->content);

    size_t cap = 256;
    int n = 0;
    Token *tokens = malloc(sizeof(Token) * cap);

    while (1) {
        if (n >= cap) {
            cap *= 2;
            tokens = realloc(tokens, sizeof(Token) * cap);
        }
        tokens[n] = lex_next(l);
        if (tokens[n].type == TOK_EOF) {
            n++;
            break;
        }
        n++;
    }
    *count = n;
    return tokens;
}

#ifndef NDEBUG
void print_tokens(Token *tokens, int count)
{ // Print array of tokens for debugging
    int top_line = 0; // biggest line
    for (int i = 0; i < count; i++) {
        if (tokens[i].line > top_line) {
            top_line = tokens[i].line;
            printf("%s:%d\n", tokens[i].source->name, top_line);
        }
        printf("  %-12s '%.*s'\n", token_type_name(tokens[i].type), (int)tokens[i].length,
            tokens[i].loc);
    }
}
#endif

const char *token_type_name(TokenType type) {
    switch (type) {
    case TOK_INT_LIT: return "INT_LIT";
    case TOK_FLOAT_LIT: return "FLOAT_LIT";
    case TOK_STR_LIT: return "STR_LIT";
    case TOK_IDENT: return "IDENT";
    case TOK_FUNC: return "FUNC";
    case TOK_LET: return "LET";
    case TOK_MUT: return "MUT";
    case TOK_UINT: return "UINT";
    case TOK_INT: return "INT";
    case TOK_FLOAT: return "FLOAT";
    case TOK_BOOL: return "BOOL";
    case TOK_STR: return "STR";
    case TOK_PLUS: return "PLUS";
    case TOK_MINUS: return "MINUS";
    case TOK_STAR: return "STAR";
    case TOK_SLASH: return "SLASH";
    case TOK_PERCENT: return "PERCENT";
    case TOK_LPAREN: return "LPAREN";
    case TOK_RPAREN: return "RPAREN";
    case TOK_LBRACE: return "LBRACE";
    case TOK_RBRACE: return "RBRACE";
    case TOK_LBRACKET: return "LBRACKET";
    case TOK_RBRACKET: return "RBRACKET";
    case TOK_DOT: return "DOT";
    case TOK_COMMA: return "COMMA";
    case TOK_EOF: return "EOF";
    case TOK_ERR: return "ERR";
    }
    return "UNKNOWN";
}
