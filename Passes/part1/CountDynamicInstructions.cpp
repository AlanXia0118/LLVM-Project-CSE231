#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

using namespace llvm;
using namespace std;

namespace {
struct CountStaticInstructions : public FunctionPass {
    static char ID;
    CountStaticInstructions() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        map<string, int> instructionCounter;
        // Count static instructions
        for (inst_iterator it = inst_begin(F), end = inst_end(F); it != end; ++it) {
            instructionCounter[it->getOpcodeName()]++;
        } 
        // Output results of counting
        for (map<string, int>::iterator it = instructionCounter.begin(); it != instructionCounter.end(); ++it) {
            errs() << it -> first << '\t' << it -> second << '\n';            
        }

    return false;
  }
}; // end of struct TestPass
}  // end of anonymous namespace

char CountStaticInstructions::ID = 0;
// X(passName, DocString)
static RegisterPass<CountStaticInstructions> X("cse231-csi", "Developed to count static instructions",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
