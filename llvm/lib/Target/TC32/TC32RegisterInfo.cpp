#include "TC32RegisterInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32FrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "TC32GenRegisterInfo.inc"

TC32RegisterInfo::TC32RegisterInfo() : TC32GenRegisterInfo(TC32::R15) {}

const MCPhysReg *TC32RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg CalleeSavedRegs[] = {0};
  (void)MF;
  return CalleeSavedRegs;
}

BitVector TC32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  if (MF.getSubtarget().getFrameLowering()->hasFP(MF))
    Reserved.set(TC32::R7);
  Reserved.set(TC32::R13);
  Reserved.set(TC32::R14);
  Reserved.set(TC32::R15);
  return Reserved;
}

const TargetRegisterClass *TC32RegisterInfo::getPointerRegClass(unsigned Kind) const {
  (void)Kind;
  return &TC32::GR32RegClass;
}

bool TC32RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                                           unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  (void)II;
  (void)SPAdj;
  (void)FIOperandNum;
  (void)RS;
  report_fatal_error("TC32 frame indices are not implemented yet");
}

Register TC32RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MF.getSubtarget().getFrameLowering()->hasFP(MF) ? TC32::R7
                                                         : TC32::R13;
}
