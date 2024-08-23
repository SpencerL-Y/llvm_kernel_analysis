
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
#include <fstream>
#include <sys/resource.h>
#include <filesystem>
#include <boost/algorithm/string/predicate.hpp>

#include "CallGraph.h"
std::shared_ptr<KernelCG> globalCallGraph = std::make_shared<KernelCG>();
std::string project_root = "/home/clexma/Desktop/fox3/fuzzing/";
bool containFile(std::string filename){
	std::ifstream f(filename.c_str());
	return f.good();
}

int main(int argc, char **argv) {
	std::string targetFuncName = argv[1];
	std::string max_depth_str = "";
	if(argc > 2) {
		max_depth_str = argv[2];
	}
	std::cout << "target function: " << targetFuncName << std::endl; 
	std::cout << "start analyzing" << std::endl;
	int depth = argc > 2 ? std::stoi(max_depth_str) : 20;
	std::ofstream pathsFile;
	pathsFile.open("pathsFile.txt", std::ios::out);
	if(containFile("callgraphFile.txt")) {	
		globalCallGraph->restoreKernelCGFromFile();
		std::vector<CallPathPtr> callpaths =  globalCallGraph->searchCallPath(targetFuncName, depth);
		for(CallPathPtr p : callpaths) {
			pathsFile << "#path" << std::endl;
			for(std::string fn : p->toStringVec()) {
				pathsFile << fn << std::endl;
			}
		}
	} else {
		bool debug = false;
		if(debug) {
			std::string testbcFile = project_root + "linuxRepo/llvm_kernel_analysis/bc_dir/fs/fsopen.llbc";
			LLVMContext *LLVMCtx = new LLVMContext();
			SMDiagnostic Err;
			std::unique_ptr<Module> curr_M = parseIRFile(testbcFile, Err, *LLVMCtx);
			ModuleAnalysisManager MAM;
			CallGraphPass CGP;
			CGP.run(*curr_M, MAM);
			for(auto item : globalCallGraph->funcName2FuncDef) {
				std::cout << item.first << std::endl;
			}
			globalCallGraph->export2file();
		} else {
			SMDiagnostic Err;
			std::string kernelBCDir = project_root + "linuxRepo/llvm_kernel_analysis/bc_dir";
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
				std::cout << "IRFileName: " << inputFileNames[i] << std::endl;
				ModuleAnalysisManager MAM;
				CallGraphPass CGP;
				CGP.run(*curr_M, MAM);
			}
			for(auto item : globalCallGraph->funcName2FuncDef) {
				std::cout << item.first << std::endl;
			}
			globalCallGraph->export2file();
			globalCallGraph->searchCallPath(targetFuncName, depth);
			std::vector<CallPathPtr> callpaths = globalCallGraph->searchCallPath(targetFuncName, depth);
			for(CallPathPtr p : callpaths) {
				pathsFile << "#path" << std::endl;
				for(std::string fn : p->toStringVec()) {
					pathsFile << fn << std::endl;
				}
			}
		}
		
	}
	int steps = 2;
	std::set<std::string> reachFunctions = globalCallGraph->findFunctionsWithinNSteps(targetFuncName, steps);
	std::cout << "Functions reachable within " << steps << " steps to " << targetFuncName << ":" << std::endl;
	for(std::string funcName : reachFunctions){
		std::cout << funcName << std::endl;
	}
	std::cout << "---- over ----" << std::endl;
	
	return 0;
	
}
