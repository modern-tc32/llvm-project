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
  bool isR7Reserved(const MachineFunction &MF) const;
  bool usesPushLR(const MachineFunction &MF) const;
  bool usesPushR7LR(const MachineFunction &MF) const;
  unsigned getFixedObjectBaseOffset(const MachineFunction &MF) const;
  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS) const override;

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasReservedCallFrame(const MachineFunction &MF) const override;
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;
};

} // namespace llvm

#endif
