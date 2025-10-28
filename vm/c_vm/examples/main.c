#include <stdio.h>
#include <string.h>
#include "../include/bytecode.h"
#include "../include/vm.h"

int main(void)
{
    Bytecode bc;
    bc_init(&bc);
    int ci_hello = bc_add_const_string(&bc, "Hello from C VM");
    int ci_num = bc_add_const_int(&bc, 12345);

    /* main program: load string into r0, call user function (we'll append the function later), load int into r1, print r1, halt */
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 0);
    bc_emit_i32(&bc, ci_hello);
    bc_emit(&bc, OP_CALL_USER);
    size_t call_ci_pos = bc.code_size;
    bc_emit_i32(&bc, 0); /* placeholder for const index */
    bc_emit_i32(&bc, 1);
    bc_emit_i32(&bc, 0);
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 1);
    bc_emit_i32(&bc, ci_num);
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 1);
    bc_emit(&bc, OP_HALT);

    /* append function body: PRINT r0 ; RET r0 */
    int func_start = (int)bc.code_size;
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 0);
    bc_emit(&bc, OP_RET);
    bc_emit_i32(&bc, 0);
    int ci_func = bc_add_const_function(&bc, func_start, 1);
    /* patch call const index */
    memcpy(&bc.code[call_ci_pos], &ci_func, 4);

    VMOptions opts;
    opts.num_registers = 8;
    VM *vm = vm_create(&opts);
    vm_load(vm, &bc);
    const char *err = vm_run(vm);
    if (err)
    {
        printf("VM runtime error: %s\n", err);
    }
    vm_destroy(vm);
    bc_free(&bc);
    return 0;
}
