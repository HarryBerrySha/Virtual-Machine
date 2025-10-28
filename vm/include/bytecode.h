// bytecode.h - simple register-based bytecode format
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <variant>

namespace vm
{

    using u8 = uint8_t;
    using i32 = int32_t;

    enum Opcode : u8
    {
        OP_HALT = 0,
        OP_LOAD_CONST, // reg, const_index
        OP_MOV,        // dst, src
        OP_ADD,        // dst, lhs, rhs
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_PRINT,        // reg
        OP_JMP,          // rel
        OP_JZ,           // reg, rel
        OP_CALL,         // func_index, nargs, dest_reg
        OP_RET,          // reg
        OP_ALLOC_STR,    // dst, const_index
        OP_THROW,        // reg
        OP_PUSH_HANDLER, // ip_rel
        OP_POP_HANDLER,
    };

    struct Constant
    {
        enum Type
        {
            INT,
            DOUBLE,
            STRING
        } type;
        std::variant<int64_t, double, std::string> value;
    };

    struct Bytecode
    {
        std::vector<u8> code;
        std::vector<Constant> consts;
        // helpers
        void emit(u8 b) { code.push_back(b); }
        void emit_i32(i32 v)
        {
            auto p = (u8 *)&v;
            for (size_t i = 0; i < sizeof(i32); ++i)
                code.push_back(p[i]);
        }
    };

} // namespace vm
