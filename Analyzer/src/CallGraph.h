#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include <stdlib.h>
#include <string>
#include <iostream>
#include <set>


static bool startsWith(std::string s, std::string prefix);
static bool isCoreFuncName(std::string funcName);
static std::set<std::string> excluded_prefixes = {
    "__asan_",
    "__kasan_",
    "__ubsan_",
    "__sanitizer_cov_",
    "__msan_",
    "__tsan_",
    "__stack_chk_",
    "__traceiter_",
    "__trace_",
    "__lock_",
    "__fentry__",
	"llvm.",
};
class FuncDef {
public:
	FuncDef(std::string funcname) : funcName(funcname) {}
	std::string funcName;
	virtual bool is_syscall() {
		return false;
	}
};

typedef std::shared_ptr<FuncDef> FuncDefPtr;

class SyscallDef : public FuncDef {
public:
	SyscallDef(std::string funcName, std::string prefix) : FuncDef(funcName) {
		if(prefix == "__x64_sys_") {
			this->syscallName = funcName.substr(10);
		} else if(prefix == "__se_sys_") {
			this->syscallName = funcName.substr(9);
		} else if(prefix == "__do_sys_") {
			this->syscallName = funcName.substr(9);
		} else {
			std::cout << "ERROR: prefix is not syscall" << std::endl;
		}
	}
	std::string syscallName;
	bool is_syscall() override {
		return true; 
	}
};

class KernelCG {
public:
	KernelCG(){}
	
	std::map<std::string, FuncDefPtr> funcName2FuncDef;
	std::map<FuncDefPtr, std::set<FuncDefPtr>> node2succ;
	std::map<FuncDefPtr, std::set<FuncDefPtr>> node2pred;
	void addCallRel(FuncDefPtr caller, FuncDefPtr callee) {
		if(this->hasCallSucc(caller)) {
			this->node2succ[caller].insert(callee);
		} else {
			std::set<FuncDefPtr> newSet;
			newSet.insert(callee);
			this->node2succ[caller] = newSet;
		}
		if(this->hasCallPred(callee)) {
			this->node2pred[callee].insert(caller);
		} else {
			std::set<FuncDefPtr> newSet;
			newSet.insert(caller);
			this->node2pred[callee] = newSet;
		}
	}
	void addCallRel(std::string callerName, std::string calleeName) {
		FuncDefPtr callerDef = this->testAndGetFuncName(callerName);
		FuncDefPtr calleeDef = this->testAndGetFuncName(calleeName);
		this->addCallRel(callerDef, calleeDef);
	}

	bool hasFuncName(std::string funcname) {
		if(this->funcName2FuncDef.find(funcname) != this->funcName2FuncDef.end()) {
			return true;
		}
		return false;
	}

	bool hasCallSucc(FuncDefPtr funcDef) {
		if(this->node2succ.find(funcDef) != this->node2succ.end()) {
			return true;
		}
		return false;
	}

	bool hasCallPred(FuncDefPtr funcDef) {
		if(this->node2pred.find(funcDef) != this->node2pred.end()) {
			return true;
		}
		return false;
	}

	FuncDefPtr testAndGetFuncName(std::string functionName) {
		bool is_syscall = false;
		std::string prefix = "";
		if(startsWith(functionName, "__do_sys_")){
			prefix = "__do_sys_";
			is_syscall = true;
		} else if(startsWith(functionName, "__se_sys_"))  {
			prefix = "__se_sys_";
			is_syscall = true;
		} else if(startsWith(functionName, "__x64_sys_")) {
			prefix = "__x64_sys_";
			is_syscall = true;
		}
		FuncDefPtr newFunc = nullptr;
		if(this->hasFuncName(functionName)) {
			newFunc = this->funcName2FuncDef[functionName];
		} else {
			if(!is_syscall) {
				newFunc = std::make_shared<FuncDef>(functionName);
			} else {
				newFunc = std::make_shared<SyscallDef>(functionName, prefix);
			}
			this->funcName2FuncDef[functionName] = newFunc;
		}
		return newFunc;
	}
	

	
};

static KernelCG globalCallGraph;

using namespace llvm;
class CallGraphPass : public PassInfoMixin<CallGraphPass> {
public:
PreservedAnalyses run(Function& F, FunctionAnalysisManager &AM);
};

#endif
