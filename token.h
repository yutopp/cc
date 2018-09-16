#ifndef CC_TOKEN_H
#define CC_TOKEN_H

#include <stdio.h>

typedef enum {
    TOK_KIND_EOF,
    TOK_KIND_SHARP,
    TOK_KIND_LT,
    TOK_KIND_GT,
    TOK_KIND_LPAREN,
    TOK_KIND_RPAREN,
    TOK_KIND_LBLOCK,
    TOK_KIND_RBLOCK,
    TOK_KIND_SEMICOLON,
    TOK_KIND_COLON,
    TOK_KIND_DOT,
    TOK_KIND_COMMA,
    TOK_KIND_PLUS,
    TOK_KIND_MINUS,
    TOK_KIND_MUL,
    TOK_KIND_DIV,
    TOK_KIND_ASSIGN,
    TOK_KIND_NOT,
    TOK_KIND_AND,
    TOK_KIND_OR,
    TOK_KIND_LOGICAL_AND,
    TOK_KIND_LOGICAL_OR,
    TOK_KIND_ID,
    TOK_KIND_RETURN,
    TOK_KIND_INT_LIT,
    TOK_KIND_CHAR_LIT,
    TOK_KIND_STRING_LIT,
} TokenKind;

typedef struct {
    TokenKind kind;

    char const* buf_ref;
    size_t pos_begin;
    size_t pos_end;
} Token;

char* token_to_string(Token *tok);
void token_fprint(FILE *fp, Token *tok);
void token_fprint_buf(FILE *fp, Token *tok);

#endif /*CC_TOKEN_H*/
