#ifndef CC_H
#define CC_H

#include "parser.h"
#include "analyzer.h"

struct cc_t;
typedef struct cc_t CC;

CC* cc_new(char const* buffer, char const* fpath);
void cc_drop(CC* cc);

ParserResult cc_parse(CC* cc);
void cc_analyze(CC* cc, Node* node);
int cc_compile(CC* cc, Node* node);

#endif /* CC_H */
