#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "analyzer.h"
#include "asm_x86_64.h"
#include "vector.h"
#include "cc.h"

static char* read_all(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *fcontent = malloc(size+1);
    fread(fcontent, sizeof(char), size, fp);
    fcontent[size] = '\0';

    return fcontent;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "argc: %d shoule be 2", argc);
        return 1;
    }

    int exit_code = 1;
    char *fpath = argv[1];

    printf("C => %s\n", fpath);
    FILE *fp = fopen(fpath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: ");
        goto exit_0;
    }

    char *fcontent = read_all(fp);
    fclose(fp);

    printf("%s\n", fcontent);

    CC* cc = cc_new(fcontent, fpath);

    ParserResult res = cc_parse(cc);
    if (res.result == PARSER_ERROR) {
        fprintf(stderr, "ERROR: ");
        parser_fprint_error(stderr, &res.error);
        fprintf(stderr, "\n");
        goto exit;
    }
    assert(res.value.node);

    printf("= AST =\n");
    node_fprint(stdout, res.value.node);
    printf("\n");

    /*AnalyzerResult res = */cc_analyze(cc, res.value.node);
    /* TODO: error check */

    int err =
    /*CompileResult res = */cc_compile(cc, res.value.node);
    if (err) {
        goto exit;
    }

    exit_code = 0; // Success

exit:
    cc_drop(cc);

exit_0:
    free(fcontent);

    return exit_code;
}
