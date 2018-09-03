#ifndef CC_LEXER_H
#define CC_LEXER_H

#include "token.h"

struct lexer_t;
typedef struct lexer_t Lexer;

Lexer* lexer_new(const char *buffer, const char* filepath);
void lexer_delete(Lexer *lex);

Token lexer_read(Lexer* lex);

#endif /*CC_LEXER_H*/
