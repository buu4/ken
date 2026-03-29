#include "kenc.h"

// print with the following format
// file.zd:10:10: <error message here>
//   10  |    func main(
//       |             ^
static void verror_tok(Token *tok, const char *fmt, va_list ap)
{
    // Print filename:line:col: <msg>
    fprintf(stderr, "%s:%d:%d: ", tok->file->name, tok->line, tok->col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    // Print format
    //  <ln>  |   <line>
    //        |   ^
    fprintf(stderr, "%-5d |    %s\n");
    fprintf(stderr, "%-5s |    ", " ");
    // Print carret with token length
    fprintf(stderr, "%.*s", tok->col, " "); // width
    fprintf(stderr, "%.*s", tok->length, "^");
    fprintf(stderr, "\n");
}

static void verror_at(const char *filename, const char *input, int line_no,
                      const char *loc, const char *fmt, va_list ap)
{
    // Gather start
    char *line = loc;
    while (input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end && *end == '\n')
        end++;

    char *start;
    memcpy(start, line, (size_t)(end - line));
    start[(size_t)(end - line)] = '\0';

    // print char token
    Token char_tok = (Token){
        .start = start,
        .length = 1;
        .line = line_no,
        .col = (int)(loc - line) + 1,
    }
    verror_tok(char_tok, fmt, ap);
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
{
    int line_no = 1;
    for (char *p = l->source->content; p < loc; p++)
        if (*p == '\n')
            line_no++;

    va_list ap;
    va_start(ap, fmt);
    verror_at(l->source->name, l->source->content, line_no,
            loc, fmt, ap);
    exit(1);
}

static Token make_token(Lexer *l, TokenType type, const char *start, size_t len)
{
    return (Token){
        .type = type,
        .start = start,
        .length = len,
        .line = l->line,
        .col = l->col,
    }
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

Token *lex_number(Lexer *l)
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

Token *lex_next(Lexer *l)
{
    skip_whitespace(l);
    if (l->pos >= l->source->length)
        return make_token(l, TOK_EOF, l->source->content + l->pos, 0);

    char c = peek(l);

    if (is_digit(c) || (c == '-' && is_digit(peek_at(l, 1))))
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
    }

    error_at(l, l->source->content + l->pos, "Unexpected character");
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
    Token tokens = malloc(sizeof(Token) * cap);

    while (1) {
        tokens[n] = lex_next(l);
        if (tokens[n].type == TOK_EOF) {
            n++;
            break;
        }
        n++:
    }
    *count = n;
    return tokens;
}

const char *token_type_name(TokenType type)
{
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
    case TOK_FLOAT: return "TOK_FLOAT";
    case TOK_VOID: return "TOK_VOID";
    case TOK_BOOL: return "TOK_BOOL";
    case TOK_STR: return "TOK_STR";
    case TOK_PLUS: return "TOK_PLUS";
    case TOK_MINUS: return "TOK_MINUS";
    case TOK_STAR: return "TOK_STAR";
    case TOK_SLASH: return "TOK_SLASH";
    case TOK_PERCENT: return "TOK_PERCENT";
    case TOK_LPAREN: return "TOK_LPAREN";
    case TOK_RPAREN: return "TOK_RPAREN";
    case TOK_LBRACE: return "TOK_LBRACE";
    case TOK_RBRACE: return "TOK_RBRACE";
    case TOK_EOF: return "TOK_EOF";
    default: return "UNKNOWN";
    }
}
