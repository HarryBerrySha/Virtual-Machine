// verifier.h
#pragma once
#include "bytecode.h"
#include <optional>
#include <string>
namespace vm
{
    std::optional<std::string> verify_bytecode(const Bytecode &bc);
}
