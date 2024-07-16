//===-- CallGraph.cc - Build global call-graph------------------===//
// 
// This pass builds a global call-graph. The targets of an indirect
// call are identified based on two-layer type-analysis.
//
// First layer: matching function type
// Second layer: matching struct type
//
// In addition, loops are unrolled as "if" statements
//
//===-----------------------------------------------------------===//

#include <llvm/IR/DebugInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include "llvm/IR/Instruction.h"
#include <llvm/Support/Debug.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Analysis/CallGraph.h>
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"  
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/BasicBlock.h" 
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include <llvm/IR/LegacyPassManager.h>
#include <map> 
#include <vector> 
#include "llvm/IR/CFG.h" 
#include "llvm/Transforms/Utils/BasicBlockUtils.h" 
#include "llvm/IR/IRBuilder.h"

#include "CallGraph.h"
#include <iostream>

using namespace llvm;


bool startsWith(std::string s, std::string prefix){
  return s.find(prefix) == 0?true:false;
}

bool isCoreFuncName(std::string funcName) {
	for(std::string prefix : excluded_prefixes) {
		if(startsWith(funcName, prefix)) {
			return false;
		}
	}
	return true;
}

PreservedAnalyses CallGraphPass::run(Function &F, FunctionAnalysisManager &AM) {
	if(!F.isDeclaration()) {
		bool is_syscall = false;
		std::string functionName = F.getName().str();
		// std::cout << "----- func name: " << functionName << std::endl;

		FuncDefPtr caller = globalCallGraph->testAndGetFuncName(functionName);

		for(auto &BB : F) {
			for(auto &I : BB) {
				if(auto *call = dyn_cast<CallBase>(&I)) {
					if(Function* calledFunction = call->getCalledFunction()) {
						std::string calledFuncName = calledFunction->getName().str();
						if(isCoreFuncName(calledFuncName)) {
							// std::cout << "called func name: " << calledFuncName<< std::endl;
							FuncDefPtr callee = globalCallGraph->testAndGetFuncName(calledFuncName);
							globalCallGraph->addCallRel(caller, callee);
						}
					}
				}
			}
		}
	}
	return PreservedAnalyses::all();
}

std::vector<CallPathPtr> KernelCG::searchCallPath(std::string targetFunc, int depth) {
		std::vector<CallPathPtr> result;
		if(this->funcName2FuncDef.find(targetFunc) == this->funcName2FuncDef.end()) {
			std::cout << "ERROR: target function does not exists" << std::endl;
			return result;
		}
		FuncDefPtr targetFuncDef = this->funcName2FuncDef[targetFunc];
		if(targetFuncDef->is_syscall()) {
			std::vector<CallPathPtr> pathInits;
			CallPathPtr path = std::make_shared<CallPath>();
			path->append(targetFuncDef);
			pathInits.push_back(targetFuncDef);
			return pathInits;
		}

		if(this->node2pred.find(targetFuncDef) != this->node2pred.end()) {
			
		} else {
			return result;
		}

		

	}

void KernelCG::export2file() {
	{
		std::string fileName = "callgraphFile.txt";
		std::ofstream fileOut(fileName, std::ios::out);
		fileOut << "-----funcnames" << std::endl;
		for(auto item : this->funcName2FuncDef) {
			std::cout << "export funcname: " << item.first << std::endl;
			if(item.second->is_syscall()) {
				fileOut << "1," << item.second->funcName << std::endl;
			} else {
				fileOut << "0," << item.second->funcName << std::endl;
			}
		}
		fileOut << "-----forwardrel" << std::endl;
		for(auto item : this->node2succ) {
			for(auto specific_to : item.second) {
				fileOut << item.first->funcName << "," << specific_to->funcName << std::endl;
			}
		}
		fileOut << "-----backwardrel" << std::endl;
		for(auto item : this->node2pred) {
			for(auto specific_from : item.second) {
				fileOut << item.first->funcName << "," << specific_from->funcName << std::endl;
			}
		}
	}	
}

void KernelCG::restoreKernelCGFromFile() {
		std::ifstream infile("callgraphFile.txt");
		std::string line;
		bool mode = -1;// 0 for funcnames, 1 for fowardrel and 2 for backwardrel
		while(std::getline(infile, line)) {
			if(startsWith(line, "-----")) {
				if(line.find("funcnames") != std::string::npos) {
					mode = 0;
				} else if(line.find("forwardrel") != std::string::npos) {
					mode = 1;
				} else if(line.find("backwardrel") != std::string::npos) {
					mode = 2;
				} else {
					std::cout << "ERROR: unrecognized format" << std::endl;
					return;
				}
			} else {
				assert(mode >= 0);
				if(mode == 0) {
					// parse funcdefs
					std::istringstream iss(line);
					std::string token;
					int curr_pos = 0;
					bool is_syscall = false;
					while(getline(iss, token, ',')) {
						std::cout << "token: " << token << "\t";
						if(curr_pos == 0) {
							if(token.compare("1") == 0) {
								is_syscall = true;
							} else {
								is_syscall = false;
							}
						} else {
							FuncDefPtr newFunc = nullptr;
							if(is_syscall) {
								std::string prefix = "";
								if(token.find("__x64_sys_") != std::string::npos) {
									prefix = "__x64_sys_";
								} else if(token.find("__se_sys_") != std::string::npos) {
									prefix = "__se_sys_";
								} else {
									prefix = "__do_sys_";
								}
								newFunc = std::make_shared<SyscallDef>(token, prefix);
							} else {
								newFunc = std::make_shared<FuncDef>(token);
							}
							globalCallGraph->funcName2FuncDef[token] = newFunc;
						}
						curr_pos += 1;
					}
					std::cout << std::endl;
				} else if(mode == 1) {
					// parse forward relations
					std::istringstream iss(line);
					std::string token;
					int curr_pos = 0;
					FuncDefPtr from = nullptr;
					FuncDefPtr to = nullptr;
					while(getline(iss, token, ',')) {
						if (curr_pos == 0) {
							from = globalCallGraph->funcName2FuncDef[token];
						} else {
							to = globalCallGraph->funcName2FuncDef[token];
						}
						curr_pos += 1;
					}
					assert(from != nullptr && to != nullptr);
					if(globalCallGraph->node2succ.find(from) != globalCallGraph->node2succ.end()) {
						globalCallGraph->node2pred[from].insert(to);
					} else {
						std::set<FuncDefPtr> newSet;
						newSet.insert(to);
						globalCallGraph->node2pred[from] = newSet;
					}
				} else if(mode == 2) {
					// parse backward relations
					std::istringstream iss(line);
					std::string token;
					int curr_pos = 0;
					FuncDefPtr from = nullptr;
					FuncDefPtr to = nullptr;
					while(getline(iss, token, ',')) {
						if(curr_pos == 0) {
							to = globalCallGraph->funcName2FuncDef[token];
						} else {
							from = globalCallGraph->funcName2FuncDef[token];
						}
						curr_pos += 1;
					}
					assert(from != nullptr && to != nullptr);
					if(globalCallGraph->node2pred.find(to) != globalCallGraph->node2pred.end()) {
						globalCallGraph->node2pred[to].insert(from);
					} else {
						std::set<FuncDefPtr> newSet;
						newSet.insert(from);
						globalCallGraph->node2pred[to] = newSet;
					}
				} else {
					std::cout << "ERROR: error reading mode" << std::endl;
					return;
				}
			}
		}
	}
