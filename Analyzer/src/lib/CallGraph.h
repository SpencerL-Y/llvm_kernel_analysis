#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "Analyzer.h"

class CallGraphPass : public IterativeModulePass {

	private:
		const DataLayout *DL;
	public:
		CallGraphPass(GlobalContext *Ctx_)
			: IterativeModulePass(Ctx_, "CallGraph") { }

		virtual bool doInitialization(llvm::Module *);
		virtual bool doFinalization(llvm::Module *);
		virtual bool doModulePass(llvm::Module *);

};

#endif
