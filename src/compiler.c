#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "compiler.h"
#include "optimizer.h"
#include "vm.h"
#include "list.h"

void bf_state_init(bf_state* state) {

}

bf_state* bf_create_state() {
    bf_state* state = (bf_state*) malloc(sizeof(bf_state));
    return state;
}

void bf_state_read(bf_state* state, char* filename) {
    state->fd = open(filename, O_RDONLY);
    state->filename = filename;
    if (state->fd < 0) {
        printf("cannot open file '%s'\n", filename);
    }
    struct stat finfo;
    fstat(state->fd, &finfo);
    state->code = (char*)malloc(finfo.st_size);
    if (!state->code) {
        printf("cannot read file '%s'\n", filename);
    }
    read(state->fd, state->code, finfo.st_size);
    state->code_size = finfo.st_size;
}

bf_program* bf_create_program() {
    return (bf_program*) malloc(sizeof(bf_program));
}

bf_block* bf_create_block(bool is_loop, bool is_container) {
    bf_block* block = (bf_block*) malloc(sizeof(bf_block));
    block->first = NULL;
    block->last = NULL;
    block->next = NULL;
    block->prev = NULL;
    block->parent = NULL;
    if (is_container) {
        block->container = (_container*) malloc(sizeof(_container));
        block->container->first = NULL;
        block->container->last = NULL;
    } else {
        block->container = NULL;
    }
    block->is_container = is_container;
    block->is_loop = is_loop;
}

void bf_delete_block(bf_block* block) {
    bf_op* ptr = block->first;
    while (ptr != NULL) {
        bf_op* next = ptr->next;
        bf_delete_op(ptr);
        ptr = next;
    }
    free(block->container);
    free(block);
}

bf_op* bf_create_op(bf_op_type op, int count, int offset) {
    bf_op* op_ = (bf_op*) malloc(sizeof(bf_op));
    op_->op = op;
    op_->count = count;
    op_->offset = offset;
    op_->next = NULL;
    op_->prev = NULL;
    return op_;
}

void bf_delete_op(bf_op* op) {
    free(op);
}

int bf_count_eq_ops(bf_state* state, size_t* code_pos, char op) {
    int count = 0;
    while (*code_pos < state->code_size && state->code[*code_pos] == op) {
        (*code_pos)++;
        count++;
    }
    return count;
}

bf_program* bf_compile_state(bf_state* state) {
    size_t code_pos = 0;
    bf_program* program = bf_create_program();
    bf_block* block = bf_create_block(false, false);
    program->container = bf_create_block(false, true);
    bf_block* container = program->container;
    bf_list_append(container->container, block);
    int count;
    bf_op* new_op;
    bf_block* new_block;
    bf_block* new_container;
    while (code_pos < state->code_size) {
        char op = state->code[code_pos];
        switch (op) {
            case '+':
                count = bf_count_eq_ops(state, &code_pos, op);
                new_op = bf_create_op(BF_ADD, count, 0);
                bf_list_append(block, new_op);
                break;
            case '-':
                count = bf_count_eq_ops(state, &code_pos, op);
                new_op = bf_create_op(BF_ADD, -count, 0);
                bf_list_append(block, new_op);
                break;
            case '<':
                count = bf_count_eq_ops(state, &code_pos, op);
                new_op = bf_create_op(BF_SFT, -count, 0);
                bf_list_append(block, new_op);
                break;
            case '>':
                count = bf_count_eq_ops(state, &code_pos, op);
                new_op = bf_create_op(BF_SFT, count, 0);
                bf_list_append(block, new_op);
                break;
            case '.':
                new_op = bf_create_op(BF_PUT, 1, 0);
                bf_list_append(block, new_op);
                code_pos++;
                break;
            case ',':
                new_op = bf_create_op(BF_GET, 1, 0);
                bf_list_append(block, new_op);
                code_pos++;
                break;
            case '[':
                new_container = bf_create_block(true, true);
                new_block = bf_create_block(false, false);
                bf_list_append(container->container, block);
                block = new_block;
                new_container->parent = container;
                bf_list_append(container->container, new_container);
                container = new_container;
                code_pos++;
                break;
            case ']':
                bf_list_append(container->container, block);
                container = container->parent;
                block = bf_create_block(false, false);
                code_pos++;
                break;
            default:
                code_pos++;
                break;
        }
    }
    bf_list_append(container->container, block);
    return program;
}

void bf_print_op(bf_op* op) {
    if (op == NULL) {
        return;

    }
    switch (op->op) {
        case BF_ADD:
            printf("add %d (%d)", op->count, op->offset);
            break;
        case BF_SFT:
            printf("sft %d (%d)", op->count, op->offset);
            break;
        case BF_PUT:
            printf("put %d (%d)", op->count, op->offset);
            break;
        case BF_GET:
            printf("get %d (%d)", op->count, op->offset);
            break;
    }
}

void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("    ");
    }
}

void bf_print_ops(bf_block* block, int indent) {
    bf_op* op;
    BF_LIST_FOREACH(block, op) {
        print_indent(indent); bf_print_op(op);
        printf("\n");
    }
}

void bf_print_block(bf_block* container, int indent) {
    bf_block* block;
    BF_LIST_FOREACH(container->container, block) {
        if (block->is_loop) {
            print_indent(indent); printf("while [\n");
        }
        if (block->is_container) {
            bf_print_block(block, indent + 1);
        } else {
            bf_print_ops(block, indent);
        }
        if (block->is_loop) {
            print_indent(indent); printf("]\n");
        }
    }
    if (!bf_is_list_empty(container)) {
        printf("\n");
    }
}

void bf_print_program(bf_program* program) {
    bf_print_block(program->container, 0);
    printf("=====\n\n");
}

bf_vm* bf_compile_program(bf_program* program) {
    bf_vm* vm = vm_create();
    vm_begin_jit(vm);
    bf_compile_block(vm, program->container);
    vm_end_jit(vm);
    return vm;
}

void bf_compile_block(bf_vm* vm, bf_block* container) {

    bf_block* block;
    bf_block* prev_block;
    BF_LIST_FOREACH(container->container, block) {
        if (block->is_loop) {
            vm_compile_loop_start(vm);
        }
        if (block->is_container) {
            bf_compile_block(vm, block);
        } else {
            bf_op* op;
            BF_LIST_FOREACH(block, op) {
                switch (op->op) {
                    case BF_ADD:
                        vm_compile_add(vm, op->count, op->offset);
                        break;
                    case BF_SFT:
                        vm_compile_shift(vm, op->count);
                        break;
                    case BF_PUT:
                        vm_compile_put(vm, op->offset);
                        break;
                    case BF_GET:
                        vm_compile_get(vm, op->offset);
                        break;
                }       
            }
        }

        if (block->is_loop) {
            vm_compile_loop_end(vm);
        }
    }
}

bf_vm* bf_compile_file(char* filename) {
    bf_state* state = bf_create_state();
    bf_state_init(state); // TODO: error handling
    bf_state_read(state, filename);
    bf_program* program = bf_compile_state(state);
    bf_optimize_program(program);
    bf_print_program(program);
    bf_vm* vm = bf_compile_program(program);
    return vm;
}
