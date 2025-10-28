// compiler.h - tiny compiler from a minimal Python-like language to bytecode
#pragma once

#include "bytecode.h"
#include <string>

namespace vm
{

    // compile source text to bytecode; returns error on failure
    std::optional<std::string> compile_to_bytecode(const std::string &src, Bytecode &out);

} // namespace vm
