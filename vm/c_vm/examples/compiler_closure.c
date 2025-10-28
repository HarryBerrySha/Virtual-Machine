#include <stdio.h>
#include <string.h>
#include "../include/bytecode.h"
#include "../include/vm.h"

int main(void)
{
    Bytecode bc;
    bc_init(&bc);

    // Add other constants used by main (string and int) first
    int s_ci = bc_add_const_string(&bc, "captured-for-gc");
    int i_ci = bc_add_const_int(&bc, 777);

    // Emit main program: load const string -> r0, load const int -> r1
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 0); // dst r0
    bc_emit_i32(&bc, s_ci);

    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 1); // dst r1
    bc_emit_i32(&bc, i_ci);

    // mk_closure dst=r2, ci=func_ci (placeholder), ncaptures=2, captures = r0, r1
    bc_emit(&bc, OP_MK_CLOSURE);
    bc_emit_i32(&bc, 2); // dst r2
    int ci_placeholder_pos = (int)bc.code_size;
    bc_emit_i32(&bc, -1); // placeholder for function const index
    bc_emit_i32(&bc, 2);  // ncaptures
    bc_emit_i32(&bc, 0);  // capture r0
    bc_emit_i32(&bc, 1);  // capture r1

    // call_closure objr=r2, nargs=0, dst=0
    bc_emit(&bc, OP_CALL_CLOSURE);
    bc_emit_i32(&bc, 2); // obj register
    bc_emit_i32(&bc, 0); // nargs
    bc_emit_i32(&bc, 0); // dst r0

    // halt (after main)
    bc_emit(&bc, OP_HALT);

    // Now emit the function body at the end of the code buffer
    int func_start = (int)bc.code_size;
    // function body: print r0; print r1; ret r0
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 0); // print r0
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 1); // print r1
    bc_emit(&bc, OP_RET);
    bc_emit_i32(&bc, 0); // return register 0

    // Add function constant now that we know its start
    int func_ci = bc_add_const_function(&bc, func_start, 0);

    // Patch the placeholder function const index in the earlier mk_closure operand
    // ci_placeholder_pos points at the 4-byte slot where we wrote -1
    memcpy(&bc.code[ci_placeholder_pos], &func_ci, 4);

    // Run the bytecode
    VMOptions opts = {.num_registers = 8};
    VM *vm = vm_create(&opts);
    vm_load(vm, &bc);
    const char *err = vm_run(vm);
    if (err)
        fprintf(stderr, "VM error: %s\n", err);

    bc_free(&bc);
    vm_destroy(vm);
    return (err == NULL) ? 0 : 1;
}
