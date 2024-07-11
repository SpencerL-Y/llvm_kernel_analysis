
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/JSON.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/Passes/PassBuilder.h>


#include <memory>
#include <vector>
#include <sstream>
#include <iostream>
#include <sys/resource.h>
#include <filesystem>
#include <boost/algorithm/string/predicate.hpp>

#include "CallGraph.h"


int main(int argc, char **argv) {
	SMDiagnostic Err;
	std::string kernelBCDir = "/home/clexma/Desktop/fox3/fuzzing/linuxRepo/llvm_compile/bc_dir";
    std::vector<std::string> inputFileNames;
	for (const auto& p : std::filesystem::recursive_directory_iterator(kernelBCDir)) {
		if (!std::filesystem::is_directory(p)) {
				std::filesystem::path path = p.path();
				if (boost::algorithm::ends_with(path.string(), ".llbc")) {
					inputFileNames.push_back(path.string());
					std::cout << (path.u8string()) << std::endl;
				}
		}
	}


	for(unsigned i = 0; i < inputFileNames.size(); i ++) {
		LLVMContext *LLVMCtx = new LLVMContext();
		std::unique_ptr<Module> curr_M = parseIRFile(inputFileNames[i], Err, *LLVMCtx);
		// std::cout << "IRFileName: " << inputFileNames[i] << std::endl;
		FunctionAnalysisManager FAM;
		CallGraphPass CGP;
		for(Function &F : *curr_M) {
			CGP.run(F, FAM);
		}
	}
	return 0;
	
}
