import subprocess
import logging
import os
import shutil

EmitScriptPath = "/home/clexma/Desktop/fox3/fuzzing/linuxRepo/llvm_compile/emit-llvm.sh"
BCDir = "/home/clexma/Desktop/fox3/fuzzing/linuxRepo/llvm_compile/bc_dir"
configPath="/home/clexma/Desktop/fox3/fuzzing/linuxRepo/linux_new"
ClangPath="/usr/local/bin/clang"
kernelAnalyzeResDir = "/home/clexma/Desktop/fox3/fuzzing/linuxRepo/llvm_compile/analyze_result"
    
def CompileKernelToBitcodeNormal():
    emit_contents=f'''#!/bin/sh
CLANG={ClangPath}
if [ ! -e $CLANG ]
then
    exit
fi
OFILE=`echo $* | sed -e 's/^.* \(.*\.o\) .*$/\\1/'`
if [ "x$OFILE" != x -a "$OFILE" != "$*" ] ; then
    $CLANG -emit-llvm -g "$@" >/dev/null 2>&1 > /dev/null
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
    
    caseBCDir=BCDir
    
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
            
        

    

def IsCompilationSuccessfulByCase(caseBCDir):
    bc_bzImage_path = os.path.join(caseBCDir, "arch/x86/boot/bzImage")
    return os.path.exists(caseBCDir) and os.path.exists(bc_bzImage_path)
        
    
    # return all fail cases, [] if all succeed
def IsCompilationSuccessful():
    return [
        datapoint['idx'] for datapoint in Config.datapoints if not IsCompilationSuccessfulByCase(Config.getBitcodeDirByCase(datapoint['idx']))
    ]

def ConstructCallGraphForKernel():
    #function_modeling
    bitcodeDir = BCDir



if __name__ == "__main__":
    CompileKernelToBitcodeNormal()