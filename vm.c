#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <jit/jit.h>
#include "vm.h"


int vm_init(VM* vm) {
    size_t sz = sizeof(cell_t) * MAX_CELLS;
    vm->data = (cell_t*)malloc(sz);
    memset(vm->data, 0, sz);
    vm->jit_context = jit_context_create();
    return 0;
}

VM* vm_create() {
    VM* vm = (VM*)malloc(sizeof(VM));
    vm_init(vm);
    return vm;
}

void vm_free(VM* vm) {
    free(vm->code);
    free(vm->data);
    jit_context_destroy(vm->jit_context);

    if (vm->fd >= 0) {
        close(vm->fd);
    }
}

int vm_open_file(VM* vm, char* filename) {
    vm->fd = open(filename, O_RDONLY);
    vm->filename = filename;
    if (vm->fd < 0) {
        printf("cannot open file '%s'\n", filename);
        return -1;
    }
    struct stat finfo;
    fstat(vm->fd, &finfo);
    vm->code = (char*)malloc(finfo.st_size);
    if (!vm->code) {
        printf("cannot read file '%s'\n", filename);
        return -1;
    }
    read(vm->fd, vm->code, finfo.st_size);
    vm->code_size = finfo.st_size;
    return vm->code_size;
}

jit_value_t vm_value_by_ptr(VM* vm) {
    return jit_insn_load_relative(vm->jit_function, vm->data_ptr, 0, jit_type_int);
}

void vm_compile_add(VM* vm, int to_add) {
    jit_value_t diff = jit_value_create_nint_constant(vm->jit_function, jit_type_int, to_add);
    jit_value_t value = vm_value_by_ptr(vm);
    jit_value_t new_value = jit_insn_add(vm->jit_function, value, diff);
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, 0, new_value);
}

void vm_compile_shift(VM* vm, int count) {
    jit_value_t new_data_ptr = jit_insn_add_relative(vm->jit_function, vm->data_ptr, count * sizeof(cell_t));
    jit_insn_store(vm->jit_function, vm->data_ptr, new_data_ptr);
}

void vm_compile_loop_start(VM* vm) {
    VM_loop* loop = (VM_loop*)malloc(sizeof(VM_loop));

    loop->start_label = jit_label_undefined;
    loop->end_label = jit_label_undefined;

    if (!vm->current_loop) {
        vm->current_loop = loop;
    } else {
        loop->parent = vm->current_loop;
        vm->current_loop = loop;
    }

    jit_insn_label(vm->jit_function, &loop->start_label);
    jit_value_t value = vm_value_by_ptr(vm);
    jit_insn_branch_if_not(vm->jit_function, value, &loop->end_label);
}

void vm_compile_loop_end(VM* vm) {
    if (!vm->current_loop) {
        // TODO: fail here
    }
    VM_loop* loop = vm->current_loop;
    jit_insn_branch(vm->jit_function, &loop->start_label);
    jit_insn_label(vm->jit_function, &loop->end_label);
    vm->current_loop = vm->current_loop->parent;
}

void putchar2(int a) {
    printf("put %d\n", a);
}

void vm_compile_put(VM* vm) {
    jit_value_t value = vm_value_by_ptr(vm);
    jit_insn_call_native(vm->jit_function, "putchar", putchar, vm->putchar_signature, &value, 1, JIT_CALL_NOTHROW);
}

void vm_compile_get(VM* vm) {
    jit_value_t returned_value = jit_insn_call_native(vm->jit_function, "getchar", getchar, vm->getchar_signature, NULL, 0, JIT_CALL_NOTHROW);
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, 0, returned_value);
}

void vm_prepare_jit(VM* vm) {
    jit_type_t jit_type_int_ptr = jit_type_create_pointer(jit_type_int, 1);

    jit_type_t putchar_params[1] = {jit_type_int};
    vm->putchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, putchar_params, 1, 1);
    vm->getchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, NULL, 0, 1);

    jit_type_t params[1] = {jit_type_int_ptr};
    jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 1);
    vm->jit_function = jit_function_create(vm->jit_context, signature);

    vm->data_ptr = jit_value_get_param(vm->jit_function, 0);
}

int vm_count_eq_ops(VM* vm, size_t* code_pos, char op) {
    int count = 0;
    while (*code_pos < vm->code_size && vm->code[*code_pos] == op) {
        (*code_pos)++;
        count++;
    }
    return count;
}

void vm_compile(VM* vm) {
    jit_context_build_start(vm->jit_context);

    vm_prepare_jit(vm);

    size_t code_pos = 0;
    while (code_pos < vm->code_size) {
        char command = vm->code[code_pos];
        switch (command) {
            case '+':
                vm_compile_add(vm, vm_count_eq_ops(vm, &code_pos, '+'));
                break;
            case '-':
                vm_compile_add(vm, -vm_count_eq_ops(vm, &code_pos, '-'));
                break;
            case '<':
                vm_compile_shift(vm, -vm_count_eq_ops(vm, &code_pos, '<'));
                break;
            case '>':
                vm_compile_shift(vm, vm_count_eq_ops(vm, &code_pos, '>'));
                break;
            case '[':
                vm_compile_loop_start(vm);
                code_pos++;
                break;
            case ']':
                vm_compile_loop_end(vm);
                code_pos++;
                break;
            case '.':
                vm_compile_put(vm);
                code_pos++;
                break;
            case ',':
                vm_compile_get(vm);
                code_pos++;
                break;
            default:
                code_pos++;
                break;
        }
    }
    jit_insn_return(vm->jit_function, NULL);
    jit_dump_function(stdout, vm->jit_function, "main_function");
    jit_function_compile(vm->jit_function);
    jit_context_build_end(vm->jit_context);
}

int vm_run(VM* vm) {
    vm_compile(vm);
    int* arg = vm->data + MAX_CELLS / 2;
    void* args[1] = {&arg};
    jit_function_apply(vm->jit_function, args, NULL);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("specify file to run\n");
        return 1;
    }
    VM* vm = vm_create();
    vm_open_file(vm, argv[1]);
    vm_run(vm);
    vm_free(vm);
    return 0;
}