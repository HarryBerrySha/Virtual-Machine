#include <stdio.h>
#include <string.h>
#include "../include/bytecode.h"
#include "../include/vm.h"

int main(void)
{
    Bytecode bc;
    bc_init(&bc);

    int ci_exc = bc_add_const_string(&bc, "Exception: boom!");

    /* main: push handler, call user func, halt */
    bc_emit(&bc, OP_PUSH_HANDLER);
    size_t handler_pos = bc.code_size;
    bc_emit_i32(&bc, 0); /* placeholder for handler loc */

    bc_emit(&bc, OP_CALL_USER);
    size_t call_ci_pos = bc.code_size;
    bc_emit_i32(&bc, 0); /* placeholder for function const */
    bc_emit_i32(&bc, 0); /* nargs = 0 */
    bc_emit_i32(&bc, 0); /* dst = r0 */

    bc_emit(&bc, OP_HALT);

    /* function body: load exception into r0, throw */
    int func_start = (int)bc.code_size;
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 0); /* r0 */
    bc_emit_i32(&bc, ci_exc);
    bc_emit(&bc, OP_THROW);
    bc_emit_i32(&bc, 0); /* throw from r0 */

    /* handler: print r0 (the exception), pop handler, halt */
    int handler_start = (int)bc.code_size;
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 0);
    bc_emit(&bc, OP_POP_HANDLER);
    bc_emit(&bc, OP_HALT);

    /* add function constant and patch call/handler positions */
    int ci_func = bc_add_const_function(&bc, func_start, 0);
    memcpy(&bc.code[call_ci_pos], &ci_func, 4);
    memcpy(&bc.code[handler_pos], &handler_start, 4);

    VMOptions opts;
    opts.num_registers = 8;
    VM *vm = vm_create(&opts);
    vm_load(vm, &bc);
    const char *err = vm_run(vm);
    if (err)
        printf("VM runtime error: %s\n", err);

    vm_destroy(vm);
    bc_free(&bc);
    return 0;
}
