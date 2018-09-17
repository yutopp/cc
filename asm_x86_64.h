#ifndef CC_ASM_X86_64_H
#define CC_ASM_X86_64_H

#include <stdio.h>
#include "ir.h"

struct asm_x86_64_t;
typedef struct asm_x86_64_t ASM_X86_64;

struct asm_x86_64_inst_t;
typedef struct asm_x86_64_inst_t ASM_X86_64_Inst;

ASM_X86_64* asm_x86_64_new(IRModule* mod);
void asm_x86_64_drop(ASM_X86_64 *a);

void asm_x86_64_fprint(FILE* fp, ASM_X86_64* a);

#endif /*CC_ASM_X86_64_H*/
