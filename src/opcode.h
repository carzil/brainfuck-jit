#ifndef _OPCODE_H_
#define _OPCODE_H_

typedef struct bf_op {
    enum {
        OP_LOOP,
        OP_PLUS,
        OP_MINUS,
        OP_SHR,
        OP_SHL,
        OP_PUT,
        OP_GET,
        OP_END,
    } type;
    int count;
    char command;
    struct bf_op* next;
} bf_op_t;

typedef struct bf_block {
    enum {
        BLOCK_NORMAL,
        BLOCK_LOOP,
    } type;
    struct bf_block* first_child_block;
    struct bf_block* parent;
    struct bf_block* next;
    struct bf_op* first_op;
} bf_block_t;

#endif