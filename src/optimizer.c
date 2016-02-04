#include "compiler.h"
#include "list.h"

void bf_optimize_program(bf_program* program) {
    bf_eliminate_dead_code(program->container);
    bf_optimize_reorder(program->container);
    bf_optimize_linear_loops(program->container);
}

void bf_eliminate_dead_code(bf_block* container) {
    bf_block* block;
    bf_block* tmp;
    BF_LIST_FOREACH_SAFE(container, block, tmp) {
        if (block->is_container) {
            bf_eliminate_dead_code(block);
        } else {
            if (bf_is_list_empty(block)) {
                bf_list_remove(container, block);
                bf_delete_block(block);
            }
        }
    }
}


void bf_optimize_reorder(bf_block* container) {
    bf_block* block;
    BF_LIST_FOREACH(container, block) {
        if (block->is_container) {
            bf_optimize_reorder(container);
        } else {
            int offset = 0;
            bf_op* tmp;
            bf_op* op;
            BF_LIST_FOREACH_SAFE(block, op, tmp) {
                switch (op->op) {
                    case BF_ADD:
                    case BF_PUT:
                    case BF_GET:
                        op->offset = offset;
                        break;
                    case BF_SFT:
                        bf_list_remove(block, op);
                        offset += op->count;
                        break;
                }       
            }
            if (offset) {
                bf_list_append(block, bf_create_op(BF_SFT, offset, 0));
            }
        }
    }
}

void bf_optimize_linear_loops(bf_block* container) {

}