#ifndef VM_H_
#define VM_H_

#include <jit/jit.h>

#define MAX_CELLS 30000

typedef int cell_t;

struct _VM_loop {
    jit_label_t start_label;
    jit_label_t end_label;
    struct _VM_loop* parent;
};

typedef struct _VM_loop VM_loop;

typedef struct {
    char* filename;
    int fd;

    unsigned char* code;
    size_t code_size;

    cell_t* data;

    // JIT
    VM_loop* current_loop;

    jit_context_t jit_context;
    jit_value_t data_ptr;
    jit_function_t jit_function;

    jit_type_t putchar_signature;
    jit_type_t getchar_signature;

} VM;


VM* vm_create();
int vm_init(VM* vm);
void vm_free();
int vm_load_file(VM* vm, char* filename);
int vm_run(VM* vm);


#endif