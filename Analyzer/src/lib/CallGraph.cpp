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
#include "Config.h"
#include "Common.h"

using namespace llvm;

class FuncDef {
public:
	FuncDef(std::string funcname) : funcName(funcname) {}
	std::string funcName;
};

typedef std::shared_ptr<FuncDef> FuncDefPtr;

class CallGraph {
public:
	CallGraph(){}
	
	std::map<std::string, FuncDefPtr> funcName2FuncDef;
	std::map<FuncDefPtr, std::set<FuncDefPtr>> node2succ;
	void addCallRel(FuncDefPtr caller, FuncDefPtr callee) {
		if(this->node2succ.find(caller) != this->node2succ.end()) {
			
		} else {

		}
	}
	void addCallRel(std::string callerName, std::string calleeName) {
		FuncDefPtr callerDef;
		FuncDefPtr calleeDef;
		if(funcName2FuncDef.find(callerName) == funcName2FuncDef.end()) {
			FuncDefPtr newCaller = std::make_shared<FuncDef>(callerName);
			funcName2FuncDef[callerName] = newCaller;
			callerDef = newCaller;
		} else {
			callerDef = funcName2FuncDef[callerName];
		}
		if(funcName2FuncDef.find(calleeName) == funcName2FuncDef.end()) {
			FuncDefPtr newCallee = std::make_shared<FuncDef>(calleeName);
			funcName2FuncDef[calleeName] = newCallee;
			calleeDef = newCallee;
		} else {
			calleeDef = funcName2FuncDef[calleeName];
		}
		this->addCallRel(callerDef, calleeDef);
	}
	

	
};

bool CallGraphPass::doInitialization(Module *M) {

	return false;
}

bool CallGraphPass::doFinalization(Module *M) {

	return false;
}


bool CallGraphPass::doModulePass(Module *M) {


	return false;
}
