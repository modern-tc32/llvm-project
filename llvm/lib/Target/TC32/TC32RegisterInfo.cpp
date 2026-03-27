#include "TC32RegisterInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32FrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "TC32GenRegisterInfo.inc"

TC32RegisterInfo::TC32RegisterInfo() : TC32GenRegisterInfo(TC32::R15) {}

const MCPhysReg *TC32RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg CalleeSavedRegs[] = {
      TC32::R4,
      TC32::R5,
      0};
  (void)MF;
  return CalleeSavedRegs;
}

BitVector TC32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(TC32::R6);
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
  (void)SPAdj;
  (void)RS;

  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();

  int FI = MI.getOperand(FIOperandNum).getIndex();
  bool Fixed = MFI.isFixedObjectIndex(FI);
  int Offset = MFI.getObjectOffset(FI);
  int StackSize = MFI.getStackSize();
  int ExtraImm = 0;

  switch (MI.getOpcode()) {
  case TC32::TLOADru3:
  case TC32::TSTOREru3:
    ExtraImm = MI.getOperand(FIOperandNum + 1).getImm();
    break;
  default:
    break;
  }

  unsigned BaseReg = Fixed ? TC32::R13 : TC32::R7;
  int FrameImm = (Fixed ? Offset : Offset + StackSize) + ExtraImm;
  if (TFI->hasFP(MF) && Fixed) {
    BaseReg = TC32::R7;
    FrameImm += StackSize + 8;
  }

  switch (MI.getOpcode()) {
  case TC32::TLOADrr: {
    if (BaseReg == TC32::R13 && FrameImm >= 0 && FrameImm <= 1020 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADspu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      return false;
    }
    if (BaseReg == TC32::R7 && FrameImm >= 0 && FrameImm <= 252 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADfpu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      return false;
    }
    break;
  }
  case TC32::TSTORErr: {
    if (BaseReg == TC32::R13 && FrameImm >= 0 && FrameImm <= 1020 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREspu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      return false;
    }
    if (BaseReg == TC32::R7 && FrameImm >= 0 && FrameImm <= 252 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREfpu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      return false;
    }
    break;
  }
  case TC32::TLOADru3: {
    if (BaseReg == TC32::R13 && FrameImm >= 0 && FrameImm <= 1020 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADspu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    if (BaseReg == TC32::R7 && FrameImm >= 0 && FrameImm <= 252 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADfpu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    break;
  }
  case TC32::TSTOREru3: {
    if (BaseReg == TC32::R13 && FrameImm >= 0 && FrameImm <= 1020 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREspu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    if (BaseReg == TC32::R7 && FrameImm >= 0 && FrameImm <= 252 &&
        (FrameImm & 3) == 0) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREfpu8));
      MI.getOperand(FIOperandNum).ChangeToImmediate(FrameImm);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    break;
  }
  default:
    break;
  }

  report_fatal_error("TC32 frame index address does not fit supported spill forms");
}

Register TC32RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MF.getSubtarget().getFrameLowering()->hasFP(MF) ? TC32::R7
                                                         : TC32::R13;
}
