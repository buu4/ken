#include "ken.h"

public char *ken_progname = "ken";

int main(int argc, char **argv)
{ ken_progname = argv[0];
    if (argc == 1) {
        error("error: input file required");
    }

    File file;
    Lexer lexer;

    file_init(&file, argv[1]);
    lex_init(&lexer, &file);

    int count;
    Token *tokens = lex_tokenize(&lexer, &count);

    print_tokens(tokens, count);
    file_close(&file);
}
