#include <stdlib.h>
#include <stdio.h> // TODO: remove
#include "parser.h"
#include "vector.h"
#include "token.h"

struct parser_t {
    Vector* tokens;
    Arena* arena;
};

Parser* parser_new(Vector* tokens, Arena* arena) {
    Parser* p = (Parser*)malloc(sizeof(Parser));
    if (!p) {
        return 0;
    }
    p->tokens = tokens;
    p->arena = arena;

    return p;
}

void parser_drop(Parser *parser) {
    if (!parser) {
        return;
    }

    free(parser);
}

ParserResult parser_parse(Parser *parser);
ParserResult parse_transition_unit(Parser *parser);
ParserResult parse_external_declaration(Parser *parser);
ParserResult parse_function_definition(Parser *parser);

ParserResult parser_parse(Parser *parser) {
    return parse_transition_unit(parser);
}

ParserResult parse_transition_unit(Parser *parser) {
    return parse_external_declaration(parser);
}

ParserResult parse_external_declaration(Parser *parser) {
    return parse_function_definition(parser);
}

ParserResult parse_function_definition(Parser *parser) {
    ParserResult ret;

    Token* t = (Token*)vector_at(parser->tokens, 0);
    if (!t) {
        ret.result = PARSER_ERROR;
        return ret;
    }

    token_fprint(stdout, t);

    ret.result = PARSER_OK;
    return ret;
}
