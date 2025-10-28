#include <stdio.h>
#include <string.h>
#include "../include/bytecode.h"
#include "../include/vm.h"

// This test creates a closure that captures a string and an int, then
// performs many allocations to trigger GC and ensures the closure's
// captured values are still available when the closure is invoked.
int main(void)
{
    Bytecode bc;
    bc_init(&bc);

    int ci_str = bc_add_const_string(&bc, "captured-for-gc");
    int ci_num = bc_add_const_int(&bc, 777);

    // Load captures into r2,r3
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 2);
    bc_emit_i32(&bc, ci_str);
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 3);
    bc_emit_i32(&bc, ci_num);

    // MK_CLOSURE r1, const(func) placeholder, ncaptures=2, r2, r3
    bc_emit(&bc, OP_MK_CLOSURE);
    bc_emit_i32(&bc, 1); // dst r1
    size_t mk_ci_pos = bc.code_size;
    bc_emit_i32(&bc, 0); // placeholder
    bc_emit_i32(&bc, 2); // ncaptures
    bc_emit_i32(&bc, 2);
    bc_emit_i32(&bc, 3);

    // Now do many allocations to trigger GC (use same constant repeatedly)
    for (int i = 0; i < 1100; ++i)
    {
        bc_emit(&bc, OP_ALLOC_STR);
        bc_emit_i32(&bc, 10); // dst r10 (temp)
        bc_emit_i32(&bc, ci_str);
    }

    // Call closure r1 nargs=0 dst=r0
    bc_emit(&bc, OP_CALL_CLOSURE);
    bc_emit_i32(&bc, 1); // obj r1
    bc_emit_i32(&bc, 0); // nargs
    bc_emit_i32(&bc, 0); // dst r0

    bc_emit(&bc, OP_HALT);

    // function: print r0 (string), print r1 (int), ret r0
    int func_start = (int)bc.code_size;
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 0);
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 1);
    bc_emit(&bc, OP_RET);
    bc_emit_i32(&bc, 0);
    int ci_func = bc_add_const_function(&bc, func_start, 0);

    // patch closure const index
    memcpy(&bc.code[mk_ci_pos], &ci_func, 4);

    VMOptions opts;
    opts.num_registers = 16;
    VM *vm = vm_create(&opts);
    vm_load(vm, &bc);
    const char *err = vm_run(vm);
    if (err)
        printf("VM error: %s\n", err);
    vm_destroy(vm);
    bc_free(&bc);
    return 0;
}
