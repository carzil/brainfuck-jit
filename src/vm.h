#ifndef bf_vm_H_
#define bf_vm_H_
#include <stdlib.h>
#include <jit/jit.h>

#define MAX_CELLS 30000

typedef int cell_t;

typedef struct {
    jit_label_t start_label;
    jit_label_t end_label;
    struct _bf_vm_loop* parent;
} bf_vm_loop;

typedef struct {
    cell_t* data;

    // JIT
    bf_vm_loop* current_loop;

    jit_context_t jit_context;
    jit_value_t data_ptr;
    jit_function_t jit_function;

    jit_type_t putchar_signature;
    jit_type_t getchar_signature;

} bf_vm;

bf_vm* vm_create();
int vm_init(bf_vm* vm);
void vm_free();
int vm_load_file(bf_vm* vm, char* filename);
int vm_run(bf_vm* vm);
void vm_begin_jit(bf_vm* vm);
void vm_end_jit(bf_vm* vm);
void vm_compile_add(bf_vm* vm, int to_add, int offset);
void vm_compile_shift(bf_vm* vm, int to_shift);
void vm_compile_put(bf_vm* vm, int offset);
void vm_compile_get(bf_vm* vm, int offset);
void vm_compile_loop_start(bf_vm* vm);
void vm_compile_loop_end(bf_vm* vm);


#endif