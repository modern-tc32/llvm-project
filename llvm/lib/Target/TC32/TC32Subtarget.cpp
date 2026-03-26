#include "TC32Subtarget.h"

using namespace llvm;

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "TC32GenSubtargetInfo.inc"

TC32Subtarget &TC32Subtarget::initializeSubtargetDependencies(StringRef CPU,
                                                              StringRef FS) {
  StringRef CPUName = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(CPUName, CPUName, FS);
  return *this;
}

TC32Subtarget::TC32Subtarget(const Triple &TT, const std::string &CPU,
                             const std::string &FS, const TargetMachine &TM)
    : TC32GenSubtargetInfo(TT, CPU, CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), TLInfo(TM, *this),
      FrameLowering(*this) {}
