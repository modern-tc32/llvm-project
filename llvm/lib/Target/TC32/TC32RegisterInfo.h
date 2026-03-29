#ifndef LLVM_LIB_TARGET_TC32_TC32REGISTERINFO_H
#define LLVM_LIB_TARGET_TC32_TC32REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "TC32GenRegisterInfo.inc"

namespace llvm {

class TC32RegisterInfo : public TC32GenRegisterInfo {
public:
  TC32RegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  const TargetRegisterClass *getPointerRegClass(unsigned Kind = 0) const override;
  bool requiresRegisterScavenging(const MachineFunction &MF) const override;
  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override;
  bool requiresFrameIndexReplacementScavenging(
      const MachineFunction &MF) const override;
  bool useFPForScavengingIndex(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif
