#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <jit/jit.h>
#include "bf.h"
#include "vm.h"


int bf_vm_init(bf_vm* vm) {
    size_t sz = sizeof(cell_t) * MAX_CELLS;
    vm->data = (cell_t*)malloc(sz);
    memset(vm->data, 0, sz);
    vm->jit_context = jit_context_create();
    return 0;
}

bf_vm* bf_vm_create() {
    bf_vm* vm = (bf_vm*)malloc(sizeof(bf_vm));
    bf_vm_init(vm);
    return vm;
}

void bf_vm_free(bf_vm* vm) {
    free(vm->data);
    jit_type_free(vm->putchar_signature);
    jit_type_free(vm->getchar_signature);
    jit_context_destroy(vm->jit_context);
    free(vm);
}

void putchar2(int a) {
    printf("put %d (%c)\n", a, a);
}

char getchar2() {
    printf("> ");
    return getchar();
}

void bf_vm_compile_add(bf_vm* vm, int to_add, int offset) {
    jit_value_t diff = jit_value_create_nint_constant(vm->jit_function, jit_type_int, to_add);
    jit_value_t value = jit_insn_load_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), jit_type_int);
    jit_value_t new_value = jit_insn_add(vm->jit_function, value, diff);
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), new_value);
}

void bf_vm_compile_mul(bf_vm* vm, int mul_by, int offset) {
    jit_value_t factor = jit_value_create_nint_constant(vm->jit_function, jit_type_int, mul_by);
    jit_value_t value = jit_insn_load_relative(vm->jit_function, vm->data_ptr, 0, jit_type_int);
    jit_value_t old_value = jit_insn_load_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), jit_type_int);
    jit_value_t mul_value = jit_insn_mul(vm->jit_function, value, factor);
    jit_value_t new_value = jit_insn_add(vm->jit_function, old_value, mul_value);
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), new_value);
}

void bf_vm_compile_shift(bf_vm* vm, int count) {
    jit_value_t new_data_ptr = jit_insn_add_relative(vm->jit_function, vm->data_ptr, count * sizeof(cell_t));
    jit_insn_store(vm->jit_function, vm->data_ptr, new_data_ptr);
}

void bf_vm_compile_clear(bf_vm* vm) {
    jit_value_t zero = jit_value_create_nint_constant(vm->jit_function, jit_type_int, 0);
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, 0, zero);
}

void bf_vm_compile_put(bf_vm* vm, int offset) {
    jit_value_t value = jit_insn_load_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), jit_type_int);
    if (bf_options.debug) {
        jit_insn_call_native(vm->jit_function, "putchar2", putchar2, vm->putchar_signature, &value, 1, JIT_CALL_NOTHROW);
    } else {
        jit_insn_call_native(vm->jit_function, "putchar", putchar, vm->putchar_signature, &value, 1, JIT_CALL_NOTHROW);
    }
}

void bf_vm_compile_get(bf_vm* vm, int offset) {
    jit_value_t returned_value;
    if (bf_options.debug) {
        returned_value = jit_insn_call_native(vm->jit_function, "getchar2", getchar2, vm->getchar_signature, NULL, 0, JIT_CALL_NOTHROW);
    } else {
        returned_value = jit_insn_call_native(vm->jit_function, "getchar", getchar, vm->getchar_signature, NULL, 0, JIT_CALL_NOTHROW);
    }
    jit_insn_store_relative(vm->jit_function, vm->data_ptr, offset * sizeof(cell_t), returned_value);
}

void bf_vm_compile_loop_start(bf_vm* vm) {
    bf_vm_loop* loop = (bf_vm_loop*)malloc(sizeof(bf_vm_loop));

    loop->start_label = jit_label_undefined;
    loop->end_label = jit_label_undefined;

    if (!vm->current_loop) {
        vm->current_loop = loop;
    } else {
        loop->parent = vm->current_loop;
        vm->current_loop = loop;
    }

    jit_insn_label(vm->jit_function, &loop->start_label);
    jit_value_t value = jit_insn_load_relative(vm->jit_function, vm->data_ptr, 0, jit_type_int);
    jit_insn_branch_if_not(vm->jit_function, value, &loop->end_label);
}

void bf_vm_compile_loop_end(bf_vm* vm) {
    if (!vm->current_loop) {
        // TODO: fail here
    }
    bf_vm_loop* loop = vm->current_loop;
    jit_insn_branch(vm->jit_function, &loop->start_label);
    jit_insn_label(vm->jit_function, &loop->end_label);
    vm->current_loop = vm->current_loop->parent;
    free(loop);
}

void bf_vm_begin_jit(bf_vm* vm) {
    jit_context_build_start(vm->jit_context);

    jit_type_t jit_type_int_ptr = jit_type_create_pointer(jit_type_int, 1);

    jit_type_t putchar_params[1] = { jit_type_int };
    vm->putchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, putchar_params, 1, 1);
    vm->getchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, NULL, 0, 1);

    jit_type_t params[1] = { jit_type_int_ptr };
    jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 0);
    vm->jit_function = jit_function_create(vm->jit_context, signature);
    jit_type_free(signature);

    vm->data_ptr = jit_value_get_param(vm->jit_function, 0);
}

void bf_vm_end_jit(bf_vm* vm) {
    jit_insn_return(vm->jit_function, NULL);

    if (bf_options.dump_jit) {
        jit_dump_function(stdout, vm->jit_function, "main_function");
    }

    jit_function_compile(vm->jit_function);
    jit_context_build_end(vm->jit_context);
}

int bf_vm_run(bf_vm* vm) {
    cell_t* arg = vm->data + MAX_CELLS / 2;
    void* args[1] = {&arg};
    jit_function_apply(vm->jit_function, args, NULL);
    return 0;
}
