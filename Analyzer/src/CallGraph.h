#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

class FuncDef {
public:
	FuncDef(std::string funcname) : funcName(funcname) {}
	std::string funcName;
};

typedef std::shared_ptr<FuncDef> FuncDefPtr;

class KernelCG {
public:
	KernelCG(){}
	
	std::map<std::string, FuncDefPtr> funcName2FuncDef;
	std::map<FuncDefPtr, std::set<FuncDefPtr>> node2succ;
	void addCallRel(FuncDefPtr caller, FuncDefPtr callee) {
		if(this->node2succ.find(caller) != this->node2succ.end()) {
			this->node2succ[caller].insert(callee);
		} else {
			std::set<FuncDefPtr> newSet;
			newSet.insert(callee);
			this->node2succ[caller] = newSet;
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

static KernelCG globalCallGraph;

using namespace llvm;
class CallGraphPass : public ModulePass {
public:
	static char ID;

	CallGraphPass() : ModulePass(ID){}

	bool runOnModule(Module &M) override;
	
};

char CallGraphPass::ID = 0;
#endif
