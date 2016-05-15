#ifndef _BF_COMPILER_H_
#define _BF_COMPILER_H_

#include <stdlib.h>
#include <stdbool.h>
#include "vm.h"

typedef enum {
    BF_ADD,
    BF_SFT,
    BF_PUT,
    BF_GET,
    BF_CLEAR,
    BF_MUL
} bf_op_type;

typedef struct {
    const char* filename;
    int fd;

    char* code;
    size_t code_size;


} bf_state;

struct _bf_op {
    bf_op_type op;
    int arg;
    int offset;

    struct _bf_op* next;
    struct _bf_op* prev;
};

struct _bf_container {
    struct _bf_block* first;
    struct _bf_block* last;
};

struct _bf_block {
    bool is_loop;
    bool is_container; // if true, no ops can present in this block, only other blocks
    bool is_linear;

    struct _bf_op* first;
    struct _bf_op* last;

    struct _bf_container* container;

    struct _bf_block* next;
    struct _bf_block* prev;
    struct _bf_block* parent;
};

struct _bf_program {
    struct _bf_block* container;
};

typedef struct _bf_op bf_op;
typedef struct _bf_block bf_block;
typedef struct _bf_program bf_program;
typedef struct _bf_container bf_container;

bf_op* bf_create_op(bf_op_type op, int count, int offset);
void bf_delete_op();
bf_block* bf_create_block(bool is_loop, bool is_container);
void bf_delete_block(bf_block* block);
bf_block* bf_block_copy(bf_block* block);

bf_vm* bf_compile_file(const char* filename);

void bf_print_op(bf_op* op);
void bf_print_ops(bf_block* block, int indent);
void bf_print_block(bf_block* container, int indent);
void bf_print_program(bf_program* program);


#endif