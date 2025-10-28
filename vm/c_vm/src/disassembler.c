#include "../include/disassembler.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

static int32_t read_i32(const u8 *code, size_t size, size_t *ip)
{
    if (*ip + 4 > size)
        return 0;
    int32_t v;
    memcpy(&v, &code[*ip], 4);
    *ip += 4;
    return v;
}

void disassemble_bytecode(const Bytecode *bc, FILE *os)
{
    size_t ip = 0;
    while (ip < bc->code_size)
    {
        u8 op = bc->code[ip++];
        fprintf(os, "%04zu: %02u ", ip - 1, op);
        switch (op)
        {
        case OP_HALT:
            fprintf(os, "OP_HALT\n");
            break;
        case OP_LOAD_CONST:
        {
            int32_t r = read_i32(bc->code, bc->code_size, &ip);
            int32_t ci = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_LOAD_CONST r%d const#%d\n", r, ci);
            break;
        }
        case OP_MOV:
        {
            int32_t d = read_i32(bc->code, bc->code_size, &ip);
            int32_t s = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_MOV r%d r%d\n", d, s);
            break;
        }
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        {
            const char *name = op == OP_ADD ? "ADD" : op == OP_SUB ? "SUB"
                                                  : op == OP_MUL   ? "MUL"
                                                                   : "DIV";
            int32_t dst = read_i32(bc->code, bc->code_size, &ip);
            int32_t a = read_i32(bc->code, bc->code_size, &ip);
            int32_t b = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_%s r%d r%d r%d\n", name, dst, a, b);
            break;
        }
        case OP_PRINT:
        {
            int32_t r = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_PRINT r%d\n", r);
            break;
        }
        case OP_JMP:
        {
            int32_t rel = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_JMP %d\n", rel);
            break;
        }
        case OP_JZ:
        {
            int32_t r = read_i32(bc->code, bc->code_size, &ip);
            int32_t rel = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_JZ r%d %d\n", r, rel);
            break;
        }
        case OP_ALLOC_STR:
        {
            int32_t dst = read_i32(bc->code, bc->code_size, &ip);
            int32_t ci = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_ALLOC_STR r%d const#%d\n", dst, ci);
            break;
        }
        case OP_CALL:
        {
            int32_t fi = read_i32(bc->code, bc->code_size, &ip);
            int32_t nargs = read_i32(bc->code, bc->code_size, &ip);
            int32_t dst = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_CALL f%d nargs=%d dst=r%d\n", fi, nargs, dst);
            break;
        }
        case OP_CALL_USER:
        {
            int32_t ci = read_i32(bc->code, bc->code_size, &ip);
            int32_t nargs2 = read_i32(bc->code, bc->code_size, &ip);
            int32_t dst2 = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_CALL_USER const#%d nargs=%d dst=r%d\n", ci, nargs2, dst2);
            break;
        }
        case OP_RET:
        {
            int32_t r = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_RET r%d\n", r);
            break;
        }
        case OP_THROW:
        {
            int32_t r = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_THROW r%d\n", r);
            break;
        }
        case OP_MK_CLOSURE:
        {
            int32_t dst = read_i32(bc->code, bc->code_size, &ip);
            int32_t ci = read_i32(bc->code, bc->code_size, &ip);
            int32_t nc = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_MK_CLOSURE r%d const#%d ncaptures=%d\n", dst, ci, nc);
            for (int i = 0; i < nc; ++i)
            {
                int32_t r = read_i32(bc->code, bc->code_size, &ip);
                fprintf(os, "    capture r%d\n", r);
            }
            break;
        }
        case OP_CALL_CLOSURE:
        {
            int32_t robj = read_i32(bc->code, bc->code_size, &ip);
            int32_t nargs = read_i32(bc->code, bc->code_size, &ip);
            int32_t dst = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_CALL_CLOSURE robj=r%d nargs=%d dst=r%d\n", robj, nargs, dst);
            break;
        }
        case OP_PUSH_HANDLER:
        {
            int32_t rel = read_i32(bc->code, bc->code_size, &ip);
            fprintf(os, "OP_PUSH_HANDLER %d\n", rel);
            break;
        }
        case OP_POP_HANDLER:
            fprintf(os, "OP_POP_HANDLER\n");
            break;
        default:
            fprintf(os, "UNKNOWN OPCODE %u\n", op);
            break;
        }
    }
}
