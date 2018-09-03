#include <stdio.h>
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
    }

    fprintf(fp, " (%ld, %ld)", tok->pos_begin, tok->pos_end);
}
