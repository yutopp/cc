#ifndef CC_PARSER_H
#define CC_PARSER_H

#include "token.h"
#include "vector.h"
#include "arena.h"

struct parser_t;
typedef struct parser_t Parser;

enum ParserResult {
    PARSER_OK,
    PARSER_ERROR,
};
typedef size_t NodeId;
typedef struct parser_result_t {
    enum ParserResult result;
    NodeId id;
    const char* message;
} ParserResult;

Parser* parser_new(Vector* tokens, Arena* arena);
void parser_drop(Parser *parser);

ParserResult parser_parse();

#endif /* CC_PARSER_H */
