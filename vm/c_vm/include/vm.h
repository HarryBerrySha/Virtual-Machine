#ifndef VM_H
#define VM_H

#include "bytecode.h"
#include <stdio.h>

typedef enum
{
    V_NONE = 0,
    V_INT,
    V_DOUBLE,
    V_STRING,
    V_OBJECT
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        int64_t i;
        double d;
        int str_idx; /* index into heap strings */
        int obj_idx; /* index into heap objects */
    } as;
} Value;

typedef struct
{
    int num_registers;
} VMOptions;

typedef struct VM VM;

/* lifecycle */
VM *vm_create(const VMOptions *opts);
void vm_destroy(VM *vm);

/* load bytecode and run */
void vm_load(VM *vm, const Bytecode *bc);
/* returns NULL on success, otherwise pointer to static error string */
const char *vm_run(VM *vm);

/* alloc string on VM heap, returns index */
int vm_alloc_string(VM *vm, const char *s);

/* disassemble and verify */
void vm_disassemble(VM *vm, FILE *os);
const char *vm_verify(VM *vm);

/* debug helpers */
void vm_print_registers(VM *vm, FILE *os);

/* object allocation and access */
int vm_alloc_object(VM *vm, int field_count);
void vm_set_object_field(VM *vm, int obj_idx, int field, Value val);
Value vm_get_object_field(VM *vm, int obj_idx, int field);

/* native function support: a native receives the vm, nargs and pointer to array of args, and returns a Value result */
typedef Value (*NativeFn)(VM *vm, int nargs, const Value *args);
void vm_register_native(VM *vm, int index, NativeFn fn);

#endif
