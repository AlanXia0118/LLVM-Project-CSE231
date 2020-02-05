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
struct CountDynamicInstructions : public FunctionPass {
  static char ID;
  CountDynamicInstructions() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
      Module *module = F.getParent();
      LLVMContext &Context = module->getContext();

      for(Function::iterator blk = F.begin(), blk_end = F.end(); blk != blk_end; ++blk) {
            map<int,int> instCount;

            
            for(BasicBlock::iterator Instru = blk->begin(), Instru_end = blk->end(); Instru != Instru_end; ++Instru){
                instCount[(int) Instru -> getOpcode()]++;
            }

            int InstruSize = instCount.size();
            vector<Value *> args;
            vector<Constant *> keys;
            vector<Constant *> vals;

            for(map<int,int>::iterator it = instCount.begin(), it_end = instCount.end(); it != it_end; ++it){
                keys.push_back(ConstantInt::get(Type::getInt32Ty(Context),it->first));
                vals.push_back(ConstantInt::get(Type::getInt32Ty(Context),it->second));
            }

            ArrayType* ArrTy = ArrayType::get(IntegerType::getInt32Ty(Context), InstruSize);
            GlobalVariable *KeysGlobal = new GlobalVariable(
                *module,
                ArrTy,
                true,
                GlobalVariable::InternalLinkage,
                ConstantArray::get(ArrTy,keys),
                "KeysGlobal");
            GlobalVariable *ValsGlobal = new GlobalVariable(
                *module,
                ArrTy,
                true,
                GlobalVariable::InternalLinkage,
                ConstantArray::get(ArrTy,vals),
                "ValsGlobal");

            IRBuilder<> Builder(&*blk);
            Builder.SetInsertPoint(blk->getTerminator());
            args.push_back((Value*)ConstantInt::get(Type::getInt32Ty(Context),InstruSize));
            args.push_back(Builder.CreatePointerCast(KeysGlobal, Type::getInt32PtrTy(Context)));
            args.push_back(Builder.CreatePointerCast(ValsGlobal, Type::getInt32PtrTy(Context)));

            FunctionCallee UpdateInstrInfo = module->getOrInsertFunction(
                "updateInstrInfo",          
                Type::getVoidTy(Context),   
                Type::getInt32Ty(Context),  
                Type::getInt32PtrTy(Context), 
                Type::getInt32PtrTy(Context)  
            );
            Builder.CreateCall(UpdateInstrInfo, args);

            for(BasicBlock::iterator bblk = blk->begin(), bblk_end = blk->end(); bblk != bblk_end; ++bblk){
                if ((string)bblk->getOpcodeName() == "ret"){
                    Builder.SetInsertPoint(&*bblk);

                    FunctionCallee PrintOutInstrInfo = module->getOrInsertFunction(
                        "printOutInstrInfo", // function name
                        Type::getVoidTy(Context) // return type
                    );

                    Builder.CreateCall(PrintOutInstrInfo);
                }
            }
      }
      return false;
  }
}; // end of struct CountDynamicInstructions
}  // end of anonymous namespace

char CountDynamicInstructions::ID = 0;
static RegisterPass<CountDynamicInstructions> X("cse231-cdi", "Dynamic Intructions Counting Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);