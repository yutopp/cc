#ifndef CC_NODE_H
#define CC_NODE_H

#include "vector.h"
#include "token.h"

struct node_t;

typedef enum {
    NODE_FUNC_DEF,
    NODE_STMT_COMPOUND,
    NODE_STMT_JUMP,
    NODE_LIT_INT,
    NODE_ID,
} NodeKind;

typedef union {
    struct {
        struct node_t* decl_spec;
        struct node_t* decl;
        struct node_t* block;
    } func_def;
    struct {
        Vector* stmts; // Vector<Node*>
    } stmt_compound;
    struct {
        TokenKind kind;
        struct node_t* expr;
    } stmt_jump;
    struct {
        int v;
    } lit_int;
    struct {
        Token* tok;
    } id;
} NodeValue;

typedef struct node_t {
    NodeKind kind;
    NodeValue value;
} Node;

void node_drop(Node* node);
void node_fprint(FILE *fp, Node* node);

#endif /* CC_PARSER_H */
