#include "231DFA.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/IR/ConstantFolder.h"
#include <unordered_map>


using namespace llvm;
using namespace std;

class ConstPropInfo : public Info {
	public:
        enum ConstState { Bottom, Const, Top };
        struct Const { ConstState state; Constant* value; };
        typedef unordered_map< Value*, struct Const > ConstPropContent;

		ConstPropInfo() {}
        ~ConstPropInfo() {}

        ConstPropInfo(ConstPropContent m) {
			const_map = m;
		}

        ConstPropContent const_map;

        void print() {
            // errs() << "PRINT begins" << "\n";
			for (auto r = const_map.begin(), r_end = const_map.end(); r != r_end; ++r) {
                Value* var = r->first;
                // determine if '', '', or constant
                ConstState state = r->second.state;
                errs() << var->getName() << "=";
                if (state == Bottom) {
                    errs() << "⊥|";
                }
                else if (state == Top) {
                    errs() << "⊤|";
                }
                else {
                    errs() << *(r->second.value) << '|';
                }

			}
			errs() << "\n";

		}

		ConstPropContent getConstMap() {
			return const_map;
		}

		static bool equals(ConstPropInfo * info1, ConstPropInfo * info2) {
			// return info1->getConstMap() == info2->getConstMap();
            return true;
		}

		static int join(ConstPropInfo * info1, ConstPropInfo * info2, ConstPropInfo * result) {
			ConstPropContent info2_map;
            info2_map = info2->getConstMap();
            ConstPropContent return_map;
            return_map = info1->getConstMap();
			
            auto it = info2_map.begin();
			for (auto it_end = info2_map.end(); it != it_end; ++it) {
				return_map[it->first] = it->second;
			}

			result->setConstMap(return_map);

			return 0;
		}

        void setConstMap(ConstPropContent val) {
			const_map = val;
		}

        Constant* getConst(Value* key) {
            return const_map[key].value;
        }

        // typedef unordered_map< Value*, struct Const > ConstPropContent;
        // struct Const { ConstState state; Constant* value; };
        int setTop(Value* key) {
            if (const_map.count(key) == 0) {
                struct Const newCon;
                const_map[key] = newCon;
            }
            const_map[key].state = Top;
            return 0;
        }

        int setBottom(Value* key) {
            if (const_map.count(key) == 0) {
                struct Const newCon;
                const_map[key] = newCon;
            }
            const_map[key].state = Bottom;
            return 0;
        }

        int setConst (Value* key, Constant* c) {
            if (const_map.count(key) == 0) {
                struct Const newCon;
                const_map[key] = newCon;
            }
            const_map[key].state = Const;
            const_map[key].value = c;
            return 0;
        }

};





class ConstPropAnalysis : public DataFlowAnalysis<ConstPropInfo, true> {
	protected:
		typedef pair<unsigned, unsigned> Edge;
		
	public:

		ConstPropAnalysis(ConstPropInfo & bottom, ConstPropInfo & initialState) : 
			DataFlowAnalysis(bottom, initialState) {}

		void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges,
									   std::vector<unsigned> & OutgoingEdges,
									   std::vector<ConstPropInfo *> & Infos) {
            

			map<Edge, ConstPropInfo *> edgeToInfo = EdgeToInfo;
            // ConstantFolder *FOLDER;
            string op_name = I->getOpcodeName();

            ConstPropInfo * info = new ConstPropInfo();
            unsigned idx_of_inst = InstrToIndex[I];


            size_t ii = 0;
			for (; ii < IncomingEdges.size(); ++ii) {
				Edge in_edge = make_pair(IncomingEdges[ii], idx_of_inst);
				ConstPropInfo::join(info, edgeToInfo[in_edge], info);
			}


            info->setTop(I);

            // if (BinaryOperator* bin_op = dyn_cast(BinaryOperator)(I) ) {
            //     Value* x = I.getOperand(0);
            //     Value* y = I.getOperand(1);

            //     Constant* cons_x = dyn_cast<Constant>(x);
            //     Constant* cons_y = dyn_cast<Constant>(y);

            //     if (!cons_x) {
            //         cons_x = info->getConst(x);
            //     }
            //     if (!cons_y) {
            //         cons_y = info->getConst(y);
            //     }

            //     if ( cons_x && cons_y ) {
            //         // a will be a constant
            //         int ret = info->setConst(I, FOLDER.CreateBinOp(bin_op->getOpCode(), cons_x, cons_y) );
            //     }
            //     else {
            //         int ret = info->setTop(I);
            //     }

            // }
            
            // else if (UnaryOperator* u_op = dyn_cast(UnaryOperator)(I) ) {
            //     Value* x = I.getOperand(0);
            //     Constant* cons_x = dyn_cast<Constant>(x);

            //     if (!cons_x) {
            //         cons_x = info->getConst(x);
            //     }
            
            //     if (cons_x) {
            //         int ret = info->setConst(I, FOLDER.CreateUnOp(bin_op->getOpCode(), cons_x) );         
            //     }
            //     else {
            //         int ret = info->setTop(I);
            //     }
            // }

            // else () {
            //     int ret = info->setTop(I);
            // }

			size_t j = 0;
			for (; j < OutgoingEdges.size(); ++j) {
				Infos.push_back(info);
			}

		}
};



namespace {
    struct ConstPropAnalysisPass : public CallGraphSCCPass {
        static char ID;
        static set<Value*> MPT;

        ConstPropAnalysisPass() : CallGraphSCCPass(ID) {}

        bool doInitialization(CallGraph &CG) {
            // build MPT

            return false;
        }

        bool runOnSCC(CallGraphSCC &SCC) {
            return false;
        }

        bool doFinalization(CallGraph &CG) {
            set<Value*> globvars;

            for (auto &g : CG.getModule().getGlobalList() ) {
                globvars.insert( &g ) ; 
            }


            for (Function& F : CG.getModule().functions()) {
                ConstPropInfo bottom;
                ConstPropInfo initialState;

                for (Value* val : globvars )  {
                    bottom.setBottom(val);
                    initialState.setTop(val);
                }

                ConstPropAnalysis * cp = new ConstPropAnalysis(bottom, initialState);
                cp->runWorklistAlgorithm(&F);
                cp->print();
            }

            return false;
        }

    }; // end of struct ConstPropAnalysisPass
}  // end of anonymous namespace


char ConstPropAnalysisPass::ID = 0;
static RegisterPass<ConstPropAnalysisPass> X("cse231-constprop", "Implementing ConstProp analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);