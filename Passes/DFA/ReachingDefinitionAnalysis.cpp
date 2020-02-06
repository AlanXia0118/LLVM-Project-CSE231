#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"
#include <cstdint>
#include "231DFA.h"
#include <vector>
#include <map>
#include <set>


using namespace llvm;
using namespace std;

namespace llvm {
	class ReachingInfo: public Info {
	public:
		ReachingInfo() {}
		~ReachingInfo() {}

        set<unsigned> info_list;

		void print() {
			for(auto i = info_list.begin(), i_end = info_list.end(); i != i_end; ++i) {
				errs() << *i << '|' ;
			}
			errs() << '\n';	
		}

		static bool equals(ReachingInfo * i1, ReachingInfo * i2) {
            if (i1->info_list == i2->info_list) {
                return true;
            }
            return false;
		}

		static void join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result) {

			result->info_list.insert(info1->info_list.begin(), info1->info_list.end());
			result->info_list.insert(info2->info_list.begin(), info2->info_list.end());
			return;
		}

		
	};


	class ReachingDefinitionAnalysis : public DataFlowAnalysis<ReachingInfo, true> { 
	private:
		map<string, int> opname_map = {{"alloca", 1}, {"load", 1}, {"select", 1},
										 {"icmp", 1}, {"fcmp", 1}, {"getelementptr", 1},
										 {"br", 2}, {"switch", 2}, {"store", 2}, {"phi", 3}};
	public:
		ReachingDefinitionAnalysis(ReachingInfo &InitialStates, ReachingInfo &Bottom):
								   DataFlowAnalysis<ReachingInfo, true>(InitialStates, Bottom) {}

		~ReachingDefinitionAnalysis() {}

	    /*
     	* The flow function.
     	*   Instruction I: the IR instruction to be processed.
     	*   std::vector<unsigned> & IncomingEdges: the vector of the indices of the source instructions of the incoming edges.
     	*   std::vector<unsigned> & OutgoingEdges: the vector of indices of the source instructions of the outgoing edges.
     	*   std::vector<Info *> & Infos: the vector of the newly computed information for each outgoing eages.
     	*
     	* Direction:
     	* 	 Implement this function in subclasses.
     	*/
		void flowfunction(Instruction * I,
    													std::vector<unsigned> & IncomingEdges,
															std::vector<unsigned> & OutgoingEdges,
															std::vector<ReachingInfo *> & Infos)
		{
			ReachingInfo *info_in = new ReachingInfo();
            unsigned idx = InstrToIndex[I];
            for (size_t i = 0; i < IncomingEdges.size(); ++i) {
                unsigned src = IncomingEdges[i];
				Edge in_edge = std::make_pair(src, idx);
				ReachingInfo * info_of_edge = EdgeToInfo[in_edge];
				ReachingInfo::join(info_in, info_of_edge, info_in);
			}

			// unsigned op = (*I).getOpcode();
			string opname = I->getOpcodeName();
			int cat;
			if (I->isBinaryOp() ) {
				cat = 1;
			}
			else {
				cat = opname_map.count(opname) ? opname_map[opname]: 2;
			}

            ReachingInfo* info_idx = new ReachingInfo();

			if(cat == 1)
			{
				info_idx->info_list.insert(idx);
			}
			else if(cat == 3) {
				BasicBlock* blk = I->getParent();
          		for (auto it = blk->begin(), i_end = blk->end(); it != i_end; ++it) {
            		Instruction * inst = &*it;
            		if (!isa<PHINode>(inst)){
                        continue;
            		}
                    info_idx->info_list.insert(InstrToIndex[inst]);
          		}
			}
            
            for (size_t i = 0; i < OutgoingEdges.size(); ++i) {
                // unsigned dst = OutgoingEdges[i];
	  			// Edge out_edge = std::make_pair(idx, dst);
	  			ReachingInfo* tmp_info_of_edge = new ReachingInfo();
				ReachingInfo::join(info_in, info_idx, tmp_info_of_edge);
				Infos.push_back(tmp_info_of_edge);
			}

			return;
		}

	};


	struct ReachingDefinitionAnalysisPass : public FunctionPass {
	  static char ID;
	  ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}

	  bool runOnFunction(Function &F) override {
	  	ReachingInfo Bottom;
        ReachingInfo InitialStates;
	  	ReachingDefinitionAnalysis * rda_instance = new ReachingDefinitionAnalysis(InitialStates, Bottom);

	  	rda_instance -> runWorklistAlgorithm(&F);
	  	rda_instance -> print();
	  	
	    return false;
	  }
	}; // end of struct ReachingDefinitionAnalysisPass
} // end of anonymous namespace

char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X("cse231-reaching", "Implementing reaching definition analysis",
                             						  false /* Only looks at CFG */,
                             						  false /* Analysis Pass */);