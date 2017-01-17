#ifndef _BF_VM_H_
#define _BF_VM_H_

#include <stdlib.h>
#include <stdint.h>
#include <ostream>
#include <jit/jit.h>
#include <vector>
#include "compiler.h"
#include <unordered_map>

#define MAX_CELLS 30000

typedef int cell_t;

class BfVM {
    public:
        BfVM();
        ~BfVM();

        void CompileProgram(const BfProgram& program);

        void LoadFile(const char* filename);
        int Run();

    private:
        struct Loop {
            jit_label_t start_label;
            jit_label_t end_label;
            Loop* parent;

            Loop() : start_label(jit_label_undefined), end_label(jit_label_undefined) {}
        };

        cell_t* data;

        Loop* current_loop;

        jit_context_t jit_context;
        jit_value_t data_ptr;
        jit_function_t jit_function;

        jit_type_t putchar_signature;
        jit_type_t getchar_signature;

        std::unordered_map<int, jit_value_t> temporary_values;

        void BeginCompilation();
        void EndCompilation();

        void CompileOpcode(const BfOp::Opcode&);
        void CompileOpcodeCached(const BfOp::Opcode& op);


        jit_value_t CompileLoadValue(int from);
        jit_value_t CompileLoadCached(int from);
        void CompileStoreValue(jit_value_t, int dst);
        jit_value_t CompileAdd(jit_value_t, int arg);
        jit_value_t CompileMul(jit_value_t, jit_value_t, int arg);
        void CompileShift(int);
        void CompilePut(jit_value_t);
        jit_value_t CompileGet();
        jit_value_t CompileSet(int arg);

        void BeginLoop();
        void EndLoop();

        void CompileBlock(const BfBlock& block);
};

#endif