#ifndef CC_NODE_H
#define CC_NODE_H

#include "vector.h"
#include "token.h"
#include "arena.h"

struct node_t;
typedef struct node_t Node;

typedef enum {
    NODE_TRANS_UNIT,
    NODE_FUNC_DEF,
    NODE_STMT_COMPOUND,
    NODE_STMT_EXPR,
    NODE_STMT_IF,
    NODE_STMT_JUMP,
    NODE_EXPR_BIN, // TODO: Rename to NODE_EXPR_BINARY
    NODE_EXPR_POSTFIX,
    NODE_LIT_INT,
    NODE_LIT_STRING,
    NODE_ID,
    NODE_ARGS_LIST,
    NODE_DECLARATOR,
    NODE_DIRECT_DECLARATOR,
    NODE_PARAM_DECL,
    NODE_PARAM_LIST,
} NodeKind;

typedef enum {
    NODE_DIRECT_DECLARATOR_KIND_BASE,
    NODE_DIRECT_DECLARATOR_KIND_PARAM_TYPE_LIST,
} NodeDirectDeclaratorKind;

typedef enum {
    NODE_EXPR_POSTFIX_KIND_FUNC_CALL,
} NodeExprPostfixKind;

typedef union {
    struct {
        Vector* decls; // Vector<Node*>
    } trans_unit;
    struct {
        Node* decl_spec;
        Node* decl;
        Node* block;
    } func_def;
    struct {
        Vector* stmts; // Vector<Node*>
    } stmt_compound;
    struct {
        Node* expr; // Nullable
    } stmt_expr;
    struct {
        Node* cond;
        Node* then_b;
        Node* else_b; // Nullable
    } stmt_if;
    struct {
        TokenKind kind; // TODO: Change to Token*
        Node* expr;
    } stmt_jump;
    struct {
        Token* op;
        Node* lhs;
        Node* rhs;
    } expr_bin;
    struct {
        NodeExprPostfixKind kind;
        Node* lhs;
        Node* rhs; // Nullable
    } expr_postfix;
    struct {
        int v;
    } lit_int;
    struct {
        char const* v;
    } lit_string;
    struct {
        Token* tok;
    } id;
    struct {
        Vector* args; // Vector<Node*>
    } args_list;
    struct {
        Node* node;
    } declarator;
    struct {
        NodeDirectDeclaratorKind kind;
        Node* base;
        Node* args[2];
    } direct_declarator;
    struct {
        Node* spec;
        Node* decl; // Nullable
    } param_decl;
    struct {
        Vector* params; // Vector<Node*>
    } param_list;
} NodeValue;

struct node_t {
    NodeKind kind;
    NodeValue value;
};

void node_destruct(Node* node);
void node_fprint(FILE *fp, Node* node);

Token* node_declarator_extract_id_token(Node* node);

#endif /* CC_PARSER_H */
