#ifndef LLVM_LIB_TARGET_TC32_TC32INSTRINFO_H
#define LLVM_LIB_TARGET_TC32_TC32INSTRINFO_H

#include "TC32RegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "TC32GenInstrInfo.inc"

namespace llvm {

class TC32Subtarget;

class TC32InstrInfo : public TC32GenInstrInfo {
  const TC32RegisterInfo RI;

public:
  explicit TC32InstrInfo(const TC32Subtarget &STI);

  const TC32RegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;
};

} // namespace llvm

#endif
