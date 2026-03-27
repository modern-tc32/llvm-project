#ifndef LLVM_LIB_TARGET_TC32_TC32SUBTARGET_H
#define LLVM_LIB_TARGET_TC32_TC32SUBTARGET_H

#include "TC32FrameLowering.h"
#include "TC32ISelLowering.h"
#include "TC32InstrInfo.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include <memory>

#define GET_SUBTARGETINFO_HEADER
#include "TC32GenSubtargetInfo.inc"

namespace llvm {

class LibcallLoweringInfo;

class TC32Subtarget : public TC32GenSubtargetInfo {
  TC32InstrInfo InstrInfo;
  TC32TargetLowering TLInfo;
  TC32FrameLowering FrameLowering;
  SelectionDAGTargetInfo TSInfo;

public:
  TC32Subtarget(const Triple &TT, const std::string &CPU,
                const std::string &FS, const TargetMachine &TM);

  TC32Subtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);
  void initLibcallLoweringInfo(LibcallLoweringInfo &Info) const override;

  const TC32InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const TC32RegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const TC32TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  const TargetFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
};

} // namespace llvm

#endif
