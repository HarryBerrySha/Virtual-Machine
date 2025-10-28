// disassembler.h
#pragma once
#include "bytecode.h"
#include <ostream>
namespace vm
{
    void disassemble(const Bytecode &bc, std::ostream &os);
}
