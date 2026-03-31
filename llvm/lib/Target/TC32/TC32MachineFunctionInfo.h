#ifndef LLVM_LIB_TARGET_TC32_TC32MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_TC32_TC32MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class TC32MachineFunctionInfo : public MachineFunctionInfo {
  int BranchRelaxationScratchFrameIndex = -1;
  uint8_t PackedCalleeSavedMask = 0;
  uint8_t PackedCalleeSavedBytes = 0;

public:
  explicit TC32MachineFunctionInfo(const Function &F,
                                   const TargetSubtargetInfo *STI) {}

  int getBranchRelaxationScratchFrameIndex() const {
    return BranchRelaxationScratchFrameIndex;
  }

  void setBranchRelaxationScratchFrameIndex(int Index) {
    BranchRelaxationScratchFrameIndex = Index;
  }

  uint8_t getPackedCalleeSavedMask() const { return PackedCalleeSavedMask; }
  void setPackedCalleeSavedMask(uint8_t Mask) { PackedCalleeSavedMask = Mask; }

  uint8_t getPackedCalleeSavedBytes() const { return PackedCalleeSavedBytes; }
  void setPackedCalleeSavedBytes(uint8_t Bytes) {
    PackedCalleeSavedBytes = Bytes;
  }
};

} // namespace llvm

#endif
