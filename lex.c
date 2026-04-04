#include "kenc.h"

// print with the following format
// file.zd:10:10: <error message here>
//   10  |    func main(
//       |             ^
static void verror_tok(Token *tok, const char *fmt, va_list ap)
{
    // Print filename:line:col: <msg>
    fprintf(stderr, "%s:%d:%d: ", tok->source->name, tok->line, tok->col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    // Print format
    //  <ln>  |   <line>
    //        |   ^
    fprintf(stderr, "%5d |     %s\n", tok->line, tok->start);
    fprintf(stderr, "%5s |     ", " ");
    // Print carret with token length
    fprintf(stderr, "%.*s", tok->col - 1, " "); // width
    fprintf(stderr, "%.*s\n", (int)tok->length, "^");
}

static void verror_at(File *source, int line_no, const char *loc, const char *fmt,
                      va_list ap)
{
    // Gather start
    const char *line = loc;
    while (source->content < line && line[-1] != '\n')
        line--;

    const char *end = loc;
    while (*end && *end == '\n')
        end++;

    char start[(size_t)(end - line)];
    memcpy(start, line, (size_t)(end - line));
    start[(size_t)(end - line)] = '\0';

    // print char token
    Token char_tok = (Token){
        .type = TOK_ERR,
        .source = source,
        .start = start,
        .length = 1,
        .line = line_no,
        .col = (int)(loc - line) + 1,
    };
    verror_tok(&char_tok, fmt, ap);
}

void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

noreturn static void error_at(Lexer *l, const char *loc, const char *fmt, ...)
{ int line_no = 1;
    for (char *p = l->source->content; p < loc; p++)
        if (*p == '\n')
            line_no++;

    va_list ap;
    va_start(ap, fmt);
    verror_at(l->source, line_no, loc, fmt, ap);
    exit(1);
}

static Token make_token(Lexer *l, TokenType type, const char *start, size_t len)
{
    return (Token){
        .source = l->source,
        .type = type,
        .start = start,
        .length = len,
        .line = l->line,
        .col = l->col,
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
            while (l->pos < l->source->length && (peek(l) != '*' &&
                        peek_next(l) != '/'))
                advance(l);
            // skip */
            advance(l);
            advance(l);
        } else {
            break;
        }
    }
}

static Token lex_number(Lexer *l)
{
    const char *start = l->source->content + l->pos;

    while (isdigit(peek(l)) || (peek(l) == '_' && isdigit(peek_next(l)))) {
        advance(l);
    }
    // Check float
    for (const char *p = start; p < (l->source->content + l->pos); p++) {
        if (*p == '.')
            return make_token(l, TOK_FLOAT_LIT, start, (size_t)((l->source->content + l->pos) - start));
    }

    return make_token(l, TOK_INT_LIT, start, (size_t)((l->source->content + l->pos) - start));
}

static Token lex_string(Lexer *l)
{
    advance(l); // skip opening "
    const char *start = l->source->content + l->pos;

    while (l->pos < l->source->length) {
        if (peek(l) == '"') {
            advance(l); // skip closing "
            // ignore closing "
            return make_token(l, TOK_STR_LIT, start, (size_t)((l->source->content + l->pos) - start) - 1);
        }
        advance(l);
    }

    error_at(l, start, "Unterminated string literal");
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
            {"func", 4, TOK_FUNC},
        };

        for (size_t i = 0; i < sizeof(kw) / sizeof(*kw); i++)
            hashmap_put2(&map, kw[i].kw, kw[i].kw_len, &kw[i]);
    }

    return hashmap_get2(&map, name, len);
}

static Token lex_ident(Lexer *l)
{
    char *start = l->source->content + l->pos;
    while (isalnum(peek(l)) || peek(l) == '_')
        advance(l);

    size_t len = (size_t)((l->source->content + l->pos) - start);
    kwMap *kw = (kwMap*)check_kw(start, len);
    TokenType type = kw ? kw->type : TOK_IDENT;

    return make_token(l, type, start, len);
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

    const char *start = l->source->content + l->pos;
    advance(l); // skip c

    switch (c) {
    case '(':
        return make_token(l, TOK_LPAREN, start, 1);
    case ')':
        return make_token(l, TOK_RPAREN, start, 1);
    case '{':
        return make_token(l, TOK_LBRACE, start, 1);
    case '}':
        return make_token(l, TOK_RBRACE, start, 1);
    case '[':
        return make_token(l, TOK_LBRACKET, start, 1);
    case ']':
        return make_token(l, TOK_RBRACKET, start, 1);
    case ',':
        return make_token(l, TOK_COMMA, start, 1);
    case '+':
        return make_token(l, TOK_PLUS, start, 1);
    case '-':
        return make_token(l, TOK_MINUS, start, 1);
    case '*':
        return make_token(l, TOK_STAR, start, 1);
    case '/':
        return make_token(l, TOK_SLASH, start, 1);
    case '%':
        return make_token(l, TOK_PERCENT, start, 1);
    }

    error_at(l, start, "Unexpected character");
}

void lex_init(Lexer *l, File *file)
{
    l->source = file;
    l->pos = 0;
    l->line = 0;
    l->col = 0;
}

Token *lex_tokenize(Lexer *l, int *count)
{
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

// Print array of tokens for debugging
// will not removed, use macro instead
#ifndef NDEBUG
void print_tokens(Token *tokens, int count)
{
    int top_line = 1; // biggest line
    for (int i = 0; i < count; i++) {
        if (tokens[i].line > top_line) {
            top_line = tokens[i].line;
            printf("%s:%d\n", tokens[i].source->name, top_line);
        }
        printf("  %-12s '%.*s'\n", token_type_name(tokens[i].type), (int)tokens[i].length,
            tokens[i].start);
    }
}
#endif

const char *token_type_name(TokenType type) {
    switch (type) {
    case TOK_INT_LIT: return "INT_LIT";
    case TOK_FLOAT_LIT: return "FLOAT_LIT";
    case TOK_STR_LIT: return "FLOAT_LIT";
    case TOK_IDENT: return "IDENT";
    case TOK_FUNC: return "FUNC";
    case TOK_LET: return "LET";
    case TOK_MUT: return "MUT";
    case TOK_UINT: return "UINT";
    case TOK_INT: return "INT";
    case TOK_FLOAT: return "FLOAT";
    case TOK_VOID: return "VOID";
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
    case TOK_EOF: return "EOF";
    case TOK_ERR: return "ERR";
    default: return "UNKNOWN";
    }
}
