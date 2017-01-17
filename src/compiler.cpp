#include <iostream>
#include "compiler.h"

BfNode::~BfNode() {

}

BfBlock::BfBlock() {

}

BfBlock::~BfBlock() {
    for (auto itr = children.begin(); itr != children.end(); ++itr) {
        delete (*itr);
    }
}

BfOp::BfOp() {

}

BfOp::~BfOp() {
}

BfOp::BfOp(char op, size_t line, size_t pos) : line(line), pos(pos) {
    switch (op) {
        case '+':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_ADD, 0, 0, 1);
            break;
        case '-':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_ADD, 0, 0, -1);
            break;
        case '>':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_SFT, 0, 0, 1);
            break;
        case '<':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_SFT, 0, 0, -1);
            break;
        case '.':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_PUT, 0, 0, 0);
            break;
        case ',':
            opcode = BfOp::Opcode(BfOp::Opcode::BF_GET, 0, 0, 0);
            break;
    }
}

BfCompiler::BfCompiler() {

}

BfCompiler::~BfCompiler() {
    
}

BfProgram BfCompiler::CompileFile(const char* filename) {
    std::ifstream fstream(filename);
    BfBlock* main_block = CompileBlock(false, fstream);
    return BfProgram(main_block);

}

BfBlock* BfCompiler::CompileBlock(bool is_loop, std::istream& stream) {
    bool compiling = true;
    BfBlock* result = new BfBlock();
    result->SetCycle(is_loop);
    char op;
    while (stream.get(op)) {
        if (op == '[') {
            BfBlock* loop = CompileBlock(true, stream);
            result->Append(loop);
        } else if (op == ']') {
            if (is_loop) {
                compiling = false;
                break;
            } else {
                // TODO: error here
            }
            state.pos++;
        } else if (op == '+' || op == '-' || op == '>' || op == '<' || op == '.' || op == ',') {
            result->Append(new BfOp(op, state.line, state.pos));
            state.pos++;
        } else if (op == '\n') {
            state.line++;
            state.pos = 0;
        }
    }
    return result;
}
