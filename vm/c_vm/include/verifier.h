#ifndef VERIFIER_H
#define VERIFIER_H

#include "bytecode.h"

/* returns NULL on success or pointer to static error string */
const char *verify_bytecode(const Bytecode *bc);

#endif
