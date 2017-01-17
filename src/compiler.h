#ifndef _BF_COMPILER_H_
#define _BF_COMPILER_H_

#include <fstream>
#include <list>
#include <ostream>
#include <stdlib.h>

#include "utils.h"

class BfNode {
    public:
        enum Type {
            OP,
            BLOCK
        };
        virtual ~BfNode() = 0;
        virtual Type GetType() = 0;
        virtual void Print(size_t indent) = 0;
};

class BfBlock : public BfNode {
    private:
        bool cycle;

    public:
        std::list<BfNode*> children;

        BfBlock();
        virtual ~BfBlock() override;

        void SetCycle(bool r) {
            cycle = r;
        }

        bool IsCycle() const {
            return cycle;
        }

        void Append(BfNode* node) {
            children.push_back(node);
        }

        BfNode::Type GetType() override {
            return BfNode::Type::BLOCK;
        }

        virtual void Print(size_t indent) override {
            print_indent(indent);
            if (cycle) {
                std::cout << "while ";
            }
            std::cout << "{" << std::endl;
            for (auto itr = children.begin(); itr != children.end(); ++itr) {
                (*itr)->Print(indent + 1);
            }
            print_indent(indent); std::cout << "}" << std::endl;
        }
};

class BfOp : public BfNode {
    private:
        size_t line;
        size_t pos;

    public:
        struct Opcode {
            enum Type : uint8_t {
                BF_ADD = 0x01,
                BF_MUL = 0x02,
                BF_SFT = 0x03,
                BF_PUT = 0x04,
                BF_GET = 0x05,
                BF_SET = 0x06,
                BF_NOP = 0x07,
            } opcode;

            int32_t src;
            int32_t dst;
            int32_t arg;

            Opcode() : opcode(BF_NOP), src(0), dst(0), arg(0) {}
            Opcode(Type opcode, int32_t src, int32_t dst, int32_t arg) : opcode(opcode), src(src), dst(dst), arg(arg) {}

            friend std::ostream& operator<<(std::ostream& stream, const Opcode& op) {
                switch (op.opcode) {
                    case BF_ADD:
                        stream << "add " << op.arg << " (" << op.src << "->" << op.dst << ")";
                        break;
                    case BF_MUL:
                        stream << "mul " << op.arg << " (" << op.src << "->" << op.dst << ")";
                        break;
                    case BF_SFT:
                        stream << "sft " << op.arg;
                        break;
                    case BF_PUT:
                        stream << "put (" << op.src << ")";
                        break;
                    case BF_GET:
                        stream << "get (" << op.dst << ")";
                        break;
                    case BF_SET:
                        stream << "set " << op.arg << " (" << op.dst << ")";
                        break;
                    case BF_NOP:
                        stream << "nop";
                        break;
                }
                return stream;
            }
        };

        Opcode opcode;

        BfOp();
        BfOp(char op, size_t line, size_t pos);
        BfOp(const Opcode& opcode) : opcode(opcode) {};
        virtual ~BfOp() override;
        
        BfNode::Type GetType() override {
            return BfNode::Type::OP;
        }

        friend std::ostream& operator<<(std::ostream& stream, const BfOp& op) {
            return stream << op.opcode;
        }

        virtual void Print(size_t indent) override {
            print_indent(indent); std::cout << opcode << std::endl;
        }
};

class BfProgram {
    public:
        BfBlock* block;

        BfProgram(BfBlock* block) : block(block) {}

        ~BfProgram() {
            delete block;
        }

        void Print() {
            block->Print(0);
        }
};



class BfCompiler {
    private:
        struct CompileState {
            size_t line;
            size_t pos;

            CompileState() : line(0), pos(0) {};
        } state;

        BfBlock* CompileBlock(bool loop, std::istream& stream);

    public:
        BfCompiler();
        ~BfCompiler();

        BfProgram CompileFile(const char* filename);
};


#endif