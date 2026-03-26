#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheTC32Target() {
  static Target TheTC32Target;
  return TheTC32Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32TargetInfo() {
  RegisterTarget<Triple::tc32> X(getTheTC32Target(), "tc32", "Telink TC32",
                                 "TC32");
}
