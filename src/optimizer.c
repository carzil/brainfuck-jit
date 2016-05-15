#include <stdio.h>
#include "bf.h"
#include "compiler.h"
#include "list.h"

int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

void bf_eliminate_empty_blocks(bf_block* container) {
    bf_block* block;
    bf_block* tmp;
    BF_LIST_FOREACH_SAFE(container->container, block, tmp) {
        if (block->is_container) {
            bf_eliminate_empty_blocks(block);
        } else {
            if (bf_is_list_empty(block)) {
                bf_list_remove(container->container, block);
                bf_delete_block(block);
            }
        }
    }
}

bool bf_optimize_clear(bf_block* container, bool is_parent_loop) {
    bf_block* block;
    bf_block* tmp;
    BF_LIST_FOREACH_SAFE(container->container, block, tmp) {
        if (block->is_container && block->container->first == block->container->last) {
            if (bf_optimize_clear(block, block->is_loop)) {
                bf_op* new_op = bf_create_op(BF_CLEAR, 0, 0);
                bf_block* new_block = bf_create_block(false, false);
                bf_list_append(new_block, new_op);
                bf_list_replace(container->container, block, new_block);
                bf_delete_block(block);
            }

        } else if (is_parent_loop) {
            bf_op* op = block->first;
            if (op && !op->next) {
                // loop with single instruction
                if (op->op == BF_ADD && op->arg == -1) {
                    // we found [-], so we can replace it with clear
                    return true;
                }
            }
        }
    }
    return false;
}

/* This block-pass. For each block, we optimize shift operations by using offsets.
   [++>-->++] will be converted into:
   while {
       add 2, +0
       add -2, +1
       add 2, +2
       sft 2
   }
 */
void bf_optimize_reorder(bf_block* container) {
    bf_block* block;
    BF_LIST_FOREACH(container->container, block) {
        if (block->is_container) {
            bf_optimize_reorder(block);
        } else {
            int offset = 0;
            int max_offset = 0;
            bf_op* tmp;
            bf_op* op;
            BF_LIST_FOREACH_SAFE(block, op, tmp) {
                switch (op->op) {
                    case BF_ADD:
                    case BF_PUT:
                    case BF_GET:
                    case BF_CLEAR:
                        op->offset = offset;
                        max_offset = max(offset, max_offset);
                        break;
                    case BF_SFT:
                        bf_list_remove(block, op);
                        offset += op->arg;
                        bf_delete_op(op);
                        break;
                    case BF_MUL:
                        break;
                }       
            }
            if (offset) {
                bf_op* new_sft = bf_create_op(BF_SFT, offset, 0);
                bf_list_append(block, new_sft);
            }
        }
    }
}

void bf_make_block_linear(bf_block* block) {
    bf_op* tmp;
    bf_op* op;
    BF_LIST_FOREACH_SAFE(block, op, tmp) {
        switch (op->op) {
            case BF_ADD:
                if (op->offset != 0) {
                    bf_op* new_op = bf_create_op(BF_MUL, op->arg, op->offset);
                    bf_list_replace(block, op, new_op);
                } else {
                    bf_list_remove(block, op);
                }
                bf_delete_op(op);
            default:
                break;
        }
    }
    bf_op* new_op = bf_create_op(BF_CLEAR, 0, 0);
    bf_list_append(block, new_op);
}

bool bf_optimize_loops(bf_block* container) {
    bf_block* block;
    bf_block* tmp;
    BF_LIST_FOREACH_SAFE(container->container, block, tmp) {
        if (block->is_container) {
            bool is_optimizable = bf_optimize_loops(block);
            if (block->is_loop && is_optimizable && block->container->first && !block->container->first->next) {
                bf_block* copy = bf_block_copy(block->container->first);
                bf_make_block_linear(copy);
                bf_list_replace(container->container, block, copy);
                bf_delete_block(block);
            }
        } else {
            bf_op* tmp;
            bf_op* op;
            int offset = 0;
            int add = 0;
            bool no_io = true;
            BF_LIST_FOREACH_SAFE(block, op, tmp) {
                switch (op->op) {
                    case BF_ADD:
                        if (op->offset == 0) {
                            add += op->arg;
                        }
                        break;
                    case BF_SFT:
                        offset += op->arg;
                        break;
                    case BF_PUT:
                    case BF_GET:
                        no_io = false;
                        break;
                    default:
                        break;
                }       
            }
            if (no_io && offset == 0 && add == -1) {
                return true;
            }
        }
    }
    return false;
}

void bf_optimize_merge_ops(bf_block* container) {
    bf_block* block;
    BF_LIST_FOREACH(container->container, block) {
        if (block->is_container) {
            bf_optimize_merge_ops(block);
        } else {
            for (bf_op* op = block->first; op;) {
                switch(op->op) {
                    case BF_ADD:
                        if (op->next && op->next->op == BF_ADD && op->next->offset == op->offset) {
                            // we have two ADD-ops with same offset in block
                            // so we can merge them
                            bf_op* new_op = bf_create_op(BF_ADD, op->arg + op->next->arg, op->offset);
                            bf_list_replace(block, op, new_op);
                            bf_list_remove(block, op->next);
                            bf_delete_op(op->next);
                            bf_delete_op(op);
                            op = new_op;
                            break;
                        }
                    default:
                        op = op->next;
                        break;
                }
            }
        }
    }
}

void bf_optimize_program(bf_program* program) {
    printf("before optimization:\n");
    bf_print_program(program);
    bf_eliminate_empty_blocks(program->container);
    if (bf_options.optimize_level == 1) {
        bf_optimize_clear(program->container, program->container->is_loop);
    } else if (bf_options.optimize_level == 2) {
        bf_optimize_reorder(program->container);
        bf_optimize_clear(program->container, program->container->is_loop);
        bf_optimize_merge_ops(program->container);
    } else if (bf_options.optimize_level == 3) {
        bf_optimize_reorder(program->container);
        bf_optimize_merge_ops(program->container);
        bf_optimize_loops(program->container);
    }

    printf("after optimization:\n");
    bf_print_program(program);
}
