#ifndef LLVM_LIB_TARGET_TC32_TC32MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_TC32_TC32MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class TC32MachineFunctionInfo : public MachineFunctionInfo {
  int BranchRelaxationScratchFrameIndex = -1;

public:
  explicit TC32MachineFunctionInfo(const Function &F,
                                   const TargetSubtargetInfo *STI) {}

  int getBranchRelaxationScratchFrameIndex() const {
    return BranchRelaxationScratchFrameIndex;
  }

  void setBranchRelaxationScratchFrameIndex(int Index) {
    BranchRelaxationScratchFrameIndex = Index;
  }
};

} // namespace llvm

#endif
