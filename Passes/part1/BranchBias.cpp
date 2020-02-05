
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"

#include <string>
#include <map>
#include <vector>

using namespace std;
using namespace llvm;

namespace {
struct BranchBias: public FunctionPass {
    static char ID;
    BranchBias() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        Module* module = F.getParent();
        LLVMContext &context = module->getContext();

        FunctionCallee updateBranchInfoFunc = module->getOrInsertFunction(
            "updateBranchInfo",         // name of function
            Type::getVoidTy(context),   // return type
            Type::getInt1Ty(context)    // first parameter type
        );
        FunctionCallee printOutBranchInfoFunc = module->getOrInsertFunction(
            "printOutBranchInfo",        // name of function
            Type::getVoidTy(context)     // return type                   
        );


        for (Function::iterator blk = F.begin(), blk_end = F.end(); blk != blk_end; ++blk) {
            // blk: pointer to a basic block
            for (BasicBlock::iterator it = blk->begin(), it_end = blk->end(); it != it_end; ++it) {
                // it: pointer to an instruction
                int instCode = it->getOpcode();
                if ( 2 == instCode) {
                    BranchInst* brInst = cast<BranchInst>(it);
                    if (brInst->isConditional() ) {
                        IRBuilder<> Builder(brInst);
                        vector<Value *> args;
                        args.push_back(brInst->getCondition());
                        Builder.CreateCall(updateBranchInfoFunc, args);
                    }
                } 

            }
        }

        IRBuilder<> Builder(&(F.back().back()));
        Builder.CreateCall(printOutBranchInfoFunc);
        return true;
    }
}; // end of struct CountDymInstruPass
}  // end of anonymous namespace

char BranchBias::ID = 2;
static RegisterPass<BranchBias> X("cse231-bb", "Dynamic BrachBias Counting Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);