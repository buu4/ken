#include "ken.h"

noreturn void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", ken_progname);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static void verror_tok(Token *tok, const char *fmt, va_list ap)
{ // print with the following format
  // file.ken:10:6: <error message here>
  //    10 |     func main(
  //       |          ^^^^

    const char *start = tok->loc;
    const char *end = tok->loc;

    while (tok->source->content < start && start[-1] != '\n')
        start--;

    while (*end && *end != '\n')
        end++;

    // Print filename:line:col: <msg>
    fprintf(stderr, "%s:%d:%d: ", tok->source->name, tok->line, tok->col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    // Print line context
    fprintf(stderr, "%5d |     %.*s\n", tok->line, (int)(end - start), start);
    fprintf(stderr, "%5s |     ", " ");
    // Print carret with token length
    for (int i = 1; i < tok->col; i++)
        fprintf(stderr, " ");
    for (int i = 0; i < tok->length; i++)
        fprintf(stderr, "^");

    fprintf(stderr, "\n");
}

void error_tok(Token *tok, const char *fmt, ...)
{ // print error when tokens is ready
  // commonly used by parser/semantic analysis

    va_list ap;
    va_start(ap, fmt);
    verror_tok(tok, fmt, ap);
    va_end(ap);
}

noreturn void error_at(Lexer *l, const char *loc, const char *fmt, ...)
{ // print error when tokens are not ready yet
  // specialy for lexer & character only

    const char *start = loc;
    int line = 1;

    for (char *p = l->source->content; p < loc; p++)
        if (*p == '\n')
            line++;

    while (l->source->content < start && start[-1] != '\n')
        start--;

    Token char_tok = (Token){
        .type = TOK_ERR,
        .source = l->source,
        .loc = loc,
        .length = 1,
        .line = line,
        .col = (int)(loc - start) + 1,
    };

    va_list ap;
    va_start(ap, fmt);
    verror_tok(&char_tok, fmt, ap);
    va_end(ap);
    exit(1);
}
