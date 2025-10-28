#include "../include/vm.h"
#include <iostream>
#include <cstring>

namespace vm
{

    VM::VM(const VMOptions &opts) : opts_(opts), regs_(opts.num_registers) {}
    VM::~VM() {}

    void VM::load(const Bytecode &bc)
    {
        bc_ = bc;
        ip_ = 0;
    }

    int64_t VM::alloc_string(const std::string &s)
    {
        heap_strings_.push_back(s);
        marked_.push_back(0);
        return (int64_t)heap_strings_.size() - 1;
    }

    void VM::mark_from_roots()
    {
        // mark regs
        for (const auto &r : regs_)
        {
            if (r.type == Value::STRING && r.str_idx >= 0 && (size_t)r.str_idx < heap_strings_.size())
                marked_[r.str_idx] = 1;
        }
        // TODO: mark stack frames if they reference heap
    }

    void VM::sweep()
    {
        for (size_t i = 0; i < heap_strings_.size(); ++i)
        {
            if (!marked_[i])
            {
                // reclaim
                heap_strings_[i].clear();
            }
            else
            {
                marked_[i] = 0; // reset
            }
        }
    }

    void VM::gc()
    {
        mark_from_roots();
        sweep();
    }

    std::optional<std::string> VM::run()
    {
        auto &code = bc_.code;
        auto read_i32 = [&](int32_t &out) -> bool
        {
            if (ip_ + 4 > code.size())
                return false;
            std::memcpy(&out, &code[ip_], 4);
            ip_ += 4;
            return true;
        };

        try
        {
            while (ip_ < code.size())
            {
                u8 op = code[ip_++];
                switch (op)
                {
                case OP_HALT:
                    return std::nullopt;
                case OP_LOAD_CONST:
                {
                    int32_t reg, ci;
                    read_i32(reg);
                    read_i32(ci);
                    if (ci < 0 || (size_t)ci >= bc_.consts.size())
                        return std::string("const index OOB");
                    auto &c = bc_.consts[ci];
                    if (c.type == Constant::INT)
                    {
                        regs_[reg].type = Value::INT;
                        regs_[reg].i = std::get<int64_t>(c.value);
                    }
                    else if (c.type == Constant::DOUBLE)
                    {
                        regs_[reg].type = Value::DOUBLE;
                        regs_[reg].d = std::get<double>(c.value);
                    }
                    else if (c.type == Constant::STRING)
                    {
                        regs_[reg].type = Value::STRING;
                        regs_[reg].str_idx = alloc_string(std::get<std::string>(c.value));
                    }
                    break;
                }
                case OP_MOV:
                {
                    int32_t dst, src;
                    read_i32(dst);
                    read_i32(src);
                    regs_[dst] = regs_[src];
                    break;
                }
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                {
                    int32_t dst, a, b;
                    read_i32(dst);
                    read_i32(a);
                    read_i32(b);
                    // only int for simplicity
                    if (regs_[a].type != Value::INT || regs_[b].type != Value::INT)
                        return std::string("type error: expected INT");
                    int64_t av = regs_[a].i, bv = regs_[b].i, rv = 0;
                    if (op == OP_ADD)
                        rv = av + bv;
                    else if (op == OP_SUB)
                        rv = av - bv;
                    else if (op == OP_MUL)
                        rv = av * bv;
                    else
                        rv = (bv == 0 ? 0 : av / bv);
                    regs_[dst].type = Value::INT;
                    regs_[dst].i = rv;
                    break;
                }
                case OP_PRINT:
                {
                    int32_t r;
                    read_i32(r);
                    if (regs_[r].type == Value::INT)
                        std::cout << regs_[r].i << "\n";
                    else if (regs_[r].type == Value::DOUBLE)
                        std::cout << regs_[r].d << "\n";
                    else if (regs_[r].type == Value::STRING)
                        std::cout << heap_strings_[regs_[r].str_idx] << "\n";
                    else
                        std::cout << "<none>\n";
                    break;
                }
                case OP_JMP:
                {
                    int32_t rel;
                    read_i32(rel);
                    ip_ = (size_t)rel;
                    break;
                }
                case OP_JZ:
                {
                    int32_t r, rel;
                    read_i32(r);
                    read_i32(rel);
                    bool iszero = (regs_[r].type == Value::INT && regs_[r].i == 0);
                    if (iszero)
                        ip_ = (size_t)rel;
                    break;
                }
                case OP_ALLOC_STR:
                {
                    int32_t dst, ci;
                    read_i32(dst);
                    read_i32(ci);
                    if (ci < 0 || (size_t)ci >= bc_.consts.size())
                        return std::string("const index OOB");
                    auto &c = bc_.consts[ci];
                    if (c.type != Constant::STRING)
                        return std::string("const not string");
                    regs_[dst].type = Value::STRING;
                    regs_[dst].str_idx = alloc_string(std::get<std::string>(c.value));
                    break;
                }
                case OP_CALL:
                {
                    int32_t fi, nargs, dst;
                    read_i32(fi);
                    read_i32(nargs);
                    read_i32(dst);
                    // for simplicity we only allow builtin print function at index 0
                    if (fi == 0)
                    {
                        // print first arg
                        int32_t argreg = 0; // assume arg in r0
                        if (regs_[argreg].type == Value::INT)
                            std::cout << regs_[argreg].i << "\n";
                        else if (regs_[argreg].type == Value::STRING)
                            std::cout << heap_strings_[regs_[argreg].str_idx] << "\n";
                        regs_[dst].type = Value::NONE;
                    }
                    else
                        return std::string("unknown function index");
                    break;
                }
                case OP_RET:
                {
                    int32_t r;
                    read_i32(r);
                    // for single-main program halt
                    return std::nullopt;
                }
                case OP_THROW:
                {
                    int32_t r;
                    read_i32(r);
                    return std::string("unhandled exception");
                }
                case OP_PUSH_HANDLER:
                {
                    int32_t rel;
                    read_i32(rel);
                    handler_stack_.push_back(rel);
                    break;
                }
                case OP_POP_HANDLER:
                {
                    if (!handler_stack_.empty())
                        handler_stack_.pop_back();
                    break;
                }
                default:
                    return std::string("unknown opcode at runtime: ") + std::to_string(op);
                }
                // opportunistic GC
                if (heap_strings_.size() > 1024)
                    gc();
            }
        }
        catch (const std::exception &e)
        {
            return std::string("runtime exception: ") + e.what();
        }
        return std::nullopt;
    }

    void VM::disassemble(std::ostream &os) const { disassemble(bc_, os); }

    bool VM::verify(std::string &err) const
    {
        auto o = verify_bytecode(bc_);
        if (o)
        {
            err = *o;
            return false;
        }
        return true;
    }

} // namespace vm
