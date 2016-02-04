#ifndef OPTIMIZER_H_
#define OPTIMIZER_H_

#include "compiler.h"

void bf_optimize_program(bf_program* program);
void bf_eliminate_dead_code(bf_block* container);
void bf_optimize_linear_loops(bf_block* container);
void bf_optimize_reorder(bf_block* container);

#endif