#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

void token_fprint(FILE *fp, Token *tok) {
    switch(tok->kind) {
    case TOK_KIND_SHARP:
        fprintf(fp, "#");
        break;
    case TOK_KIND_LT:
        fprintf(fp, "<");
        break;
    case TOK_KIND_GT:
        fprintf(fp, ">");
        break;
    case TOK_KIND_DOT:
        fprintf(fp, ".");
        break;
    case TOK_KIND_ID:
        fprintf(fp, "ID");
        break;
    case TOK_KIND_EOF:
        fprintf(fp, "EOF");
        break;
    }

    size_t range_size = tok->pos_end - tok->pos_begin;
    char* range = malloc(sizeof(char) * (range_size + 1));
    memcpy(range, tok->buf_ref + tok->pos_begin, range_size);
    range[range_size] = '\0';

    fprintf(fp, " '%s' (%ld, %ld)", range, tok->pos_begin, tok->pos_end);

    free(range);
}
