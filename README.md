# Virtual-Machine
A project to understand how virtual machines work

Implemented a register-based bytecode VM in C including a verifier, disassembler, interpreter, and native ABI. I designed and implemented a mark-and-sweep GC with object free-list reuse for stable indices, first-class closures with captured environments, and exception handler unwinding. I also added stress tests and GitHub Actions CI to validate correctness across Linux and Windows.

Design decisions: 

array-indexed heap entries to allow O(1) stable references

Mark-and-sweep chosen for simplicity and deterministic reclamation; roots: registers + saved call-frame snapshots + object fields; sweep reclaims strings and objects and pushes freed object indices to a free-list for reuse. Current design is good for small workloads and simple code.

For performance, added save only callee-clobbered registers on CALL (reduced mem copies on nested calls).
Reuse object slots with free-list to avoid churning and keep object indices small and stable.


To-do:
    1. Improve garbage collection for scale (maybe copy mark and sweek to reduct fragmentation or performance GC like generational/parallel collector)
    2. Improve on compiler to a front-end that compiles source language to bytecode