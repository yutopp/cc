#ifndef CC_TYPE_H
#define CC_TYPE_H

struct type_t;
typedef struct type_t Type;

enum type_kind_t;
typedef enum type_kind_t TypeKind;

union type_value_t;
typedef union type_value_t TypeValue;

enum type_kind_t {
    TYPE_KIND_INT,
    TYPE_KIND_PTR,
};

union type_value_t {
    struct {
        int bits; // undefined, if bits == -1
    } int_;
    struct {
        Type* inner;
    } ptr;
};

struct type_t {
    TypeKind kind;
    TypeValue value;
};

void type_destruct(Type* ty);

#endif /*CC_TYPE_H*/
