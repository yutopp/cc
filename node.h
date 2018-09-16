#ifndef CC_NODE_H
#define CC_NODE_H

#include "vector.h"
#include "token.h"
#include "arena.h"

struct node_t;
typedef struct node_t Node;

typedef enum {
    NODE_FUNC_DEF,
    NODE_STMT_COMPOUND,
    NODE_STMT_JUMP,
    NODE_EXPR_BIN,
    NODE_LIT_INT,
    NODE_ID,
} NodeKind;

typedef union {
    struct {
        Node* decl_spec;
        Node* decl;
        Node* block;
    } func_def;
    struct {
        Vector* stmts; // Vector<Node*>
    } stmt_compound;
    struct {
        TokenKind kind;
        Node* expr;
    } stmt_jump;
    struct {
        Token* op;
        Node* lhs;
        Node* rhs;
    } expr_bin;
    struct {
        int v;
    } lit_int;
    struct {
        Token* tok;
    } id;
} NodeValue;

struct node_t {
    NodeKind kind;
    NodeValue value;
};

void node_destruct(Node* node);
void node_fprint(FILE *fp, Node* node);

#endif /* CC_PARSER_H */
