#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bf.h"
#include "compiler.h"
#include "optimizer.h"
#include "vm.h"
#include "list.h"

void bf_state_init(bf_state* state) {
    state->filename = 0;
    state->code = 0;
    state->code_size = 0;
    state->fd = 0;
}

bf_state* bf_create_state() {
    bf_state* state = (bf_state*) malloc(sizeof(bf_state));
    return state;
}

void bf_state_read(bf_state* state, const char* filename) {
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

void bf_state_free(bf_state* state) {
    free(state->code);
    free(state);
}

bf_block* bf_create_block(bool is_loop, bool is_container) {
    bf_block* block = (bf_block*) malloc(sizeof(bf_block));
    block->first = NULL;
    block->last = NULL;
    block->next = NULL;
    block->prev = NULL;
    block->parent = NULL;
    if (is_container) {
        block->container = (bf_container*) malloc(sizeof(bf_container));
        block->container->first = NULL;
        block->container->last = NULL;
    } else {
        block->container = NULL;
    }
    block->is_container = is_container;
    block->is_loop = is_loop;
    block->is_linear = false;
    return block;
}

void bf_container_free(bf_container* container) {
    bf_block* ptr = container->first;
    while (ptr) {
        bf_block* next = ptr->next;
        bf_delete_block(ptr);
        ptr = next;
    }
    free(container);
}

void bf_delete_block(bf_block* block) {
    bf_op* ptr = block->first;
    while (ptr != NULL) {
        bf_op* next = ptr->next;
        bf_delete_op(ptr);
        ptr = next;
    }
    if (block->is_container) {
        bf_container_free(block->container);
    }
    free(block);
}

bf_op* bf_create_op(bf_op_type op, int count, int offset) {
    bf_op* op_ = (bf_op*) malloc(sizeof(bf_op));
    op_->op = op;
    op_->arg = count;
    op_->arg2 = 0;
    op_->offset = offset;
    op_->next = NULL;
    op_->prev = NULL;
    return op_;
}

bf_op* bf_op_copy(bf_op* op) {
    return bf_create_op(op->op, op->arg, op->offset);
}

bf_block* bf_block_copy(bf_block* block) {
    bf_block* copy_block = bf_create_block(block->is_loop, block->is_container);
    if (block->is_container) {
        bf_block* ptr = block->container->first;
        BF_LIST_FOREACH(block->container, ptr) {
            bf_block* copy = bf_block_copy(ptr);
            bf_list_append(copy_block->container, copy);
        }
    } else {
        bf_op* op;
        BF_LIST_FOREACH(block, op) {
            bf_op* copy = bf_op_copy(op);
            bf_list_append(copy_block, copy);
        }
    }
    return copy_block;
}

void bf_delete_op(bf_op* op) {
    free(op);
}

bf_program* bf_create_program() {
    return (bf_program*) malloc(sizeof(bf_program));
}

void bf_program_free(bf_program* program) {
    bf_delete_block(program->container);
    free(program);
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
                bf_list_append(container->container, block);

                new_container = bf_create_block(true, true);
                new_container->parent = container;
                new_block = bf_create_block(false, false);

                block = new_block;
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
    if (block->first) {
        bf_list_append(container->container, block);
    } else {
        bf_delete_block(block);
    }
    return program;
}

void bf_print_op(bf_op* op) {
    if (op == NULL) {
        return;
    }
    
    switch (op->op) {
        case BF_ADD:
            printf("add %d (%d)", op->arg, op->offset);
            break;
        case BF_SFT:
            printf("sft %d (%d)", op->arg, op->offset);
            break;
        case BF_PUT:
            printf("put %d (%d)", op->arg, op->offset);
            break;
        case BF_GET:
            printf("get %d (%d)", op->arg, op->offset);
            break;
        case BF_MUL:
            printf("mul %d (%d->%d)", op->arg, op->arg2, op->offset);
            break;
        case BF_CLEAR:
            printf("clr (%d)", op->offset);
            break;
    }
}

void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
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
        print_indent(indent); printf("{\n");
        indent++;
        if (block->is_loop) {
            print_indent(indent); printf("while {\n");
            indent++;
        }
        if (block->is_container) {
            bf_print_block(block, indent + 1);
        } else {
            bf_print_ops(block, indent);
        }
        if (block->is_loop) {
            indent--;
            print_indent(indent); printf("}\n");
        }
        indent--;
        print_indent(indent); printf("}\n");
    }
    // if (!bf_is_list_empty(container)) {
    //     printf("\n");
    // }
}

void bf_compile_block(bf_vm* vm, bf_block* container) {
    bf_block* block;
    BF_LIST_FOREACH(container->container, block) {
        if (block->is_loop) {
            bf_vm_compile_loop_start(vm);
        }
        if (block->is_container) {
            bf_compile_block(vm, block);
        } else {
            bf_op* op;
            BF_LIST_FOREACH(block, op) {
                // printf("  "); bf_print_op(op); printf("\n");
                switch (op->op) {
                    case BF_ADD:
                        bf_vm_compile_add(vm, op->arg, op->offset);
                        break;
                    case BF_SFT:
                        bf_vm_compile_shift(vm, op->arg);
                        break;
                    case BF_PUT:
                        bf_vm_compile_put(vm, op->offset);
                        break;
                    case BF_GET:
                        bf_vm_compile_get(vm, op->offset);
                        break;
                    case BF_MUL:
                        bf_vm_compile_mul(vm, op->arg, op->arg2, op->offset);
                        break;
                    case BF_CLEAR:
                        bf_vm_compile_clear(vm, op->offset);
                        break;
                }       
            }
        }

        if (block->is_loop) {
            bf_vm_compile_loop_end(vm);
        }
    }
}

void bf_print_program(bf_program* program) {
    bf_print_block(program->container, 0);
    printf("=====\n\n");
}

void bf_compile_program(bf_program* program, bf_vm* vm) {
    bf_optimize_program(program);

    bf_vm_begin_jit(vm);
    bf_compile_block(vm, program->container);
    bf_vm_end_jit(vm);
}

bf_program* bf_program_from_str(char* str) {
    bf_state* state = bf_create_state();
    state->filename = "<string>";
    state->fd = 1;
    state->code = str;
    state->code_size = strlen(str);
    bf_program* program = bf_compile_state(state);
    bf_state_free(state);
    return program;
}

bf_vm* bf_compile_file(const char* filename) {
    bf_state* state = bf_create_state();
    bf_state_init(state); // TODO: error handling
    bf_state_read(state, filename);
    bf_program* program = bf_compile_state(state);

    if (bf_options.pretty_print) {
        bf_print_program(program);
    }

    bf_vm* vm = bf_vm_create();
    bf_compile_program(program, vm);
    bf_state_free(state);
    bf_program_free(program);
    return vm;
}
