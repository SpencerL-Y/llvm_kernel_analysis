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
		std::cout << "----- func name: " << functionName << std::endl;

		FuncDefPtr caller = globalCallGraph.testAndGetFuncName(functionName);

		for(auto &BB : F) {
			for(auto &I : BB) {
				if(auto *call = dyn_cast<CallBase>(&I)) {
					if(Function* calledFunction = call->getCalledFunction()) {
						std::string calledFuncName = calledFunction->getName().str();
						if(isCoreFuncName(calledFuncName)) {
							std::cout << "called func name: " << calledFuncName<< std::endl;
							FuncDefPtr callee = globalCallGraph.testAndGetFuncName(calledFuncName);
							globalCallGraph.addCallRel(caller, callee);
						}
					}
				}
			}
		}
	}
	return PreservedAnalyses::all();
}
