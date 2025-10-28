#include "../include/verifier.h"
#include <stdio.h>
#include <string.h>

const char *verify_bytecode(const Bytecode *bc)
{
    size_t ip = 0;
    while (ip < bc->code_size)
    {
        u8 op = bc->code[ip++];
        switch (op)
        {
        case OP_HALT:
            break;
        case OP_LOAD_CONST:
            ip += 8;
            break; /* reg + const idx */
        case OP_MOV:
            ip += 8;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            ip += 12;
            break;
        case OP_PRINT:
            ip += 4;
            break;
        case OP_JMP:
            ip += 4;
            break;
        case OP_JZ:
            ip += 8;
            break;
        case OP_ALLOC_STR:
            ip += 8;
            break;
        case OP_CALL:
            ip += 12;
            break;
        case OP_CALL_USER:
            ip += 12;
            break;
        case OP_RET:
            ip += 4;
            break;
        case OP_THROW:
            ip += 4; /* register operand */
            break;
        case OP_PUSH_HANDLER:
            ip += 4;
            break;
        case OP_MK_CLOSURE:
        {
            /* dst (4), const idx (4), ncaptures (4), then ncaptures * 4 bytes of register indices */
            if (ip + 12 > bc->code_size)
                return "truncated mk_closure";
            int32_t nc = 0;
            memcpy(&nc, &bc->code[ip + 8], 4); /* read ncaptures */
            if (nc < 0)
                return "negative ncaptures";
            if ((size_t)(ip + 12 + (size_t)nc * 4) > bc->code_size)
                return "truncated mk_closure captures";
            ip += 12 + (size_t)nc * 4;
            break;
        }
        case OP_CALL_CLOSURE:
            /* obj_reg (4), nargs (4), dst (4) */
            if (ip + 12 > bc->code_size)
                return "truncated call_closure";
            ip += 12;
            break;
        case OP_POP_HANDLER:
            break;
        default:
            return "unknown opcode in verifier";
        }
        if (ip > bc->code_size)
            return "bytecode truncated or malformed";
    }
    return NULL;
}
