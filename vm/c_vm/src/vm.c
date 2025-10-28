#include "../include/vm.h"
#include "../include/disassembler.h"
#include "../include/verifier.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* portable strdup implementation to avoid implicit declaration warnings on some
   platforms. Implemented here and declared in include/util.h and include/vm.h. */
char *vm_strdup(const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    return p;
}

typedef struct HeapString
{
    char *s;
    int marked;
    struct HeapString *next;
} HeapString;

typedef struct HeapObject
{
    Value *fields;
    int field_count;
    int marked;
    int alive; /* 1 = allocated/live, 0 = freed */
} HeapObject;

typedef struct Frame
{
    int return_ip;
    int return_dst;
    Value *saved_regs;
    int saved_count;
    struct Frame *next;
} Frame;

struct VM
{
    VMOptions opts;
    Value *regs;
    Bytecode bc;
    size_t ip;
    HeapString *heap_head;
    size_t heap_count;
    HeapObject *obj_array;
    size_t obj_count;
    size_t obj_cap;
    int *obj_free_list;
    size_t obj_free_count;
    size_t obj_free_cap;
    /* call frames */
    int *call_ret_ips;
    int call_count;
    int call_cap;
    /* frame list for user function calls */
    Frame *frames;
    int frames_count;
    /* handler stack */
    int *handlers;
    int handlers_count;
    int handlers_cap;
    /* native functions */
    NativeFn *natives;
    int natives_count;
    int natives_cap;
};

VM *vm_create(const VMOptions *opts)
{
    VM *vm = (VM *)malloc(sizeof(VM));
    vm->opts = *opts;
    vm->regs = (Value *)calloc(opts->num_registers, sizeof(Value));
    vm->bc.code = NULL;
    vm->bc.code_size = 0;
    vm->bc.consts = NULL;
    vm->bc.consts_count = 0;
    vm->bc.consts_cap = 0;
    vm->ip = 0;
    vm->heap_head = NULL;
    vm->heap_count = 0;
    vm->obj_array = NULL;
    vm->obj_count = 0;
    vm->obj_cap = 0;
    vm->obj_free_list = NULL;
    vm->obj_free_count = 0;
    vm->obj_free_cap = 0;
    vm->call_ret_ips = NULL;
    vm->call_count = 0;
    vm->call_cap = 0;
    vm->handlers = NULL;
    vm->handlers_count = 0;
    vm->handlers_cap = 0;
    vm->natives = NULL;
    vm->natives_count = 0;
    vm->natives_cap = 0;
    vm->frames = NULL;
    vm->frames_count = 0;
    return vm;
}

void vm_destroy(VM *vm)
{
    if (!vm)
        return;
    free(vm->regs);
    bc_free(&vm->bc);
    HeapString *cur = vm->heap_head;
    while (cur)
    {
        HeapString *n = cur->next;
        free(cur->s);
        free(cur);
        cur = n;
    }
    /* free objects array and fields */
    if (vm->obj_array)
    {
        for (size_t i = 0; i < vm->obj_count; ++i)
        {
            if (vm->obj_array[i].alive && vm->obj_array[i].fields)
                free(vm->obj_array[i].fields);
        }
        free(vm->obj_array);
    }
    free(vm->obj_free_list);
    free(vm->call_ret_ips);
    free(vm->handlers);
    free(vm->natives);
    /* free frames */
    Frame *f = vm->frames;
    while (f)
    {
        Frame *n = f->next;
        if (f->saved_regs)
            free(f->saved_regs);
        free(f);
        f = n;
    }
    free(vm);
}

void vm_load(VM *vm, const Bytecode *bc)
{
    bc_init(&vm->bc);
    /* deep copy bc */
    vm->bc.code = malloc(bc->code_size);
    memcpy(vm->bc.code, bc->code, bc->code_size);
    vm->bc.code_size = bc->code_size;
    vm->bc.consts_count = 0;
    vm->bc.consts_cap = 0;
    vm->bc.consts = NULL;
    for (size_t i = 0; i < bc->consts_count; ++i)
    {
        Constant *c = &bc->consts[i];
        if (c->type == CONST_INT)
            bc_add_const_int(&vm->bc, c->value.i);
        else if (c->type == CONST_DOUBLE)
            bc_add_const_double(&vm->bc, c->value.d);
        else if (c->type == CONST_STRING)
            bc_add_const_string(&vm->bc, c->value.s);
        else if (c->type == CONST_FUNCTION)
            bc_add_const_function(&vm->bc, c->value.func.start, c->value.func.nargs);
    }
    vm->ip = 0;
}

static void heap_mark_from_roots(VM *vm)
{
    for (int i = 0; i < vm->opts.num_registers; ++i)
    {
        if (vm->regs[i].type == V_STRING)
        {
            int idx = vm->regs[i].as.str_idx; // index -> walk linked list
            HeapString *cur = vm->heap_head;
            int j = 0;
            while (cur && j < idx)
            {
                cur = cur->next;
                ++j;
            }
            if (cur)
                cur->marked = 1;
        }
        else if (vm->regs[i].type == V_OBJECT)
        {
            int idx = vm->regs[i].as.obj_idx;
            if (idx >= 0 && (size_t)idx < vm->obj_count)
            {
                if (vm->obj_array[idx].alive)
                    vm->obj_array[idx].marked = 1;
            }
        }
    }
    /* mark from frames' saved regs (we save only the callee-clobbered subset) */
    Frame *fr = vm->frames;
    while (fr)
    {
        if (fr->saved_regs && fr->saved_count > 0)
        {
            for (int i = 0; i < fr->saved_count; ++i)
            {
                if (fr->saved_regs[i].type == V_STRING)
                {
                    int idx = fr->saved_regs[i].as.str_idx;
                    HeapString *cur = vm->heap_head;
                    int j = 0;
                    while (cur && j < idx)
                    {
                        cur = cur->next;
                        ++j;
                    }
                    if (cur)
                        cur->marked = 1;
                }
                else if (fr->saved_regs[i].type == V_OBJECT)
                {
                    int idx = fr->saved_regs[i].as.obj_idx;
                    if (idx >= 0 && (size_t)idx < vm->obj_count)
                    {
                        if (vm->obj_array[idx].alive)
                            vm->obj_array[idx].marked = 1;
                    }
                }
            }
        }
        fr = fr->next;
    }

    /* propagate marks across object graph until fixed point */
    int changed = 1;
    while (changed)
    {
        changed = 0;
        for (size_t oi = 0; oi < vm->obj_count; ++oi)
        {
            HeapObject *o = &vm->obj_array[oi];
            if (!o->alive)
                continue;
            if (o->marked)
            {
                for (int f = 0; f < o->field_count; ++f)
                {
                    Value *v = &o->fields[f];
                    if (v->type == V_STRING)
                    {
                        int idx = v->as.str_idx;
                        HeapString *cur = vm->heap_head;
                        int j = 0;
                        while (cur && j < idx)
                        {
                            cur = cur->next;
                            ++j;
                        }
                        if (cur && !cur->marked)
                        {
                            cur->marked = 1;
                            changed = 1;
                        }
                    }
                    else if (v->type == V_OBJECT)
                    {
                        int idx = v->as.obj_idx;
                        if (idx >= 0 && (size_t)idx < vm->obj_count)
                        {
                            HeapObject *obj2 = &vm->obj_array[idx];
                            if (obj2->alive && !obj2->marked)
                            {
                                obj2->marked = 1;
                                changed = 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void heap_sweep(VM *vm)
{
    HeapString **p = &vm->heap_head;
    int idx = 0;
    while (*p)
    {
        HeapString *cur = *p;
        if (!cur->marked)
        {
            *p = cur->next;
            free(cur->s);
            free(cur);
            vm->heap_count--;
            /* do not increment p or idx */
        }
        else
        {
            cur->marked = 0;
            p = &cur->next;
            ++idx;
        }
    }

    /* sweep objects: free unreachable objects and push indices onto free-list */
    for (size_t i = 0; i < vm->obj_count; ++i)
    {
        HeapObject *o = &vm->obj_array[i];
        if (!o->alive)
            continue;
        if (!o->marked)
        {
            /* free object fields */
            if (o->fields)
            {
                free(o->fields);
                o->fields = NULL;
            }
            o->field_count = 0;
            o->alive = 0;

            /* push this index onto the free-list */
            if (vm->obj_free_count + 1 > vm->obj_free_cap)
            {
                size_t newcap = vm->obj_free_cap ? vm->obj_free_cap * 2 : 8;
                vm->obj_free_list = realloc(vm->obj_free_list, newcap * sizeof(int));
                vm->obj_free_cap = newcap;
            }
            vm->obj_free_list[vm->obj_free_count++] = (int)i;
        }
        else
        {
            /* clear mark for next GC cycle */
            o->marked = 0;
        }
    }
}

void vm_gc(VM *vm)
{
    heap_mark_from_roots(vm);
    heap_sweep(vm);
}

int vm_alloc_string(VM *vm, const char *s)
{
    HeapString *hs = (HeapString *)malloc(sizeof(HeapString));
    hs->s = vm_strdup(s);
    hs->marked = 0;
    hs->next = NULL;
    if (!vm->heap_head)
        vm->heap_head = hs;
    else
    {
        HeapString *cur = vm->heap_head;
        while (cur->next)
            cur = cur->next;
        cur->next = hs;
    }
    int idx = vm->heap_count++;
    return idx;
}

int vm_alloc_object(VM *vm, int field_count)
{
    /* reuse freed slot if available */
    int idx = -1;
    if (vm->obj_free_count > 0)
    {
        idx = vm->obj_free_list[--vm->obj_free_count];
    }
    else
    {
        /* need new slot */
        if (vm->obj_count == vm->obj_cap)
        {
            size_t newcap = vm->obj_cap ? vm->obj_cap * 2 : 8;
            vm->obj_array = realloc(vm->obj_array, newcap * sizeof(HeapObject));
            /* zero new slots */
            for (size_t i = vm->obj_cap; i < newcap; ++i)
            {
                vm->obj_array[i].fields = NULL;
                vm->obj_array[i].field_count = 0;
                vm->obj_array[i].marked = 0;
                vm->obj_array[i].alive = 0;
            }
            vm->obj_cap = newcap;
        }
        idx = (int)vm->obj_count++;
    }
    /* allocate fields for this object */
    vm->obj_array[idx].fields = (Value *)calloc(field_count, sizeof(Value));
    vm->obj_array[idx].field_count = field_count;
    vm->obj_array[idx].marked = 0;
    vm->obj_array[idx].alive = 1;
    return idx;
}

void vm_set_object_field(VM *vm, int obj_idx, int field, Value val)
{
    if (obj_idx < 0 || (size_t)obj_idx >= vm->obj_count)
        return;
    HeapObject *cur = &vm->obj_array[obj_idx];
    if (!cur->alive)
        return;
    if (field < 0 || field >= cur->field_count)
        return;
    cur->fields[field] = val;
}

Value vm_get_object_field(VM *vm, int obj_idx, int field)
{
    Value none;
    none.type = V_NONE;
    if (obj_idx < 0)
        return none;
    if ((size_t)obj_idx >= vm->obj_count)
        return none;
    HeapObject *cur = &vm->obj_array[obj_idx];
    if (!cur->alive)
        return none;
    if (field < 0 || field >= cur->field_count)
        return none;
    return cur->fields[field];
}

void vm_register_native(VM *vm, int index, NativeFn fn)
{
    if (index < 0)
        return;
    if (index + 1 > vm->natives_cap)
    {
        int newcap = vm->natives_cap ? vm->natives_cap * 2 : 8;
        while (newcap <= index)
            newcap *= 2;
        vm->natives = realloc(vm->natives, newcap * sizeof(NativeFn));
        for (int i = vm->natives_count; i < newcap; ++i)
            vm->natives[i] = NULL;
        vm->natives_cap = newcap;
    }
    vm->natives[index] = fn;
    if (index >= vm->natives_count)
        vm->natives_count = index + 1;
}

const char *vm_run(VM *vm)
{
    const char *verr = vm_verify(vm);
    if (verr)
        return verr;

    while (vm->ip < vm->bc.code_size)
    {
        u8 op = vm->bc.code[vm->ip++];
        switch (op)
        {
        case OP_HALT:
            return NULL;
        case OP_LOAD_CONST:
        {
            int32_t reg;
            memcpy(&reg, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            int32_t ci;
            memcpy(&ci, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            Constant *c = &vm->bc.consts[ci];
            if (c->type == CONST_INT)
            {
                vm->regs[reg].type = V_INT;
                vm->regs[reg].as.i = c->value.i;
            }
            else if (c->type == CONST_DOUBLE)
            {
                vm->regs[reg].type = V_DOUBLE;
                vm->regs[reg].as.d = c->value.d;
            }
            else if (c->type == CONST_STRING)
            {
                vm->regs[reg].type = V_STRING;
                vm->regs[reg].as.str_idx = vm_alloc_string(vm, c->value.s);
            }
            break;
        }
        case OP_MOV:
        {
            int32_t dst;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            int32_t src;
            memcpy(&src, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            vm->regs[dst] = vm->regs[src];
            break;
        }
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        {
            int32_t dst, a, b;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&a, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&b, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (vm->regs[a].type != V_INT || vm->regs[b].type != V_INT)
                return "type error: expected int";
            int64_t av = vm->regs[a].as.i, bv = vm->regs[b].as.i, rv = 0;
            if (op == OP_ADD)
                rv = av + bv;
            else if (op == OP_SUB)
                rv = av - bv;
            else if (op == OP_MUL)
                rv = av * bv;
            else
            {
                if (bv == 0)
                    return "division by zero";
                rv = av / bv;
            }
            vm->regs[dst].type = V_INT;
            vm->regs[dst].as.i = rv;
            break;
        }
        case OP_PRINT:
        {
            int32_t r;
            memcpy(&r, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (vm->regs[r].type == V_INT)
            {
                printf("%lld\n", (long long)vm->regs[r].as.i);
            }
            else if (vm->regs[r].type == V_DOUBLE)
            {
                printf("%f\n", vm->regs[r].as.d);
            }
            else if (vm->regs[r].type == V_STRING)
            {
                HeapString *cur = vm->heap_head;
                int idx = 0;
                while (cur && idx < vm->regs[r].as.str_idx)
                {
                    cur = cur->next;
                    ++idx;
                }
                if (cur)
                    printf("%s\n", cur->s);
                else
                    printf("<string oob>\n");
            }
            else if (vm->regs[r].type == V_OBJECT)
            {
                int idx = vm->regs[r].as.obj_idx;
                if (idx >= 0 && (size_t)idx < vm->obj_count && vm->obj_array[idx].alive)
                    printf("OBJECT(fields=%d)\n", vm->obj_array[idx].field_count);
                else
                    printf("OBJECT <oob>\n");
            }
            else
            {
                printf("NONE\n");
            }
            break;
        }
        case OP_JMP:
        {
            int32_t loc;
            memcpy(&loc, &vm->bc.code[vm->ip], 4);
            vm->ip = (size_t)loc;
            break;
        }
        case OP_JZ:
        {
            int32_t r;
            memcpy(&r, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            int32_t loc;
            memcpy(&loc, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (vm->regs[r].type == V_INT && vm->regs[r].as.i == 0)
                vm->ip = (size_t)loc;
            break;
        }
        case OP_ALLOC_STR:
        {
            int32_t dst, ci;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&ci, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            vm->regs[dst].type = V_STRING;
            vm->regs[dst].as.str_idx = vm_alloc_string(vm, vm->bc.consts[ci].value.s);
            break;
        }
        case OP_CALL:
        {
            int32_t fi, nargs, dst;
            memcpy(&fi, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&nargs, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (fi >= 0 && fi < vm->natives_count && vm->natives[fi])
            {
                Value *args = NULL;
                if (nargs > 0)
                {
                    args = (Value *)malloc(sizeof(Value) * nargs);
                    for (int i = 0; i < nargs; ++i)
                        args[i] = vm->regs[i];
                }
                Value res = vm->natives[fi](vm, nargs, args);
                if (args)
                    free(args);
                vm->regs[dst] = res;
            }
            else
            {
                return "unknown function index";
            }
            break;
        }
        case OP_CALL_USER:
        {
            int32_t ci, nargs, dst;
            memcpy(&ci, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&nargs, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (ci < 0 || (size_t)ci >= vm->bc.consts_count)
                return "bad function const index";
            Constant *fc = &vm->bc.consts[ci];
            if (fc->type != CONST_FUNCTION)
                return "const is not a function";
            int target = fc->value.func.start;
            /* push frame: save only registers that will be clobbered by callee (0..nargs-1) */
            Frame *f = (Frame *)malloc(sizeof(Frame));
            if (nargs > 0)
            {
                f->saved_regs = (Value *)malloc(sizeof(Value) * nargs);
                for (int si = 0; si < nargs; ++si)
                    f->saved_regs[si] = vm->regs[si];
                f->saved_count = nargs;
            }
            else
            {
                f->saved_regs = NULL;
                f->saved_count = 0;
            }
            f->return_ip = vm->ip;
            f->return_dst = dst;
            f->next = vm->frames;
            vm->frames = f;
            vm->frames_count++;
            /* jump to function start */
            vm->ip = (size_t)target;
            break;
        }
        case OP_RET:
        {
            int32_t r;
            memcpy(&r, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            /* if no frame, terminate program returning value in r (ignored) */
            if (!vm->frames)
                return NULL;
            Frame *f = vm->frames;
            vm->frames = f->next;
            Value retval = vm->regs[r];
            /* restore only the saved registers */
            if (f->saved_count > 0 && f->saved_regs)
            {
                for (int si = 0; si < f->saved_count; ++si)
                    vm->regs[si] = f->saved_regs[si];
            }
            /* store return value into return_dst */
            vm->regs[f->return_dst] = retval;
            int ret_ip = f->return_ip;
            if (f->saved_regs)
                free(f->saved_regs);
            free(f);
            vm->frames_count--;
            vm->ip = (size_t)ret_ip;
            break;
        }
        case OP_THROW:
        {
            int32_t rsrc;
            memcpy(&rsrc, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (rsrc < 0 || rsrc >= vm->opts.num_registers)
                return "bad throw register";
            vm->regs[0] = vm->regs[rsrc];

            if (vm->handlers_count == 0)
                return "unhandled exception";
            int entry_idx = vm->handlers_count - 1;
            int handler_loc = vm->handlers[entry_idx * 2];
            int handler_frames = vm->handlers[entry_idx * 2 + 1];
            /* pop handler */
            vm->handlers_count--;

            /* unwind frame stack until we reach handler_frames */
            while (vm->frames_count > handler_frames)
            {
                Frame *ff = vm->frames;
                if (!ff)
                    break;
                vm->frames = ff->next;
                if (ff->saved_regs)
                    free(ff->saved_regs);
                free(ff);
                vm->frames_count--;
            }

            /* jump to handler location; exception value is available in r0 */
            vm->ip = (size_t)handler_loc;
            break;
        }
        case OP_PUSH_HANDLER:
        {
            int32_t loc;
            memcpy(&loc, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (vm->handlers_count + 1 > vm->handlers_cap)
            {
                int newcap = vm->handlers_cap ? vm->handlers_cap * 2 : 8;
                vm->handlers = realloc(vm->handlers, newcap * 2 * sizeof(int));
                vm->handlers_cap = newcap;
            }
            int e = vm->handlers_count++;
            vm->handlers[e * 2] = loc;
            vm->handlers[e * 2 + 1] = vm->frames_count;
            break;
        }
        case OP_POP_HANDLER:
        {
            if (vm->handlers_count > 0)
                vm->handlers_count--;
            break;
        }
        case OP_MK_CLOSURE:
        {
            int32_t dst, ci, nc;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&ci, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&nc, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (ci < 0 || (size_t)ci >= vm->bc.consts_count)
                return "bad function const index";
            int obj_idx = vm_alloc_object(vm, nc + 1);
            Value v;
            v.type = V_INT;
            v.as.i = ci;
            vm_set_object_field(vm, obj_idx, 0, v);
            for (int i = 0; i < nc; ++i)
            {
                int32_t r;
                memcpy(&r, &vm->bc.code[vm->ip], 4);
                vm->ip += 4;
                if (r < 0 || r >= vm->opts.num_registers)
                    return "bad capture register";
                vm_set_object_field(vm, obj_idx, 1 + i, vm->regs[r]);
            }
            vm->regs[dst].type = V_OBJECT;
            vm->regs[dst].as.obj_idx = obj_idx;
            break;
        }
        case OP_CALL_CLOSURE:
        {
            int32_t objr, nargs, dst;
            memcpy(&objr, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&nargs, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            memcpy(&dst, &vm->bc.code[vm->ip], 4);
            vm->ip += 4;
            if (objr < 0 || objr >= vm->opts.num_registers)
                return "bad closure obj register";
            if (vm->regs[objr].type != V_OBJECT)
                return "call_closure expected object";
            int obj_idx = vm->regs[objr].as.obj_idx;
            if (obj_idx < 0 || (size_t)obj_idx >= vm->obj_count)
                return "closure object oob";
            HeapObject *co = &vm->obj_array[obj_idx];
            if (!co->alive)
                return "dead closure object";
            Value fval = co->fields[0];
            if (fval.type != V_INT)
                return "closure missing function index";
            int ci = (int)fval.as.i;
            if (ci < 0 || (size_t)ci >= vm->bc.consts_count)
                return "bad function const index in closure";
            Constant *fc = &vm->bc.consts[ci];
            if (fc->type != CONST_FUNCTION)
                return "closure const not a function";
            int target = fc->value.func.start;
            Frame *f = (Frame *)malloc(sizeof(Frame));
            if (nargs > 0)
            {
                f->saved_regs = (Value *)malloc(sizeof(Value) * nargs);
                for (int si = 0; si < nargs; ++si)
                    f->saved_regs[si] = vm->regs[si];
                f->saved_count = nargs;
            }
            else
            {
                f->saved_regs = NULL;
                f->saved_count = 0;
            }
            f->return_ip = vm->ip;
            f->return_dst = dst;
            f->next = vm->frames;
            vm->frames = f;
            vm->frames_count++;
            int cap = co->field_count - 1;
            for (int i = 0; i < cap; ++i)
                vm->regs[nargs + i] = co->fields[1 + i];
            vm->ip = (size_t)target;
            break;
        }
        default:
            return "unknown opcode during run";
        }
        if (vm->heap_count > 1024)
            vm_gc(vm);
    }
    return NULL;
}

void vm_disassemble(VM *vm, FILE *os) { disassemble_bytecode(&vm->bc, os); }
const char *vm_verify(VM *vm) { return verify_bytecode(&vm->bc); }

void vm_print_registers(VM *vm, FILE *os)
{
    for (int i = 0; i < vm->opts.num_registers; ++i)
    {
        fprintf(os, "r%d: ", i);
        if (vm->regs[i].type == V_INT)
            fprintf(os, "INT %lld\n", (long long)vm->regs[i].as.i);
        else if (vm->regs[i].type == V_DOUBLE)
            fprintf(os, "DOUBLE %f\n", vm->regs[i].as.d);
        else if (vm->regs[i].type == V_STRING)
        {
            HeapString *cur = vm->heap_head;
            int idx = 0;
            while (cur && idx < vm->regs[i].as.str_idx)
            {
                cur = cur->next;
                ++idx;
            }
            if (cur)
                fprintf(os, "STRING \"%s\"\n", cur->s);
            else
                fprintf(os, "STRING <oob>\n");
        }
        else if (vm->regs[i].type == V_OBJECT)
        {
            int idx = vm->regs[i].as.obj_idx;
            if (idx >= 0 && (size_t)idx < vm->obj_count && vm->obj_array[idx].alive)
                fprintf(os, "OBJECT(fields=%d)\n", vm->obj_array[idx].field_count);
            else
                fprintf(os, "OBJECT <oob>\n");
        }
        else
            fprintf(os, "NONE\n");
    }
}
