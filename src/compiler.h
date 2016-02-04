#ifndef COMPILER_H_
#define COMPILER_H_

#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    BF_ADD,
    BF_SFT,
    BF_PUT,
    BF_GET
} bf_op_type;

typedef struct {
    char* filename;
    int fd;

    unsigned char* code;
    size_t code_size;


} bf_state;

typedef struct _bf_op {
    bf_op_type op;
    int count;
    int offset;

    struct _bf_op* next;
    struct _bf_op* prev;
} bf_op;

typedef struct {
    struct _bf_block* first;
    struct _bf_block* last;
} _container;

typedef struct _bf_block {
    bool is_loop;
    bool is_container; // if true, no ops can present in this block, only other blocks

    bf_op* first;
    bf_op* last;

    _container* container;

    struct _bf_block* next;
    struct _bf_block* prev;
    struct _bf_block* parent;
} bf_block;

typedef struct {
    bf_block* container;
} bf_program;

bf_op* bf_create_op(bf_op_type op, int count, int offset);
void bf_delete_op();
bf_block* bf_create_block(bool is_loop, bool is_container);
void bf_delete_block(bf_block* block);

#endif