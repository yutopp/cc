#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"

struct lexer_t {
    const char *buffer;
    size_t current_pos;
    size_t begin_pos;
};

static char current(Lexer* lex) {
    return lex->buffer[lex->current_pos];
}

static void skip(Lexer* lex) {
    lex->current_pos++;
}

static Token make_token(Lexer* lex, Kind kind) {
    Token tok = {
        .kind = kind,
        .pos_begin = lex->begin_pos,
        .pos_end = lex->current_pos,
    };
    return tok;
}

Lexer* lexer_new(const char *buffer, const char* filepath) {
    Lexer* lex = (Lexer*)malloc(sizeof(Lexer));
    lex->buffer = buffer;
    lex->current_pos = 0;
    lex->begin_pos = -1;

    return lex;
}

void lexer_delete(Lexer *lex) {
    free(lex);
}

static Token read_id(Lexer* lex);
static Token read_number_lit(Lexer* lex);
static Token read_char_lit(Lexer* lex);
static Token read_string_lit(Lexer* lex);

Token lexer_read(Lexer* lex) {
    for(;;) {
        lex->begin_pos = lex->current_pos;

        char c0 = current(lex);
        switch(c0) {
        case ' ':
        case '\t':
        case '\n':
            skip(lex);
            continue;

        case 'A' ... 'z':
            skip(lex);
            return read_id(lex);

        case '0' ... '9':
            skip(lex);
            return read_number_lit(lex);

        case '"':
            skip(lex);
            return read_string_lit(lex);

        case '\'':
            skip(lex);
            return read_char_lit(lex);

        case '#':
            skip(lex);
            return make_token(lex, TOK_KIND_SHARP);

        case '>':
            skip(lex);
            return make_token(lex, TOK_KIND_GT);

        case '<':
            skip(lex);
            return make_token(lex, TOK_KIND_LT);

        case '(':
            skip(lex);
            return make_token(lex, TOK_KIND_LPAREN);

        case ')':
            skip(lex);
            return make_token(lex, TOK_KIND_RPAREN);

        case '{':
            skip(lex);
            return make_token(lex, TOK_KIND_LBLOCK);

        case '}':
            skip(lex);
            return make_token(lex, TOK_KIND_RBLOCK);

        case ';':
            skip(lex);
            return make_token(lex, TOK_KIND_SEMICOLON);

        case ':':
            skip(lex);
            return make_token(lex, TOK_KIND_COLON);

        case '.':
            skip(lex);
            return make_token(lex, TOK_KIND_DOT);

        case ',':
            skip(lex);
            return make_token(lex, TOK_KIND_COMMA);

        case '+':
            skip(lex);
            return make_token(lex, TOK_KIND_PLUS);

        case '-':
            skip(lex);
            return make_token(lex, TOK_KIND_MINUS);

        case '*':
            skip(lex);
            return make_token(lex, TOK_KIND_MUL);

        case '/':
            skip(lex);
            return make_token(lex, TOK_KIND_DIV);

        case '=':
            skip(lex);
            return make_token(lex, TOK_KIND_ASSIGN);

        case '!':
            skip(lex);
            return make_token(lex, TOK_KIND_NOT);

        case '&':
            skip(lex);
            return make_token(lex, TOK_KIND_AND);

        case '|':
            skip(lex);
            return make_token(lex, TOK_KIND_OR);

        case '\0':
            return make_token(lex, TOK_KIND_EOF);

        default:
            fprintf(stderr, "Unexpected token: '%c'", c0);
            exit(1);
        }
    }

    // unreachable
}

void print_token(FILE *fp, Token *tok) {
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

static Token read_id(Lexer* lex) {
    for(;;) {
        char c0 = current(lex);

        if ((c0 >= 'A' && c0 <= 'z') || (c0 >= '0' && c0 <= '9')) {
            skip(lex);
            continue;
        }

        return make_token(lex, TOK_KIND_ID);
    }
}

static Token read_number_lit(Lexer* lex) {
    for(;;) {
        char c0 = current(lex);

        if (c0 >= '0' && c0 <= '9') {
            skip(lex);
            continue;
        }

        return make_token(lex, TOK_KIND_INT_LIT);
    }
}

static Token read_char_lit(Lexer* lex) {
    for(;;) {
        char c0 = current(lex);
        switch(c0) {
        case '\'':
        {
            Token tok = make_token(lex, TOK_KIND_CHAR_LIT);
            skip(lex);
            return tok;
        }

        default:
            skip(lex);
            continue;
        }
    }
}


static Token read_string_lit(Lexer* lex) {
    for(;;) {
        char c0 = current(lex);
        switch(c0) {
        case '"':
        {
            Token tok = make_token(lex, TOK_KIND_STRING_LIT);
            skip(lex);
            return tok;
        }

        default:
            skip(lex);
            continue;
        }
    }
}
