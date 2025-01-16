import subprocess
import logging
import os
import shutil
import sys


cur_path = os.getcwd()
ClangPath="/usr/local/bin/clang"
EmitScriptPath = cur_path + "/emit-llvm.sh"

# config path for fuzzing
FuzzingBCDir = cur_path + "/bc_dir"
FuzzingConfigPath=os.path.dirname(cur_path) + "/linux_new"

# config path for static analyzing
AnalyzeBCDir = cur_path + "/analyze_bc_dir"
AnalyzeConfigPath = os.path.dirname(cur_path) + "/linux_analyze"


    
def CompileKernelToBitcodeForFuzzing():
    emit_contents=f'''#!/bin/sh
CLANG={ClangPath}
if [ ! -e $CLANG ]
then
    exit
fi
OFILE=`echo $* | sed -e 's/^.* \(.*\.o\) .*$/\\1/'`
if [ "x$OFILE" != x -a "$OFILE" != "$*" ] ; then
    $CLANG -emit-llvm -O0 -femit-all-decls -fno-inline-functions -g "$@" >/dev/null 2>&1 > /dev/null
    if [ -f "$OFILE" ] ; then
        BCFILE=`echo $OFILE | sed -e 's/o$/llbc/'`
        #file $OFILE | grep -q "LLVM IR bitcode" && mv $OFILE $BCFILE || true
        if [ `file $OFILE | grep -c "LLVM IR bitcode"` -eq 1 ]; then
            mv $OFILE $BCFILE
        else
            touch $BCFILE
        fi
    fi
fi
exec $CLANG "$@"
    
    '''
    if os.path.exists(EmitScriptPath):
        os.remove(EmitScriptPath)
    with open(EmitScriptPath,"w") as f:
        f.write(emit_contents)
    os.chmod(EmitScriptPath,0o775)
    
    caseBCDir = FuzzingBCDir
    configPath = FuzzingConfigPath
    
    if IsCompilationSuccessfulByCase(caseBCDir):
        print(f"Already compiled. Skip")
        return
    elif os.path.exists(caseBCDir):
        shutil.rmtree(caseBCDir, ignore_errors=True)
    
    print(f"starting compiling, target output to {caseBCDir}")
    os.mkdir(caseBCDir)
    
    default_config_cmd = f"cd {configPath} && make clean && make CC={EmitScriptPath} O={caseBCDir}  defconfig && make CC={EmitScriptPath} O={caseBCDir}  kvm_guest.config"
    os.system(default_config_cmd)
    EnabledConfigs=[
        "CONFIG_KCOV",
        "CONFIG_DEBUG_INFO_DWARF4",
        "CONFIG_KASAN",
        "CONFIG_KASAN_INLINE",
        "CONFIG_CONFIGFS_FS",
        "CONFIG_SECURITYFS",
        "CONFIG_CMDLINE_BOOL"
    ]
    with open(os.path.join(caseBCDir, '.config'), "a") as f:
        f.writelines([f"{c}=y\n" for c in EnabledConfigs])
        f.writelines("\nCONFIG_CMDLINE=\"net.ifnames=0\"\n")
    
    compile_command = f"cd {configPath}  &&  make CC={EmitScriptPath} O={caseBCDir} olddefconfig && make CC=\'{EmitScriptPath}\' O={caseBCDir} -j16"
    # compile_command = f"cd {Config.getSrcDirByCase(caseIdx)} && git checkout -- scripts/Makefile.kcov && make clean && make mrproper && yes | make CC={Config.EmitScriptPath} O={caseBCDir} oldconfig && make CC=\'{Config.EmitScriptPath}\' O={caseBCDir} -j16"
    # print(Config.ExecuteCMD(compile_command))
    print(compile_command)
    os.system(compile_command)
    if IsCompilationSuccessfulByCase(caseBCDir):
        print(f"Successfully compiled bitcode")
    else:
        print(f"Error compiling bitcode!!!")
            
        

    
def CompileKernelToBitcodeForAnalysis():
    emit_contents=f'''#!/bin/sh
CLANG={ClangPath}
if [ ! -e $CLANG ]
then
    exit
fi
OFILE=`echo $* | sed -e 's/^.* \(.*\.o\) .*$/\\1/'`
if [ "x$OFILE" != x -a "$OFILE" != "$*" ] ; then
    $CLANG -emit-llvm -O0 -femit-all-decls -fno-inline-functions -g "$@" >/dev/null 2>&1 > /dev/null
    if [ -f "$OFILE" ] ; then
        BCFILE=`echo $OFILE | sed -e 's/o$/llbc/'`
        #file $OFILE | grep -q "LLVM IR bitcode" && mv $OFILE $BCFILE || true
        if [ `file $OFILE | grep -c "LLVM IR bitcode"` -eq 1 ]; then
            mv $OFILE $BCFILE
        else
            touch $BCFILE
        fi
    fi
fi
exec $CLANG "$@"
    
    '''
    if os.path.exists(EmitScriptPath):
        os.remove(EmitScriptPath)
    with open(EmitScriptPath,"w") as f:
        f.write(emit_contents)
    os.chmod(EmitScriptPath,0o775)
    
    caseBCDir = AnalyzeBCDir
    configPath = AnalyzeConfigPath
    
    if IsCompilationSuccessfulByCase(caseBCDir):
        print(f"Already compiled. Skip")
        return
    elif os.path.exists(caseBCDir):
        shutil.rmtree(caseBCDir, ignore_errors=True)
    
    print(f"starting compiling, target output to {caseBCDir}")
    os.mkdir(caseBCDir)

    default_config_cmd = f"cd {configPath} && make clean && make  CC={EmitScriptPath} O={caseBCDir}  defconfig && make  CC={EmitScriptPath} O={caseBCDir}  kvm_guest.config"
    os.system(default_config_cmd)
    EnabledConfigs=[
        "CONFIG_KCOV",
        "CONFIG_DEBUG_INFO",
        "CONFIG_DEBUG_INFO_DWARF4",
        "CONFIG_KASAN",
        "CONFIG_CONFIGFS_FS",
        "CONFIG_SECURITYFS",
        "CONFIG_CMDLINE_BOOL",
        "CONFIG_COMPILE_TEST"
    ]
    DisabledConfigs = [
        "CONFIG_DEBUG_INFO_REDUCED",
        "CONFIG_DEBUG_INFO_SPLIT",
        "CONFIG_HAVE_STATIC_CALL",
        "CONFIG_HAVE_STATIC_CALL_INLINE",
        "CONFIG_KASAN_INLINE"
    ]
    with open(os.path.join(caseBCDir, '.config'), "a") as f:
        f.writelines([f"{c}=y\n" for c in EnabledConfigs])
        f.writelines([f"{c}=n\n" for c in DisabledConfigs])
        f.writelines("\nCONFIG_CMDLINE=\"net.ifnames=0\"\n")
    
    compile_command = f"cd {configPath}  &&  make  CC={EmitScriptPath} O={caseBCDir} olddefconfig && make  CC=\'{EmitScriptPath}\' O={caseBCDir} -j16"
    # compile_command = f"cd {Config.getSrcDirByCase(caseIdx)} && git checkout -- scripts/Makefile.kcov && make clean && make mrproper && yes | make CC={Config.EmitScriptPath} O={caseBCDir} oldconfig && make CC=\'{Config.EmitScriptPath}\' O={caseBCDir} -j16"
    # print(Config.ExecuteCMD(compile_command))
    print(compile_command)
    os.system(compile_command)
    if IsCompilationSuccessfulByCase(caseBCDir):
        print(f"Successfully compiled bitcode")
    else:
        print(f"Error compiling bitcode!!!")
            
    

def IsCompilationSuccessfulByCase(caseBCDir):
    bc_bzImage_path = os.path.join(caseBCDir, "arch/x86/boot/bzImage")
    return os.path.exists(caseBCDir) and os.path.exists(bc_bzImage_path)
        
    
    # return all fail cases, [] if all succeed
def IsCompilationSuccessful():
    return [
        datapoint['idx'] for datapoint in Config.datapoints if not IsCompilationSuccessfulByCase(Config.getBitcodeDirByCase(datapoint['idx']))
    ]

if __name__ == "__main__":
    if sys.argv[1] == "fuzzing":
        CompileKernelToBitcodeForFuzzing()
    elif sys.argv[1] == "analyze":
        CompileKernelToBitcodeForAnalysis()
    else:
        print("need to configure compilation mode: fuzzing or analyze")
