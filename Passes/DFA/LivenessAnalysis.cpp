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
	class LivenessInfo: public Info {
	public:
		LivenessInfo() {}

		LivenessInfo(set<unsigned> s) {
			info_list = s;
		}

		~LivenessInfo() {}

		void print() {
			for(auto i = info_list.begin(), i_end = info_list.end(); i != i_end; ++i) {
				errs() << *i << '|' ;
			}
			errs() << '\n';	
		}

        set<unsigned> info_list;

		static bool equals(LivenessInfo * i1, LivenessInfo * i2) {
            if (i1->info_list == i2->info_list) {
                return true;
            }
            return false;
		}

		static void join(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result) {

			result->info_list.insert(info1->info_list.begin(), info1->info_list.end());
			result->info_list.insert(info2->info_list.begin(), info2->info_list.end());
			return;
		}

        static int remove(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result ) {
            set<unsigned> info_set = info2->info_list;
            set<unsigned> resSet = info1->info_list;

            for (auto i = info_set.begin(), i_end = info_set.end(); i != i_end; ++i ) {
                resSet.erase(*i);
            }

            result->info_list = resSet;

            return 1;
        }
		
	};




    class LivenessAnalysis : public DataFlowAnalysis<LivenessInfo, false> { 
	private:
        typedef std::pair<unsigned, unsigned> Edge;
        map<string, int> opname_map = {{"alloca", 1}, {"load", 1}, {"select", 1},
										 {"icmp", 1}, {"fcmp", 1}, {"getelementptr", 1},
										 {"br", 2}, {"switch", 2}, {"store", 2}, {"phi", 3}};

	public:
		LivenessAnalysis(LivenessInfo &bottom, LivenessInfo &initialState):
								   DataFlowAnalysis(bottom, initialState) {}

		~LivenessAnalysis() {}

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
															std::vector<LivenessInfo *> & Infos)
		{

            
            // unsigned op = (*I).getOpcode();
			string opname = I->getOpcodeName();
			int cat;
			if (I->isBinaryOp() ) {
				cat = 1;
			}
			else {
				cat = opname_map.count(opname) ? opname_map[opname]: 2;
			}

            // std::map<unsigned, Instruction *> IndexToInstr;
		    // std::map<Instruction *, unsigned> InstrToIndex;
		    // std::map<Edge, Info *> EdgeToInfo;
			LivenessInfo *info_in = new LivenessInfo();
            unsigned idx = InstrToIndex[I];
            set<unsigned> oprands_set;

            unsigned ii = 0;
            for (;ii < I->getNumOperands(); ++ii) {
                Instruction * inst = (Instruction *) I->getOperand(ii);
                if (InstrToIndex.count(inst) == 0) continue;
                else {
                    oprands_set.insert(InstrToIndex[inst]);
                }

            }


            for (size_t i = 0; i < IncomingEdges.size(); ++i) {
                unsigned src = IncomingEdges[i];
				Edge in_edge = std::make_pair(src, idx);
				LivenessInfo * info_of_edge = EdgeToInfo[in_edge];
				LivenessInfo::join(info_in, info_of_edge, info_in);
			}


			// For each category
			set<unsigned> index_set;
			if(cat == 1)
			{
				index_set.insert(idx);

				LivenessInfo::join(info_in, new LivenessInfo(oprands_set), info_in);
				LivenessInfo * index_info = new LivenessInfo(index_set);
				LivenessInfo::join(info_in, index_info, info_in );

				size_t ii = 0;
				for (; ii < OutgoingEdges.size(); ++ii ) {
					Infos.push_back(info_in);
				}
			}

			else if (cat == 2) {
				LivenessInfo * oprands_info = new LivenessInfo(oprands_set);
				LivenessInfo::join(info_in, oprands_info, info_in);

				size_t ii = 0;
				for (; ii < OutgoingEdges.size(); ++ii ) {
					Infos.push_back(info_in);
				}
			}

			else {
				Instruction * first_non_phi = I->getParent()->getFirstNonPHI();

				for (unsigned i = idx; i < InstrToIndex[first_non_phi]; i++ ) {
					index_set.insert(i);
				}

				unsigned first_non_phi_idx = InstrToIndex[first_non_phi];
				LivenessInfo * index_info = new LivenessInfo(index_set);
				LivenessInfo::remove(info_in, index_info, info_in);

				unsigned j = 0;
				for (; j < OutgoingEdges.size(); j++ ) {
					LivenessInfo * j_out = new LivenessInfo();
					set<unsigned> opr_set;

					for (unsigned i = idx; i < first_non_phi_idx; ++i ) {
						Instruction *inst = IndexToInstr[i];

						for (unsigned k = 0; k < inst->getNumOperands(); ++k) {
							Instruction *op_inst = (Instruction *) inst->getOperand(k);

							if (InstrToIndex.count(op_inst) != 0 ) {
								if (op_inst->getParent() == IndexToInstr[OutgoingEdges[j]]->getParent() ) {
									opr_set.insert(InstrToIndex[op_inst] );
									break;
								}
							}
						}
					}

					LivenessInfo * opr_info = new LivenessInfo(opr_set);
					LivenessInfo::join(info_in, opr_info, info_in );
					Infos.push_back(j_out);
				}
			}
		}


	};




	struct LivenessAnalysisPass : public FunctionPass {
	  static char ID;
	  LivenessAnalysisPass() : FunctionPass(ID) {}

	  bool runOnFunction(Function &F) override {
	  	LivenessInfo bottom;
        LivenessInfo initialState;
	  	LivenessAnalysis * rda_instance = new LivenessAnalysis(bottom, initialState);

	  	rda_instance -> runWorklistAlgorithm(&F);
	  	rda_instance -> print();
	  	
	    return false;
	  }
	}; // end of struct LivenessAnalysisPass
} // end of anonymous namespace






char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X("cse231-liveness", "Implementing liveness analysis",
                             						  false /* Only looks at CFG */,
                             						  false /* Analysis Pass */);