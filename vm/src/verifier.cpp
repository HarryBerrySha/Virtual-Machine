#include "../include/verifier.h"
#include <string>
#include <iostream>

namespace vm
{

    std::optional<std::string> verify_bytecode(const Bytecode &bc)
    {
        // Basic validation: ensure instruction operands don't run past end
        size_t ip = 0;
        auto &code = bc.code;
        auto need = [&](size_t n) -> bool
        { return ip + n <= code.size(); };
        while (ip < code.size())
        {
            u8 op = code[ip++];
            switch (op)
            {
            case OP_HALT:
                break;
            case OP_LOAD_CONST:
            case OP_MOV:
                if (!need(8))
                    return "truncated operand for LOAD_CONST/MOV";
                ip += 8;
                break;
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                if (!need(12))
                    return "truncated math operands";
                ip += 12;
                break;
            case OP_PRINT:
            case OP_RET:
            case OP_THROW:
                if (!need(4))
                    return "truncated single-reg operand";
                ip += 4;
                break;
            case OP_JMP:
                if (!need(4))
                    return "truncated JMP";
                ip += 4;
                break;
            case OP_JZ:
                if (!need(8))
                    return "truncated JZ";
                ip += 8;
                break;
            case OP_CALL:
                if (!need(12))
                    return "truncated CALL";
                ip += 12;
                break;
            case OP_ALLOC_STR:
                if (!need(8))
                    return "truncated ALLOC_STR";
                ip += 8;
                break;
            case OP_PUSH_HANDLER:
                if (!need(4))
                    return "truncated PUSH_HANDLER";
                ip += 4;
                break;
            case OP_POP_HANDLER:
                break;
            default:
                return std::string("unknown opcode ") + std::to_string(op);
            }
        }
        return std::nullopt;
    }

}
