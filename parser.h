#ifndef CC_PARSER_H
#define CC_PARSER_H

#include "token.h"
#include "vector.h"
#include "node_arena.h"
#include "node.h"

struct parser_t;
typedef struct parser_t Parser;

enum ParserResult {
    PARSER_OK,
    PARSER_ERROR,
};

typedef enum {
    PARSER_ERROR_KIND_EOF,
    PARSER_ERROR_KIND_UNEXPECTED,
    PARSER_ERROR_KIND_MORE1,
} ParserErrorKind;

typedef union {
    struct {
        Token* token;
    } unexpected;
} ParserErrorValue;

typedef struct {
    ParserErrorKind kind;
    ParserErrorValue value;
} ParseError;

typedef struct parser_result_t {
    enum ParserResult result;
    ParseError error; // TODO: Move error into the union which is named "value"
    union {
        Node* node;
        Vector* nodes; // Vector<Node*>
        Token* token;
    } value;
} ParserResult;

Parser* parser_new(Vector* tokens, NodeArena* arena);
void parser_drop(Parser *parser);

ParserResult parser_parse();

void parser_fprint_error(FILE *fp, ParseError *err);

#endif /* CC_PARSER_H */
