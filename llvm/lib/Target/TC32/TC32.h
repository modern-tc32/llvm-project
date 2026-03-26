#ifndef LLVM_LIB_TARGET_TC32_TC32_H
#define LLVM_LIB_TARGET_TC32_TC32_H

#include "llvm/Support/CodeGen.h"

namespace llvm {
class Target;
class FunctionPass;
class TC32TargetMachine;

Target &getTheTC32Target();
FunctionPass *createTC32ISelDag(TC32TargetMachine &TM,
                                CodeGenOptLevel OptLevel);
} // namespace llvm

#endif
