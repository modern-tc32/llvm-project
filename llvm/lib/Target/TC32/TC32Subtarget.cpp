#include "TC32Subtarget.h"
#include "llvm/CodeGen/LibcallLoweringInfo.h"

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

void TC32Subtarget::initLibcallLoweringInfo(LibcallLoweringInfo &Info) const {
  auto setBuiltinLibcall = [&](RTLIB::Libcall Call, StringRef Name) {
    for (RTLIB::LibcallImpl Impl :
         RTLIB::RuntimeLibcallsInfo::lookupLibcallImplName(Name)) {
      Info.setLibcallImpl(Call, Impl);
      return;
    }
  };

  setBuiltinLibcall(RTLIB::SDIV_I32, "__divsi3");
  setBuiltinLibcall(RTLIB::UDIV_I32, "__udivsi3");
  setBuiltinLibcall(RTLIB::SREM_I32, "__modsi3");
  setBuiltinLibcall(RTLIB::UREM_I32, "__umodsi3");
}
