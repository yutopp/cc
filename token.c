#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

char* token_to_string(Token *tok) {
    size_t range_size = tok->pos_end - tok->pos_begin;
    char* range = malloc(sizeof(char) * (range_size + 1));
    memcpy(range, tok->buf_ref + tok->pos_begin, range_size);
    range[range_size] = '\0';

    return range;
}

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
    default:
        fprintf(fp, "Kind(%d)", tok->kind);
        break;
    }

    fprintf(fp, " ");
    token_fprint_buf(fp, tok);
    fprintf(fp, " (%ld, %ld)", tok->pos_begin, tok->pos_end);
}

void token_fprint_buf(FILE *fp, Token *tok) {
    char const* range = token_to_string(tok);
    fprintf(fp, "%s", range);
    free((char*)range);
}
