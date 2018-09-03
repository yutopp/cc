#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

char* read_all(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *fcontent = malloc(size+1);
    fread(fcontent, sizeof(char), size, fp);
    fcontent[size] = '\0';

    return fcontent;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "argc: %d shoule be 2", argc);
        return 1;
    }

    char *fpath = argv[1];

    printf("%s\n", fpath);
    FILE *fp = fopen(fpath, "rb");
    if (fp == NULL) {
        return 1;
    }

    char *fcontent = read_all(fp);
    fclose(fp);

    printf("%s\n", fcontent);

    Lexer* lex = lexer_new(fcontent, fpath);

    for(;;) {
        Token tok = lexer_read(lex);
        token_fprint(stdout, &tok);
        printf("\n");
        if (tok.kind == TOK_KIND_EOF) {
            break;
        }
    }

    lexer_delete(lex);

    free(fcontent);

    return 0;
}
