
cmake_minimum_required(VERSION 3.10)
project(BugDetector)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++14 support
# for your compiler.
# Check for C++14 support and set the compilation flag
include(CheckCXXCompilerFlag)


include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set (AnalyzerSourceCodes
	Analyzer.cpp
	CallGraph.h
	CallGraph.cpp
)

add_library (AnalyzerObj OBJECT ${AnalyzerSourceCodes} )
add_library (AnalyzerStatic STATIC $<TARGET_OBJECTS:AnalyzerObj>)


# Create the main executable
add_executable(main
    Analyzer.cpp
)

# Link the executable against the LLVM libraries
llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
)

target_link_libraries(main PRIVATE ${llvm_libs} AnalyzerStatic)