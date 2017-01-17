#include <iostream>
#include <vector>
#include "optimizer.h"

void BfOptimizeOps::Visit(BfBlock* block) {
    for (auto itr = block->children.begin(); itr != block->children.end();) {
        BfNode* node = *itr;
        if (node->GetType() == BfNode::Type::BLOCK) {
            BfOptimizerPass::Visit(node);
            ++itr;
        } else {
            BfOp* op = static_cast<BfOp*>(node);
            if (++itr != block->children.end()) {
                BfNode* next = *itr;
                if (next->GetType() == BfNode::Type::OP) {
                    BfOp* next_op = static_cast<BfOp*>(next);
                    if (op->opcode.opcode == next_op->opcode.opcode) {
                        if (op->opcode.opcode == BfOp::Opcode::BF_ADD) {
                            if (op->opcode.src == next_op->opcode.src && op->opcode.dst == next_op->opcode.dst) {
                                op->opcode.arg += next_op->opcode.arg;
                                delete *itr;
                                block->children.erase(itr++);
                                --itr;
                            }
                        } else if (op->opcode.opcode == BfOp::Opcode::BF_MUL) {
                            if (op->opcode.src == next_op->opcode.src && op->opcode.dst == next_op->opcode.dst) {
                                op->opcode.arg *= next_op->opcode.arg;
                                delete *itr;
                                block->children.erase(itr++);
                                --itr;
                            }
                        } else if (op->opcode.opcode == BfOp::Opcode::BF_SFT) {
                            op->opcode.arg += next_op->opcode.arg;
                            delete *itr;
                            block->children.erase(itr++);
                            --itr;
                        } else if (op->opcode.opcode == BfOp::Opcode::BF_SET) {
                            delete *itr;
                            block->children.erase(itr++);
                            --itr;
                        }
                    }
                }
            } else {
                break;
            }
        }
    }
}

void BfOptimizeReorder::Visit(BfBlock* block) {
    int offset = 0;
    for (auto itr = block->children.begin(); itr != block->children.end(); ++itr) {
        BfNode* node = *itr;
        if (node->GetType() == BfNode::Type::BLOCK) {
            if (offset) {
                block->children.insert(itr, new BfOp(BfOp::Opcode(BfOp::Opcode::BF_SFT, 0, 0, offset)));
            }
            offset = 0;
            BfOptimizerPass::Visit(node);
        } else {
            BfOp* op = static_cast<BfOp*>(node);
            switch (op->opcode.opcode) {
                case BfOp::Opcode::BF_SFT:
                    offset += op->opcode.arg;
                    delete *itr;
                    block->children.erase(itr++); itr--;
                    break;
                default:
                    op->opcode.src += offset;
                    op->opcode.dst += offset;
                    break;
            }
        }
    }
    // std::cout << "=== [ optimization ] ===" << std::endl;
    // node->Print(0);
    // std::cout << "--- [ into ] ---" << std::endl;
    // block->Print(0);
    // std::cout << "========================" << std::endl << std::endl;
    if (offset) {
        block->children.push_back(new BfOp(BfOp::Opcode(BfOp::Opcode::BF_SFT, 0, 0, offset)));
    }
}

void BfOptimizeSimpleLoop::Visit(BfBlock* block) {
    bool optimizable = block->IsCycle();
    int offset = 0;
    int loop_decrement = 0;
    for (auto itr = block->children.begin(); itr != block->children.end(); ++itr) {
        BfNode* node = *itr;
        if (node->GetType() == BfNode::Type::BLOCK) {
            BfOptimizerPass::Visit(node);
            optimizable = false;
            BfBlock* bl = static_cast<BfBlock*>(node);
            if (!bl->IsCycle() && !block->IsCycle()) {
                bool can_merge = true;
                for (BfNode* n : bl->children ) {
                    if (n->GetType() == BfNode::Type::BLOCK) {
                        can_merge = false;
                    }
                }

                if (can_merge) {
                    // std::cout << "=== [ optimization ] ===" << std::endl;
                    // block->Print(0);
                    // bl->Print(0);
                    block->children.erase(itr++);
                    for (auto i = bl->children.begin(); i != bl->children.end();) {
                        BfOp* op = static_cast<BfOp*>(*i);
                        // op->opcode.src += offset;
                        // op->opcode.dst += offset;
                        block->children.insert(itr, *i);
                        bl->children.erase(i++);
                    }
                    delete bl;
                    itr--;
                    // std::cout << "--- [ into ] ---" << std::endl;
                    // block->Print(0);
                    // std::cout << "========================" << std::endl << std::endl;
                }
            } else {
                offset = 0;
            }
        } else {
            BfOp* op = static_cast<BfOp*>(node);
            switch (op->opcode.opcode) {
                case BfOp::Opcode::BF_SFT:
                    offset += op->opcode.arg;
                    break;
                case BfOp::Opcode::BF_ADD:
                    if (op->opcode.dst == 0) {
                        loop_decrement += op->opcode.arg;
                    }
                    break;
                case BfOp::Opcode::BF_MUL:
                case BfOp::Opcode::BF_PUT:
                case BfOp::Opcode::BF_GET:
                case BfOp::Opcode::BF_SET: // TODO: BF_SET and BF_MUL can be optimized in loops
                    optimizable = false;
                    break;
                default:
                    break;
            }
        }
    }

    if (optimizable && offset == 0 && loop_decrement == -1) {
        block->SetCycle(false);
        for (auto itr = block->children.begin(); itr != block->children.end(); ++itr) {
            BfOp* op = static_cast<BfOp*>(*itr);
            switch (op->opcode.opcode) {
                case BfOp::Opcode::BF_ADD:
                    if (op->opcode.dst != 0) {
                        op->opcode = BfOp::Opcode(BfOp::Opcode::BF_MUL, 0, op->opcode.dst, op->opcode.arg);
                    } else {
                        delete *itr;
                        block->children.erase(itr++);
                        itr--;
                    }
                    break;
            }
        }
        block->children.push_back(new BfOp(BfOp::Opcode(BfOp::Opcode::BF_SET, 0, 0, 0)));
    }
}

void BfOptimizeConstants::Visit(BfBlock* block) {

}

void BfOptimizer::Optimize(BfProgram& program, int level) {
    std::vector<BfOptimizerPass*> passes;

    passes = { 
        new BfOptimizeOps(),
        new BfOptimizeReorder(),
        new BfOptimizeSimpleLoop(),
        new BfOptimizeOps()
    };

    for (size_t i = 0; i < passes.size(); i++) {
        BfOptimizerPass* pass = passes[i];
        pass->Visit(program.block);
        delete pass;
    }

    // program.Print();
}

BfOptimizer::~BfOptimizer() {

}