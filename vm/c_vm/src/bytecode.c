#include "../include/bytecode.h"
#include <stdlib.h>
#include <string.h>
#include "../include/util.h"

void bc_init(Bytecode *bc)
{
    bc->code = NULL;
    bc->code_size = 0;
    bc->consts = NULL;
    bc->consts_count = 0;
    bc->consts_cap = 0;
}

void bc_free(Bytecode *bc)
{
    if (bc->code)
        free(bc->code);
    for (size_t i = 0; i < bc->consts_count; ++i)
    {
        if (bc->consts[i].type == CONST_STRING)
            free(bc->consts[i].value.s);
    }
    free(bc->consts);
    bc_init(bc);
}

static void ensure_code(Bytecode *bc, size_t extra)
{
    size_t need = bc->code_size + extra;
    if (!bc->code)
    {
        bc->code = malloc(need);
    }
    else
    {
        bc->code = realloc(bc->code, need);
    }
}

void bc_emit(Bytecode *bc, u8 b)
{
    ensure_code(bc, 1);
    bc->code[bc->code_size++] = b;
}

void bc_emit_i32(Bytecode *bc, int32_t v)
{
    ensure_code(bc, 4);
    memcpy(&bc->code[bc->code_size], &v, 4);
    bc->code_size += 4;
}

static void ensure_const_cap(Bytecode *bc)
{
    if (bc->consts_count + 1 > bc->consts_cap)
    {
        size_t newcap = bc->consts_cap ? bc->consts_cap * 2 : 8;
        bc->consts = realloc(bc->consts, newcap * sizeof(Constant));
        bc->consts_cap = newcap;
    }
}

int bc_add_const_int(Bytecode *bc, int64_t v)
{
    ensure_const_cap(bc);
    bc->consts[bc->consts_count].type = CONST_INT;
    bc->consts[bc->consts_count].value.i = v;
    return bc->consts_count++;
}

int bc_add_const_double(Bytecode *bc, double v)
{
    ensure_const_cap(bc);
    bc->consts[bc->consts_count].type = CONST_DOUBLE;
    bc->consts[bc->consts_count].value.d = v;
    return bc->consts_count++;
}

int bc_add_const_string(Bytecode *bc, const char *s)
{
    ensure_const_cap(bc);
    bc->consts[bc->consts_count].type = CONST_STRING;
    bc->consts[bc->consts_count].value.s = vm_strdup(s);
    return bc->consts_count++;
}
int bc_add_const_function(Bytecode *bc, int start, int nargs)
{
    ensure_const_cap(bc);
    bc->consts[bc->consts_count].type = CONST_FUNCTION;
    bc->consts[bc->consts_count].value.func.start = start;
    bc->consts[bc->consts_count].value.func.nargs = nargs;
    return bc->consts_count++;
}
