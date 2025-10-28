# C VM (C implementation)

This folder contains a simple register-based virtual machine implemented in plain C (C99).

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

Next steps you may ask me to implement:
- full root scanning (locals & call frames), closures, function call frames
- native function registry and better CALL/RET semantics
- a small compiler from a toy language to the VM bytecode
- unit tests and additional opcodes
