#include "231DFA.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ConstantFolder.h"


using namespace llvm;
using namespace std;

class MayPointToInfo : public Info {
	public:
		MayPointToInfo() {}

		MayPointToInfo(map<pair<char, unsigned>, set<unsigned>> m) {
			info_map = m;
		}

        map<pair<char, unsigned>, set<unsigned>> info_map;

        void print() {
			errs() << "The size of the info_map: " << info_map.size() << "\n";
			for (auto r = info_map.begin(), r_end = info_map.end(); r != r_end; ++r) {
				if (r->second.size() != 0) {
                    errs() << (char) toupper(r->first.first) << r->first.second << "->(";

                    auto m = r->second.begin();
                    for (auto m_end = r->second.end(); m != m_end; ++m) {
                        errs() << "M" << *m << "/";
                    }
                    errs() << ")|";
                }
			}
			errs() << "\n";
		}

        set<unsigned> getMemSet(pair<char, unsigned> k) {
			return info_map[k];
		}

		map<pair<char, unsigned>, set<unsigned>> getInfoMap() {
			return info_map;
		}

		void setInfoMap(map<pair<char, unsigned>, set<unsigned>> val) {
			info_map = val;
		}

		static bool equals(MayPointToInfo * info1, MayPointToInfo * info2) {
			return info1->getInfoMap() == info2->getInfoMap();
		}

		static int join(MayPointToInfo * info1, MayPointToInfo * info2, MayPointToInfo * result) {
			map<pair<char, unsigned>, set<unsigned>> info2_map;
            info2_map = info2->getInfoMap();
            map<pair<char, unsigned>, set<unsigned>> return_map;
            return_map = info1->getInfoMap();
			
            auto it = info2_map.begin();
			for (auto it_end = info2_map.end(); it != it_end; ++it) {
				return_map[make_pair(it->first.first, it->first.second)].insert(it->second.begin(), it->second.end());
			}

			result->setInfoMap(return_map);

			return 0;
		}
};





class MayPointToAnalysis : public DataFlowAnalysis<MayPointToInfo, true> {
	protected:
		typedef pair<unsigned, unsigned> Edge;
		
	public:
        const char r_char = 'R';
		const char m_char = 'm';

		MayPointToAnalysis(MayPointToInfo & bottom, MayPointToInfo & initialState) : 
			DataFlowAnalysis(bottom, initialState) {}

		void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges,
									   std::vector<unsigned> & OutgoingEdges,
									   std::vector<MayPointToInfo *> & Infos) {
            
            // std::map<unsigned, Instruction *> IndexToInstr;
		    // std::map<Instruction *, unsigned> InstrToIndex;
		    // std::map<Edge, Info *> EdgeToInfo;
			map<Edge, MayPointToInfo *> edgeToInfo = EdgeToInfo;
            string op_name = I->getOpcodeName();


			map<pair<char, unsigned>, set<unsigned>> map_rm;
            MayPointToInfo * info_in = new MayPointToInfo();
			MayPointToInfo * info_out = new MayPointToInfo();
            unsigned idx_of_inst = InstrToIndex[I];


            size_t ii = 0;
			for (; ii < IncomingEdges.size(); ++ii) {
				Edge in_edge = make_pair(IncomingEdges[ii], idx_of_inst);
				MayPointToInfo::join(info_in, edgeToInfo[in_edge], info_in);
				MayPointToInfo::join(info_out, edgeToInfo[in_edge], info_out);
			}


			if (isa<AllocaInst>(I)) {
				map_rm[make_pair(r_char, idx_of_inst)].insert(idx_of_inst);
				int t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
                t += (int) r_char;
			}

			else if (isa<BitCastInst>(I)) {
                int t = (int) r_char;
				unsigned cur = InstrToIndex[(Instruction *) I->getOperand(0)];
				set<unsigned> cur_y;
                cur_y = info_in->getMemSet(make_pair(r_char, cur));
				
				map_rm[make_pair(r_char, idx_of_inst)].insert(cur_y.begin(), cur_y.end());
				t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
                t += (int) m_char;
			}

            else if (isa<GetElementPtrInst>(I)) {
				GetElementPtrInst * ins = cast<GetElementPtrInst> (I);
				unsigned p;
                int t = (int) m_char;
                p = InstrToIndex[(Instruction *) ins->getPointerOperand()];
				set<unsigned> cur = info_in->getMemSet(make_pair(r_char, p));

				map_rm[make_pair(r_char, idx_of_inst)].insert(cur.begin(), cur.end());
				t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
                t += (int) r_char;
			}

			else if (isa<LoadInst>(I) && I->getType()->isPointerTy()) {
                LoadInst * ins = cast<LoadInst> (I);
                set<unsigned> cur_x;
                unsigned p;
                p = InstrToIndex[(Instruction *) ins->getPointerOperand()];
                set<unsigned> cur = info_in->getMemSet(make_pair(r_char, p));

                auto ii = cur.begin(), ii_end = cur.end();
                for (; ii != ii_end; ++ii) {
                    set<unsigned> se;
                    se = info_in->getMemSet(make_pair(m_char, *ii));
                    cur_x.insert(se.begin(), se.end());
                }

                map_rm[make_pair(r_char, idx_of_inst)].insert(cur_x.begin(), cur_x.end());
                int t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
                t += (int) m_char;
			}

            else if (isa<StoreInst>(I)) {
                StoreInst * ins = cast<StoreInst> (I);
                int t = (int) r_char;
                unsigned p;
                p = InstrToIndex[(Instruction *) ins->getPointerOperand()];
                unsigned v = InstrToIndex[(Instruction *) ins->getValueOperand()];
                set<unsigned> cur_x = info_in->getMemSet(make_pair(r_char, p));
                set<unsigned> cur_y = info_in->getMemSet(make_pair(r_char, v));

                auto jj = cur_y.begin(), jj_end = cur_y.end();
                for (; jj != jj_end; ++jj) {
                    auto kk = cur_x.begin(), kk_end = cur_x.end();
                    for (; kk != kk_end; ++kk) {
                        map_rm[make_pair(m_char, *kk)].insert(*jj);
                    }
                }

                t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
                t += (int) m_char;
			}

			else if (isa<SelectInst>(I)) {
				SelectInst * ins = cast<SelectInst> (I);
				unsigned v1, t;
				v1 = InstrToIndex[(Instruction *) ins->getFalseValue()];
				unsigned v2 = InstrToIndex[(Instruction *) ins->getTrueValue()];
				set<unsigned> cur_y = info_in->getMemSet(make_pair(r_char, v2));
				set<unsigned> cur_x = info_in->getMemSet(make_pair(r_char, v1));

				map_rm[make_pair(r_char, idx_of_inst)].insert(cur_y.begin(), cur_y.end());
				map_rm[make_pair(r_char, idx_of_inst)].insert(cur_x.begin(), cur_x.end());
				t = MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
				t += (int) r_char;
			}

			else if (isa<PHINode>(I)) {
				Instruction * first = I->getParent()->getFirstNonPHI();
				unsigned first_non_phi_idx;
				first_non_phi_idx = InstrToIndex[first];

				unsigned ii = idx_of_inst;
				unsigned tt = (int) r_char;
				for (; ii < first_non_phi_idx; ++ii) {
					Instruction *ins = IndexToInstr[ii];

					for (unsigned k = 0; k < ins->getNumOperands(); ++k) {
						unsigned f = InstrToIndex[(Instruction *) ins->getOperand(k)];
						set<unsigned> cur_x = info_in->getMemSet(make_pair(r_char, f));
						tt += (int) r_char;
						map_rm[make_pair(r_char, ii)].insert(cur_x.begin(), cur_x.end());
					}
				}

				tt += MayPointToInfo::join(info_out, new MayPointToInfo(map_rm), info_out);
				tt += (int) m_char;
			}

			size_t j = 0;
			for (; j < OutgoingEdges.size(); ++j) {
				Infos.push_back(info_out);
			}

		}
};




namespace {
    struct MayPointToAnalysisPass : public FunctionPass {
        static char ID;
        MayPointToAnalysisPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
        	MayPointToInfo bottom;
        	MayPointToInfo initialState;
        	MayPointToAnalysis * rda = new MayPointToAnalysis(bottom, initialState);

        	rda->runWorklistAlgorithm(&F);
        	rda->print();

            return false;
        }
    }; // end of struct MayPointToAnalysisPass
}  // end of anonymous namespace


char MayPointToAnalysisPass::ID = 0;
static RegisterPass<MayPointToAnalysisPass> X("cse231-maypointto", "Implementing maypointto analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);