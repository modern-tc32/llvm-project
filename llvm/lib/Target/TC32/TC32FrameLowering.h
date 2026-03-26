#ifndef LLVM_LIB_TARGET_TC32_TC32FRAMELOWERING_H
#define LLVM_LIB_TARGET_TC32_TC32FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class TC32Subtarget;

class TC32FrameLowering : public TargetFrameLowering {
protected:
  bool hasFPImpl(const MachineFunction &MF) const override;

public:
  explicit TC32FrameLowering(const TC32Subtarget &STI);

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
};

} // namespace llvm

#endif
