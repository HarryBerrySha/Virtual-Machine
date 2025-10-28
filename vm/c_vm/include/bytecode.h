#ifndef VM_BYTECODE_H
#define VM_BYTECODE_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;

typedef enum
{
    CONST_INT,
    CONST_DOUBLE,
    CONST_STRING,
    CONST_FUNCTION
} ConstType;

typedef struct
{
    ConstType type;
    union
    {
        int64_t i;
        double d;
        char *s;
        struct
        {
            int start;
            int nargs;
        } func;
    } value;
} Constant;

typedef struct
{
    u8 *code;
    size_t code_size;
    Constant *consts;
    size_t consts_count;
    size_t consts_cap;
} Bytecode;

/* helpers to init/free */
void bc_init(Bytecode *bc);
void bc_free(Bytecode *bc);
void bc_emit(Bytecode *bc, u8 b);
void bc_emit_i32(Bytecode *bc, int32_t v);
int bc_add_const_int(Bytecode *bc, int64_t v);
int bc_add_const_double(Bytecode *bc, double v);
int bc_add_const_string(Bytecode *bc, const char *s);
int bc_add_const_function(Bytecode *bc, int start, int nargs);

/* opcodes */
enum OpCode
{
    OP_HALT = 0,
    OP_LOAD_CONST,
    OP_MOV,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_PRINT,
    OP_JMP,
    OP_JZ,
    OP_ALLOC_STR,
    OP_CALL,
    OP_CALL_USER,
    OP_RET,
    OP_THROW,
    OP_PUSH_HANDLER,
    OP_POP_HANDLER,
    OP_MK_CLOSURE,
    OP_CALL_CLOSURE
};

#endif
