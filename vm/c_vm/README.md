# C VM (C implementation)

This folder contains a simple register-based virtual machine implemented in c.

Build (from the `vm/c_vm` directory):

```sh
mkdir -p build && cd build
cmake ..
cmake --build . --target vm_c_example
./vm_c_example
```

What is included:
- Bytecode representation (`include/bytecode.h`, `src/bytecode.c`)
- VM runtime (`include/vm.h`, `src/vm.c`): registers, interpreter, simple heap for strings, basic mark-and-sweep GC
- Disassembler and verifier (`include/disassembler.h`, `include/verifier.h`)
- Example program (`examples/main.c`)
- Try/catch example (`examples/trycatch.c`) demonstrating exception push/pop and unwinding
 
Try/catch example
-----------------

`examples/trycatch.c` demonstrates pushing a handler, calling a function that throws an exception (via `OP_THROW r`), and the handler receiving the exception value in register `r0`.

Build both examples with CMake from `vm/c_vm`:

```powershell
mkdir build; cd build; cmake ..
cmake --build . --target vm_c_example
cmake --build . --target vm_trycatch
./vm_trycatch.exe
```
Closure examples and GC test
---------------------------

`examples/closure.c` demonstrates creating a closure (via OP_MK_CLOSURE) that captures values and calling it (`OP_CALL_CLOSURE`).

`examples/closure_test.c` is a small stress-test which allocates many temporary strings to trigger the GC and then calls a closure that holds captured values; its purpose is to verify captured values survive collection.

Build and run the closure examples with CMake:

```powershell
cmake --build . --target vm_closure
cmake --build . --target vm_closure_test
./vm_closure.exe
./vm_closure_test.exe
```
- native function registry and better CALL/RET semantics
- a small compiler from a toy language to the VM bytecode
- unit tests and additional opcodes

Compiler example: emitting closures
----------------------------------

`examples/compiler_closure.c` is a tiny bytecode "compiler" that demonstrates how to emit
`OP_MK_CLOSURE` and `OP_CALL_CLOSURE` using the `bc_emit` helpers. It shows a practical
pattern for emitting closures when the function body is placed after the main code:

- emit main code including a placeholder for the function constant index
- append the function body to the end of the bytecode buffer
- add the function constant with `bc_add_const_function(start, nargs)`
- patch the placeholder bytes in the earlier `OP_MK_CLOSURE` operand with the function const index

Closure bytecode conventions used by this VM:
- OP_MK_CLOSURE dst, func_const_idx, ncaptures, capture_reg0, capture_reg1, ...
	- Allocates an object where field 0 = function constant index and fields 1..N = captured Values.
- OP_CALL_CLOSURE obj_reg, nargs, dst
	- Reads the closure object from `obj_reg`, looks up the function const, pushes a frame saving
		the first `nargs` registers, copies capture Values from the closure object into registers
		starting at `r[nargs]`, then jumps to the function start.

This design ensures captures are reachable from the heap (object fields) and are traced by the GC.

Build & run the compiler example with CMake:

```powershell
cmake --build . --target vm_compiler_closure
./vm_compiler_closure.exe
```

