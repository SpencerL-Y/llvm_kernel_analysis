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

// #define OPEN_DEBUG


//string utils
bool startsWith(std::string s, std::string prefix){
  return s.find(prefix) == 0?true:false;
}

std::string& trim(std::string &s) 
{
    if (s.empty()) 
    {
        return s;
    }
 
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

bool isCoreFuncName(std::string funcName) {
	for(std::string prefix : excluded_prefixes) {
		if(startsWith(funcName, prefix)) {
			return false;
		}
	}
	return true;
}

PreservedAnalyses CallGraphPass::run(Module& M, ModuleAnalysisManager &AM) {
	for(Function& F : M.getFunctionList()) {
		if(!F.isDeclaration()) {
			bool is_syscall = false;
			std::string functionName = F.getName().str();
			#ifdef OPEN_DEBUG
			std::cout << "----- func name: " << functionName << std::endl;
			#endif
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
	}
		
	return PreservedAnalyses::all();
}

std::vector<CallPathPtr> KernelCG::searchCallPath(std::string targetFunc, int depth) {
		#ifdef OPEN_DEBUG
		std::cout << "curr graph nodesize: " << this->funcName2FuncDef.size() << std::endl;
		std::cout << "curr graph predsize: " << this->node2pred.size() << std::endl;
		std::cout << "curr graph succsize: " << this->node2succ.size() << std::endl;
		std::cout << "search call path:" << targetFunc << "," << depth << std::endl;
		#endif
		targetFunc = trim(targetFunc);
		std::vector<CallPathPtr> result;
		if(this->funcName2FuncDef.find(targetFunc) == this->funcName2FuncDef.end()) {
			std::cout << "ERROR: target function does not exists" << std::endl;
			return result;
		}
		if(depth == 0) {
			return result;
		}
		FuncDefPtr targetFuncDef = this->funcName2FuncDef[targetFunc];
		assert(targetFuncDef != nullptr);
		if(targetFuncDef->is_syscall()) {
			std::vector<CallPathPtr> pathInits;
			CallPathPtr path = std::make_shared<CallPath>();
			path->set_complete();
			// path->append(targetFuncDef);
			pathInits.push_back(path);
			return pathInits;
		}

		if(this->node2pred.find(targetFuncDef) != this->node2pred.end()) {
			std::set<FuncDefPtr> predecessors = this->node2pred[targetFuncDef];
			#ifdef OPEN_DEBUG
			std::cout <<"predecessors: " << std::endl;
			#endif
			for(FuncDefPtr predecessor : predecessors) {
				#ifdef OPEN_DEBUG
				std::cout << predecessor->funcName << std::endl;
				#endif
				std::vector<CallPathPtr> previousCallPaths = this->searchCallPath(predecessor->funcName, depth-1);
				for(CallPathPtr callpath : previousCallPaths) {
					callpath->append(predecessor);
					result.push_back(callpath);
				}
			}
			return result;
		} else {
			// this branch means that curent  target function has no predecessors
			CallPathPtr path = std::make_shared<CallPath>();
			
			std::vector<CallPathPtr> pathInits;
			pathInits.push_back(path);
			#ifdef OPEN_DEBUG
			std::cout << "target function has no predecessors" << std::endl;
			#endif
			return pathInits;
		}

		

	}

void KernelCG::export2file() {
	{
		std::string fileName = "callgraphFile.txt";
		std::ofstream fileOut(fileName, std::ios::out);
		fileOut << "-----funcnames" << std::endl;
		for(auto item : this->funcName2FuncDef) {
			#ifdef OPEN_DEBUG
			std::cout << "export funcname: " << item.first << std::endl;
			#endif
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
		int mode = -1;// 0 for funcnames, 1 for fowardrel and 2 for backwardrel
		while(std::getline(infile, line)) {
			if(startsWith(line, "-----")) {
				#ifdef OPEN_DEBUG
				std::cout << "line: " << line << std::endl;
				#endif
				if(line.find("funcnames") != std::string::npos) {
					mode = 0;

					#ifdef OPEN_DEBUG
					std::cout << "parsing funcnames =====" << std::endl;
					#endif
				} else if(line.find("forwardrel") != std::string::npos) {
					mode = 1;

					#ifdef OPEN_DEBUG
					std::cout << "parsing successors =====" << std::endl;
					#endif
				} else if(line.find("backwardrel") != std::string::npos) {
					mode = 2;
					
					#ifdef OPEN_DEBUG
					std::cout << "parsing predecessors =====" << std::endl;
					#endif
				} else {

					#ifdef OPEN_DEBUG
					std::cout << "ERROR: unrecognized format" << std::endl;
					#endif
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
						token = trim(token);
						#ifdef OPEN_DEBUG
						std::cout << "token:" << token << "\t";
						#endif
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
							assert(newFunc != nullptr);
							this->funcName2FuncDef[token] = newFunc;
						}
						curr_pos += 1;
					}
					#ifdef OPEN_DEBUG
					std::cout << std::endl;
					#endif
				} else if(mode == 1) {
					#ifdef OPEN_DEBUG
					std::cout << "parse succ" << std::endl;
					#endif
					// parse forward relations
					std::istringstream iss(line);
					std::string token;
					int curr_pos = 0;
					FuncDefPtr from = nullptr;
					FuncDefPtr to = nullptr;
					while(getline(iss, token, ',')) {
						#ifdef OPEN_DEBUG
						std::cout << token << ",";
						#endif
						token = trim(token);
						if (curr_pos == 0) {
							from = this->funcName2FuncDef[token];
						} else {
							to = this->funcName2FuncDef[token];
						}
						curr_pos += 1;
					}
					#ifdef OPEN_DEBUG
					std::cout << "from: " << from->funcName << " to: " << to->funcName << std::endl; 
					#endif
					
					assert(from != nullptr);
					assert(to != nullptr);
					if(this->node2succ.find(from) != this->node2succ.end()) {
						this->node2succ[from].insert(to);
					} else {
						std::set<FuncDefPtr> newSet;
						newSet.insert(to);
						this->node2succ[from] = newSet;
					}
					#ifdef OPEN_DEBUG
					std::cout << std::endl;
					#endif
				} else if(mode == 2) {
					#ifdef OPEN_DEBUG
					std::cout << "parse pred" << std::endl;
					#endif
					// parse backward relations
					std::istringstream iss(line);
					std::string token;
					int curr_pos = 0;
					FuncDefPtr from = nullptr;
					FuncDefPtr to = nullptr;
					while(getline(iss, token, ',')) {
						#ifdef OPEN_DEBUG
						std::cout << token << ",";
						#endif
						token = trim(token);
						if(curr_pos == 0) {
							to = this->funcName2FuncDef[token];
						} else {
							from = this->funcName2FuncDef[token];
						}
						curr_pos += 1;
					}
					#ifdef OPEN_DEBUG
					std::cout << std::endl;
					#endif
					assert(from != nullptr);
					assert(to != nullptr);
					if(this->node2pred.find(to) != this->node2pred.end()) {
						this->node2pred[to].insert(from);
					} else {
						std::set<FuncDefPtr> newSet;
						newSet.insert(from);
						this->node2pred[to] = newSet;
					}
				} else {
					#ifdef OPEN_DEBUG
					std::cout << "ERROR: error reading mode" << std::endl;
					#endif
					return;
				}
			}
		}
		return;
	}

std::set<std::string> KernelCG::findFunctionsWithinNSteps(std::string funcName, int steps){

	std::set<std::string> reachFunctions;
	std::queue<std::pair<FuncDefPtr, int>> toVisit;
	FuncDefPtr startFunc = this->funcName2FuncDef[funcName];
	reachFunctions.insert(funcName);

	if(!startFunc){
		std::cout << "WARINING: Function " << funcName << " not found in the call graph, using the function itself" << std::endl;
		return reachFunctions;
	}

	toVisit.push({startFunc, 0});

	while (!toVisit.empty()){
		auto [currentFunc, currentStep] = toVisit.front();
		toVisit.pop();
		if(currentStep > steps) continue;
		if(currentStep > 0 && !currentFunc->is_syscall()) reachFunctions.insert(currentFunc->funcName);
		if(this->node2pred.find(currentFunc) != this->node2pred.end()){
			for(FuncDefPtr preFunc : this->node2pred[currentFunc]){
				if(reachFunctions.find(preFunc->funcName) == reachFunctions.end()){
					toVisit.push({preFunc, currentStep + 1});
				}
			}
		}		
	}
	return reachFunctions;
}