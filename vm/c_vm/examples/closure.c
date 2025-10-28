#include <stdio.h>
#include <string.h>
#include "../include/bytecode.h"
#include "../include/vm.h"

int main(void)
{
    Bytecode bc;
    bc_init(&bc);

    int ci_str = bc_add_const_string(&bc, "Captured string");
    int ci_num = bc_add_const_int(&bc, 42);

    /* main: load string->r2, int->r3, mk_closure r1,const(func),ncaptures=2, r2,r3; call_closure r1 nargs=0 dst=0; halt */
    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 2);
    bc_emit_i32(&bc, ci_str);

    bc_emit(&bc, OP_LOAD_CONST);
    bc_emit_i32(&bc, 3);
    bc_emit_i32(&bc, ci_num);

    /* placeholder for function, will append function body then add function const */
    bc_emit(&bc, OP_MK_CLOSURE);
    bc_emit_i32(&bc, 1);             /* dst r1 */
    size_t mk_ci_pos = bc.code_size; /* patch this with function const index */
    bc_emit_i32(&bc, 0);             /* placeholder const idx */
    bc_emit_i32(&bc, 2);             /* ncaptures */
    bc_emit_i32(&bc, 2);             /* capture r2 */
    bc_emit_i32(&bc, 3);             /* capture r3 */

    /* call closure r1 nargs=0 dst=0 */
    bc_emit(&bc, OP_CALL_CLOSURE);
    bc_emit_i32(&bc, 1); /* obj reg */
    bc_emit_i32(&bc, 0); /* nargs */
    bc_emit_i32(&bc, 0); /* dst r0 */

    bc_emit(&bc, OP_HALT);

    /* append function: PRINT r0; PRINT r1; RET r0 */
    int func_start = (int)bc.code_size;
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 0);
    bc_emit(&bc, OP_PRINT);
    bc_emit_i32(&bc, 1);
    bc_emit(&bc, OP_RET);
    bc_emit_i32(&bc, 0);
    int ci_func = bc_add_const_function(&bc, func_start, 0);

    /* patch closure const index */
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
