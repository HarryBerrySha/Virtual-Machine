#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "bytecode.h"
#include <stdio.h>

void disassemble_bytecode(const Bytecode *bc, FILE *os);

#endif
