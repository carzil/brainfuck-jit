#include <assert.h>
#include <iostream>
#include <jit/jit-dump.h>
#include <jit/jit-insn.h>
#include <jit/jit-value.h>
#include <jit/jit.h>
#include <stdio.h>
#include <string.h>
#include <unordered_map>

#include "bf.h"
#include "vm.h"

void putchar2(int a) {
    printf("put %d (%c)\n", a, a);
}

char getchar2() {
    printf("> ");
    return getchar();
}

BfVM::BfVM() {
    data = new cell_t[MAX_CELLS];
    memset(data, 0, MAX_CELLS * sizeof(cell_t));
    jit_context = jit_context_create();
}

BfVM::~BfVM() {
    jit_type_free(putchar_signature);
    jit_type_free(getchar_signature);
    jit_context_destroy(jit_context);
    delete[] data;
}

void BfVM::CompileProgram(const BfProgram& program) {
    BeginCompilation();
    CompileBlock(*program.block);
    for (auto pair : temporary_values) {
        CompileStoreValue(pair.second, pair.first);
    }
    EndCompilation();
}

void BfVM::CompileBlock(const BfBlock& block) {
    if (block.IsCycle()) {
        BeginLoop();
    }

    for (auto itr = block.children.begin(); itr != block.children.end(); ++itr) {
        if ((*itr)->GetType() == BfNode::Type::OP) {
            auto src = static_cast<BfOp*>(*itr)->opcode.src;
            auto dst = static_cast<BfOp*>(*itr)->opcode.dst;
            auto type = static_cast<BfOp*>(*itr)->opcode.opcode;

            if (type == BfOp::Opcode::BF_SFT) {
                auto arg = static_cast<BfOp*>(*itr)->opcode.arg;
                std::vector<std::pair<int, jit_value_t>> d;
                for (auto i = temporary_values.begin(); i != temporary_values.end(); ++i) {
                    if (i->first - arg < 0) {
                        CompileStoreValue(i->second, i->first);
                    } else {
                        d.push_back({ i->first - arg, i->second });
                    }
                }
                for (auto pair : d) {
                    if (temporary_values[pair.first] != 0) {
                        jit_insn_store(jit_function, temporary_values[pair.first], pair.second);
                    } else {
                        pair.second = CompileLoadValue(pair.first);
                    }
                }
                temporary_values.clear();
                CompileShift(static_cast<BfOp*>(*itr)->opcode.arg);
                for (auto pair : d) {
                    temporary_values.insert(pair);
                }
            } else {
                if (temporary_values.find(src) == temporary_values.end()) {
                    temporary_values[src] = CompileLoadValue(src);
                }
                if (temporary_values.find(dst) == temporary_values.end()) {
                    temporary_values[dst] = CompileLoadValue(dst);
                }
                CompileOpcodeCached(static_cast<BfOp*>(*itr)->opcode);
            }

        } else {
            CompileBlock(*static_cast<BfBlock*>(*itr));
        }
    }

    if (block.IsCycle()) {
        EndLoop();
    }
}

void BfVM::BeginCompilation() {
    jit_context_build_start(jit_context);

    jit_type_t jit_type_int_ptr = jit_type_create_pointer(jit_type_int, 1);

    jit_type_t putchar_params[1] = { jit_type_int };
    putchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, putchar_params, 1, 1);
    getchar_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, NULL, 0, 1);

    jit_type_t params[1] = { jit_type_int_ptr };
    jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 0);
    jit_function = jit_function_create(jit_context, signature);
    jit_function_set_optimization_level(jit_function, jit_function_get_max_optimization_level());
    jit_type_free(signature);

    data_ptr = jit_value_get_param(jit_function, 0);
}

void BfVM::EndCompilation() {
    jit_insn_return(jit_function, NULL);

    jit_dump_function(stdout, jit_function, "main_function");

    jit_function_compile(jit_function);
    jit_context_build_end(jit_context);
}

void BfVM::CompileOpcode(const BfOp::Opcode& op) {
    jit_value_t v, t;
    switch (op.opcode) {
        case BfOp::Opcode::BF_ADD:
            v = CompileLoadValue(op.src);
            v = CompileAdd(v, op.arg);
            CompileStoreValue(v, op.dst);
            break;
        case BfOp::Opcode::BF_MUL:
            v = CompileLoadValue(op.src);
            t = CompileLoadValue(op.dst);
            v = CompileMul(v, t, op.arg);
            CompileStoreValue(v, op.dst);
            break;
        case BfOp::Opcode::BF_SFT:
            CompileShift(op.arg);
            break;
        case BfOp::Opcode::BF_PUT:
            v = CompileLoadValue(op.src);
            CompilePut(v);
            break;
        case BfOp::Opcode::BF_GET:
            v = CompileGet();
            CompileStoreValue(v, op.dst);
            break;
        case BfOp::Opcode::BF_SET:
            v = CompileSet(op.arg);
            CompileStoreValue(v, op.dst);
            break;
        case BfOp::Opcode::BF_NOP:
            break;
    }
}

void BfVM::CompileOpcodeCached(const BfOp::Opcode& op) {
    jit_value_t v, src, dst;
    switch (op.opcode) {
        case BfOp::Opcode::BF_ADD:
            src = CompileLoadCached(op.src);
            dst = CompileLoadCached(op.dst);
            v = CompileAdd(src, op.arg);
            jit_insn_store(jit_function, dst, v);
            break;
        case BfOp::Opcode::BF_MUL:
            src = CompileLoadCached(op.src);
            dst = CompileLoadCached(op.dst);
            v = CompileMul(src, dst, op.arg);
            jit_insn_store(jit_function, dst, v);
            break;
        case BfOp::Opcode::BF_SFT:
            CompileShift(op.arg);
            break;
        case BfOp::Opcode::BF_PUT:
            src = CompileLoadCached(op.src);
            CompilePut(src);
            break;
        case BfOp::Opcode::BF_GET:
            dst = CompileLoadCached(op.dst);
            v = CompileGet();
            jit_insn_store(jit_function, dst, v);
            break;
        case BfOp::Opcode::BF_SET:
            dst = CompileLoadCached(op.dst);
            v = CompileSet(op.arg);
            jit_insn_store(jit_function, dst, v);
            break;
        case BfOp::Opcode::BF_NOP:
            break;
    }
}

jit_value_t BfVM::CompileLoadValue(int from) {
    return jit_insn_load_relative(jit_function, data_ptr, from * sizeof(cell_t), jit_type_int);
}

jit_value_t BfVM::CompileLoadCached(int from) {
    auto result = temporary_values.find(from);
    if (result != temporary_values.end()) {
        return result->second;
    } else {
        jit_value_t value = jit_insn_load_relative(jit_function, data_ptr, from * sizeof(cell_t), jit_type_int);
        temporary_values[from] = value;
        return value;
    }
}

void BfVM::CompileStoreValue(jit_value_t value, int dst) {
    jit_insn_store_relative(jit_function, data_ptr, dst * sizeof(cell_t), value);
}

jit_value_t BfVM::CompileAdd(jit_value_t src, int arg) {
    jit_value_t what = jit_value_create_nint_constant(jit_function, jit_type_int, arg);
    return jit_insn_add(jit_function, src, what);
}

void BfVM::CompileShift(int arg) {
    jit_value_t new_data_ptr = jit_insn_add_relative(jit_function, data_ptr, arg * sizeof(cell_t));
    jit_insn_store(jit_function, data_ptr, new_data_ptr);
}

void BfVM::CompilePut(jit_value_t value) {
    jit_insn_call_native(jit_function, "putchar", (void*)putchar, putchar_signature, &value, 1, JIT_CALL_NOTHROW);
}

jit_value_t BfVM::CompileGet() {
    return jit_insn_call_native(jit_function, "getchar", (void*)getchar, getchar_signature, NULL, 0, JIT_CALL_NOTHROW);
}

jit_value_t BfVM::CompileMul(jit_value_t value, jit_value_t old_value, int arg) {

    jit_value_t factor = jit_value_create_nint_constant(jit_function, jit_type_int, arg);
    jit_value_t mul_value = jit_insn_mul(jit_function, value, factor);
    jit_value_t new_value = jit_insn_add(jit_function, old_value, mul_value);

    return new_value;
}

jit_value_t BfVM::CompileSet(int arg) {
    return jit_value_create_nint_constant(jit_function, jit_type_int, arg);
}

void BfVM::BeginLoop() {
    Loop* loop = new Loop();

    if (!current_loop) {
        current_loop = loop;
    } else {
        loop->parent = current_loop;
        current_loop = loop;
    }

    jit_insn_label(jit_function, &loop->start_label);
    jit_value_t value = CompileLoadCached(0);
    jit_insn_branch_if_not(jit_function, value, &loop->end_label);
}

void BfVM::EndLoop() {
    if (!current_loop) {
        // TODO: fail here
    }
    Loop* loop = current_loop;
    jit_insn_branch(jit_function, &loop->start_label);
    jit_insn_label(jit_function, &loop->end_label);
    current_loop = current_loop->parent;
    delete loop;
}

int BfVM::Run() {
    cell_t* arg = data + MAX_CELLS / 2;
    void* args[1] = {&arg};
    jit_function_apply(jit_function, args, NULL);
    return 0;
};
