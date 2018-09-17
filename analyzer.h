#ifndef CC_ANALYZER_H
#define CC_ANALYZER_H

#include "node.h"

struct analyzer_t;
typedef struct analyzer_t Analyzer;

Analyzer* analyzer_new();
void analyzer_drop(Analyzer* a);

void analyzer_analyze(Analyzer* a, Node* node);
int analyzer_success(Analyzer* a);

#endif /*CC_ANALYZER_H*/
