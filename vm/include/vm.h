// vm.h - VM runtime interface
#pragma once

#include "bytecode.h"
#include <vector>
#include <string>
#include <optional>

namespace vm
{

    struct Value
    {
        enum Type
        {
            INT,
            DOUBLE,
            STRING,
            NONE
        } type = NONE;
        int64_t i = 0;
        double d = 0.0;
        int64_t str_idx = -1; // index into GC heap for strings
    };

    struct VMOptions
    {
        size_t num_registers = 16;
        size_t stack_limit = 1024;
    };

    struct VM
    {
        VM(const VMOptions &opts = {});
        ~VM();

        // load code
        void load(const Bytecode &bc);
        // run program, return optional error string
        std::optional<std::string> run();

        // disassemble / verify
        void disassemble(std::ostream &os) const;
        bool verify(std::string &err) const;

        // helper to allocate string on heap (returns index)
        int64_t alloc_string(const std::string &s);

    private:
        // internal
        VMOptions opts_;
        Bytecode bc_;
        std::vector<Value> regs_;
        std::vector<int32_t> call_stack_;
        std::vector<int64_t> handler_stack_; // ip of handlers

        // heap: store strings only for this minimal implementation
        std::vector<std::string> heap_strings_;
        std::vector<char> marked_; // mark bits for GC

        size_t ip_ = 0;
        // GC
        void gc();
        void mark_from_roots();
        void sweep();
    };

} // namespace vm
