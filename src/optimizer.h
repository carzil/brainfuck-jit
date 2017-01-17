#ifndef _BF_OPTIMIZER_H_
#define _BF_OPTIMIZER_H_

#include "compiler.h"
#include "utils.h"

class BfOptimizerPass {
    public:
        virtual ~BfOptimizerPass() {}

        virtual void Visit(BfNode* node) {
            if (node->GetType() == BfNode::Type::BLOCK) {
                Visit(static_cast<BfBlock*>(node));
            }
        }

        virtual void Visit(BfBlock* block) = 0;
};

class BfOptimizeOps : public BfOptimizerPass {
    public:
        BfOptimizeOps() {}

        virtual void Visit(BfBlock* block) override;
};

class BfOptimizeReorder : public BfOptimizerPass {
    public:
        BfOptimizeReorder() {}

        virtual void Visit(BfBlock* block) override;
};

class BfOptimizeSimpleLoop : public BfOptimizerPass {
    public:
        BfOptimizeSimpleLoop() {}

        virtual void Visit(BfBlock* block) override;
};

class BfOptimizeConstants : public BfOptimizerPass {
    private:

    public:
        BfOptimizeConstants() {}

        virtual void Visit(BfBlock* block) override;
};

// class BfOptimizeLinearize : public BfOptimizerPass {
//     public:
//         BfOptimizeLinearize() {}

//         virtual void Visit(BfBlock* block) override;
// };

class BfOptimizer {
    public:
        BfOptimizer() {}
        ~BfOptimizer();

        void Optimize(BfProgram& program, int level);
};

#endif