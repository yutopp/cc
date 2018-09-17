#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "analyzer.h"
#include "asm_x86_64.h"
#include "vector.h"

char* read_all(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *fcontent = malloc(size+1);
    fread(fcontent, sizeof(char), size, fp);
    fcontent[size] = '\0';

    return fcontent;
}

int main(int argc, char *argv[]) {
    int exit_code = 1; // Default error

    if (argc != 2) {
        fprintf(stderr, "argc: %d shoule be 2", argc);
        goto exit_phase0;
    }

    char *fpath = argv[1];

    printf("%s\n", fpath);
    FILE *fp = fopen(fpath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: ");
        goto exit_phase0;
    }

    char *fcontent = read_all(fp);
    fclose(fp);

    printf("%s\n", fcontent);

    Lexer* lex = lexer_new(fcontent, fpath);

    Vector* tokens = vector_new(sizeof(Token)); // Vector<Token>
    for(;;) {
        Token tok = lexer_read(lex);
        token_fprint(stdout, &tok);
        printf("\n");

        Token* elem = (Token*)vector_append(tokens);
        assert(elem);
        *elem = tok;

        if (tok.kind == TOK_KIND_EOF) {
            break;
        }
    }

    lexer_delete(lex);

    NodeArena* nodes = node_arena_new();
    Parser* parser = parser_new(tokens, nodes);

    ParserResult res = parser_parse(parser);
    if (res.result == PARSER_ERROR) {
        fprintf(stderr, "ERROR: ");
        parser_fprint_error(stderr, &res.error);
        fprintf(stderr, "\n");

        goto exit_phase1;
    }
    assert(res.value.node);

    printf("\n");
    printf("= AST =\n");
    node_fprint(stdout, res.value.node);

    Analyzer* analyzer = analyzer_new();
    analyzer_analyze(analyzer, res.value.node);
    if (!analyzer_success(analyzer)) {
        fprintf(stderr, "ERROR: analyzer");

        goto exit_phase2;
    }

    IRBuilder* ir_builder = ir_builder_new();

    IRModule* ir_mod = ir_builder_new_module(ir_builder, res.value.node);

    printf("\n");
    printf("= IR =\n");
    ir_module_fprint(stdout, ir_mod);

    ASM_X86_64* asm_x86_64 = asm_x86_64_new(ir_mod);

    printf("\n");
    printf("= ASM =\n");
    asm_x86_64_fprint(stdout, asm_x86_64);
    fflush(stdout);

    // TODO: fix
    FILE *fp_asm = fopen("_cc_out.s", "wb");
    if (fp_asm == NULL) {
        fprintf(stderr, "Failed to open asm file: ");

        goto exit_phaseN;
    }
    asm_x86_64_fprint(fp_asm, asm_x86_64);
    fflush(fp_asm);
    fclose(fp_asm);

    int cmd_res = system("as _cc_out.s -o _cc_out.o");
    if (cmd_res != 0) {
        fprintf(stderr, "FAILED: as");

        goto exit_phaseN;
    }

    cmd_res = system("gcc _cc_out.o -o _cc_out.out");
    if (cmd_res != 0) {
        fprintf(stderr, "FAILED: gcc");

        goto exit_phaseN;
    }

    exit_code = 0; // Success

exit_phaseN:
    asm_x86_64_drop(asm_x86_64);

    ir_module_drop(ir_mod);
    ir_builder_drop(ir_builder);

exit_phase2:
    analyzer_drop(analyzer);

exit_phase1:
    parser_drop(parser);
    node_arena_drop(nodes);

    vector_drop(tokens);

    free(fcontent);

exit_phase0:
    return exit_code;
}
