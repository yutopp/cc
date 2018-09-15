#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "vector.h"

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

    Vector* tokens = vector_new(sizeof(Token));
    for(;;) {
        Token tok = lexer_read(lex);
        token_fprint(stdout, &tok);
        printf("\n");
        if (tok.kind == TOK_KIND_EOF) {
            break;
        }

        Token* elem = (Token*)vector_append(tokens);
        assert(elem);
        *elem = tok;
    }

    lexer_delete(lex);

    Arena* nodes = arena_new();
    Parser* parser = parser_new(tokens, nodes);

    ParserResult res = parser_parse(parser);
    if (res.result == PARSER_ERROR) {
        fprintf(stderr, "ERROR: ");
        parser_fprint_error(stderr, &res.error);
        fprintf(stderr, "\n");

        goto exit;
    }
    assert(res.value.node);

    printf("\n");
    printf("= AST =\n");
    node_fprint(stdout, res.value.node);

exit:
    parser_drop(parser);
    arena_drop(nodes);
    vector_drop(tokens);

    free(fcontent);

    return 0;
}
