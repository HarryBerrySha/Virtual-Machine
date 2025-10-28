#include "../include/disassembler.h"
#include <iostream>
#include <iomanip>

namespace vm
{

    static int32_t read_i32(const std::vector<u8> &code, size_t &ip)
    {
        if (ip + 4 > code.size())
            return 0;
        int32_t v = 0;
        std::memcpy(&v, &code[ip], 4);
        ip += 4;
        return v;
    }

    void disassemble(const Bytecode &bc, std::ostream &os)
    {
        const auto &code = bc.code;
        size_t ip = 0;
        while (ip < code.size())
        {
            auto op = code[ip++];
            os << std::setw(4) << (ip - 1) << ": ";
            switch (op)
            {
            case OP_HALT:
                os << "HALT\n";
                break;
            case OP_LOAD_CONST:
            {
                int32_t reg = read_i32(code, ip);
                int32_t ci = read_i32(code, ip);
                os << "LOAD_CONST r" << reg << ", const#" << ci << '\n';
                break;
            }
            case OP_MOV:
            {
                int32_t dst = read_i32(code, ip);
                int32_t src = read_i32(code, ip);
                os << "MOV r" << dst << ", r" << src << "\n";
                break;
            }
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            {
                int32_t dst = read_i32(code, ip), a = read_i32(code, ip), b = read_i32(code, ip);
                const char *m = op == OP_ADD ? "ADD" : op == OP_SUB ? "SUB"
                                                   : op == OP_MUL   ? "MUL"
                                                                    : "DIV";
                os << m << " r" << dst << ", r" << a << ", r" << b << "\n";
                break;
            }
            case OP_PRINT:
            {
                int32_t r = read_i32(code, ip);
                os << "PRINT r" << r << "\n";
                break;
            }
            case OP_JMP:
            {
                int32_t rel = read_i32(code, ip);
                os << "JMP " << rel << "\n";
                break;
            }
            case OP_JZ:
            {
                int32_t r = read_i32(code, ip);
                int32_t rel = read_i32(code, ip);
                os << "JZ r" << r << ", " << rel << "\n";
                break;
            }
            case OP_CALL:
            {
                int32_t fi = read_i32(code, ip);
                int32_t nargs = read_i32(code, ip);
                int32_t dst = read_i32(code, ip);
                os << "CALL #" << fi << " nargs=" << nargs << " -> r" << dst << "\n";
                break;
            }
            case OP_RET:
            {
                int32_t r = read_i32(code, ip);
                os << "RET r" << r << "\n";
                break;
            }
            case OP_ALLOC_STR:
            {
                int32_t dst = read_i32(code, ip);
                int32_t ci = read_i32(code, ip);
                os << "ALOC_STR r" << dst << ", const#" << ci << "\n";
                break;
            }
            case OP_THROW:
            {
                int32_t r = read_i32(code, ip);
                os << "THROW r" << r << "\n";
                break;
            }
            case OP_PUSH_HANDLER:
            {
                int32_t rel = read_i32(code, ip);
                os << "PUSH_HANDLER " << rel << "\n";
                break;
            }
            case OP_POP_HANDLER:
                os << "POP_HANDLER\n";
                break;
            default:
                os << "<unknown op " << (int)op << ">\n";
                break;
            }
        }
    }

} // namespace vm
