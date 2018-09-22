#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cc.h"
#include "vector.h"
#include "log.h"
#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "ir.h"
#include "asm_x86_64.h"

struct cc_t {
    char const* buffer;
    char const* fpath;
    Vector* tokens;         // phase1, Vector<Token>
    NodeArena* nodes;       // phase1
    Parser* parser;         // phase1
    TypeArena* types;       // phase2
    Analyzer* analyzer;     // phase2
    IRBuilder* ir_builder;  // phase3
    IRModule* ir_mod;       // phase3
    enum {
        CC_STATE_INIT,
    } state;
};

CC* cc_new(char const* buffer, char const* fpath) {
    CC* cc = malloc(sizeof(CC));
    cc->buffer = buffer;
    cc->fpath = fpath;
    cc->tokens = NULL;
    cc->nodes = NULL;
    cc->parser = NULL;
    cc->types = NULL;
    cc->analyzer = NULL;
    cc->ir_builder = NULL;
    cc->ir_mod = NULL;
    cc->state = CC_STATE_INIT;

    return cc;
}

void cc_drop(CC* cc) {
    if (cc->ir_mod) {
        ir_module_drop(cc->ir_mod);
    }
    if (cc->ir_builder) {
        ir_builder_drop(cc->ir_builder);
    }

    if (cc->analyzer) {
        analyzer_drop(cc->analyzer);

        assert(cc->types);
        type_arena_drop(cc->types);
    }

    if (cc->parser) {
        parser_drop(cc->parser);

        assert(cc->nodes);
        node_arena_drop(cc->nodes);
    }

    if (cc->tokens) {
        vector_drop(cc->tokens);
    }

    free(cc);
}

static void cc_lex(CC* cc) {
    cc->tokens = vector_new(sizeof(Token)); // Vector<Token>

    Lexer* lex = lexer_new(cc->buffer, cc->fpath);
    for(;;) {
        Token tok = lexer_read(lex);

        token_fprint(DEBUGOUT, &tok); printf("\n");

        Token* elem = (Token*)vector_append(cc->tokens);
        assert(elem);
        *elem = tok;

        if (tok.kind == TOK_KIND_EOF) {
            break;
        }
    }
    lexer_delete(lex);
}

ParserResult cc_parse(CC* cc) {
    cc_lex(cc); // TODO: pass lexer to parser

    cc->nodes = node_arena_new();
    cc->parser = parser_new(cc->tokens, cc->nodes);

    return parser_parse(cc->parser);
}

// TODO: returns AnalyzerResult
void cc_analyze(CC* cc, Node* node) {
    cc->types = type_arena_new();
    cc->analyzer = analyzer_new(cc->types);

    analyzer_analyze(cc->analyzer, node);
}

static IRModule* cc_build_ir(CC* cc, Node* node) {
    cc->ir_builder = ir_builder_new();
    cc->ir_mod = ir_builder_new_module(cc->ir_builder, node);

    fprintf(DEBUGOUT,"= IR =\n");
    ir_module_fprint(DEBUGOUT, cc->ir_mod);
    fprintf(DEBUGOUT, "\n");
    fflush(DEBUGOUT);

    return cc->ir_mod;
}

// TODO: returns CompileResult
int cc_compile(CC* cc, Node* node) {
    int err = 0;
    IRModule* ir_mod = cc_build_ir(cc, node);

    ASM_X86_64* asm_x86_64 = asm_x86_64_new(ir_mod);

    fprintf(DEBUGOUT, "= ASM =\n");
    asm_x86_64_fprint(DEBUGOUT, asm_x86_64);
    fprintf(DEBUGOUT, "\n");
    fflush(DEBUGOUT);

    // TODO: fix
    char spath[16] = "/tmp/ccXXXXXX.s";
    int fd = mkstemps(spath, 2);
    if (fd == -1) {
        // TODO: fix
        fprintf(stderr, "Failed to open asm file: \n");
        err = 1;
        goto exit;
    }
    FILE* fp = fdopen(fd, "wb");
    asm_x86_64_fprint(fp, asm_x86_64);
    fclose(fp);

    char opath[16];
    memcpy(opath, spath, 16);
    opath[14] = 'o';

    char cmd[1024];
    int n = snprintf(cmd, 1024, "as %s -o %s", spath, opath);
    if (n < 0 || n >= 1024) {
        // TODO: fix
        fprintf(stderr, "Failed to write cmd: \n");
        err = 1;
        goto exit;
    }
    int cmd_res = system(cmd);
    if (cmd_res != 0) {
        // TODO: fix
        fprintf(stderr, "FAILED: as");
        err = 1;
        goto exit;
    }

    n = snprintf(cmd, 1024, "gcc %s -o a.out", opath);
    if (n < 0 || n >= 1024) {
        // TODO: fix
        fprintf(stderr, "Failed to write cmd: \n");
        err = 1;
        goto exit;
    }
    cmd_res = system(cmd);
    if (cmd_res != 0) {
        // TODO: fix
        fprintf(stderr, "FAILED: gcc");
        err = 1;
        goto exit;
    }

exit:
    asm_x86_64_drop(asm_x86_64);
    return err;
}
