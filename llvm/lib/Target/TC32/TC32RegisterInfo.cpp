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
  static const MCPhysReg CalleeSavedRegsNoFP[] = {
      TC32::R4,
      TC32::R5,
      TC32::R6,
      TC32::R7,
      0};
  static const MCPhysReg CalleeSavedRegsWithFP[] = {
      TC32::R4,
      TC32::R5,
      TC32::R6,
      0};
  if (MF && MF->getSubtarget().getFrameLowering()->hasFP(*MF))
    return CalleeSavedRegsWithFP;
  return CalleeSavedRegsNoFP;
}

BitVector TC32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(TC32::CPSR);
  Reserved.set(TC32::R6);
  Reserved.set(TC32::R8);
  Reserved.set(TC32::R9);
  Reserved.set(TC32::R10);
  Reserved.set(TC32::R11);
  Reserved.set(TC32::R12);
  if (MF.getSubtarget().getFrameLowering()->hasFP(MF))
    Reserved.set(TC32::R7);
  Reserved.set(TC32::R13);
  Reserved.set(TC32::R14);
  Reserved.set(TC32::R15);
  return Reserved;
}

const TargetRegisterClass *TC32RegisterInfo::getPointerRegClass(unsigned Kind) const {
  (void)Kind;
  return &TC32::LoGR32RegClass;
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
  case TC32::TLOADBru3:
  case TC32::TSTOREBru3:
    ExtraImm = MI.getOperand(FIOperandNum + 1).getImm();
    break;
  case TC32::TLOADru3:
  case TC32::TSTOREru3:
    ExtraImm = MI.getOperand(FIOperandNum + 1).getImm();
    break;
  default:
    break;
  }

  unsigned BaseReg = TFI->hasFP(MF) ? TC32::R7 : TC32::R13;
  int FrameImm = (Fixed ? Offset : Offset + StackSize) + ExtraImm;
  if (TFI->hasFP(MF) && Fixed) {
    BaseReg = TC32::R7;
    FrameImm += StackSize + 8;
  }

  auto materializeByteAddr = [&](int EffectiveImm) -> bool {
    if (EffectiveImm < 0 || EffectiveImm > 255)
      return false;

    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();
    if (BaseReg == TC32::R13) {
      BuildMI(MBB, II, DL, TII->get(TC32::TADDdstspu8), TC32::R6).addImm(EffectiveImm);
    } else {
      BuildMI(MBB, II, DL, TII->get(TC32::TMOVrr), TC32::R6).addReg(BaseReg);
      BuildMI(MBB, II, DL, TII->get(TC32::TADDSri8), TC32::R6)
          .addReg(TC32::R6)
          .addImm(EffectiveImm);
    }
    return true;
  };

  auto materializeFrameAddr = [&](MachineInstr &FrameMI, Register DestReg) -> bool {
    if (FrameImm < 0)
      return false;

    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    DebugLoc DL = FrameMI.getDebugLoc();
    MachineBasicBlock::iterator InsertPt = FrameMI;

    if (BaseReg == TC32::R13 && FrameImm <= 255) {
      BuildMI(MBB, InsertPt, DL, TII->get(TC32::TADDdstspu8), DestReg)
          .addImm(FrameImm);
      return true;
    }

    BuildMI(MBB, InsertPt, DL, TII->get(TC32::TMOVrr), DestReg).addReg(BaseReg);

    int Remaining = FrameImm;
    InsertPt = std::next(InsertPt);
    while (Remaining > 255) {
      BuildMI(MBB, InsertPt, DL, TII->get(TC32::TADDSri8), DestReg)
          .addReg(DestReg)
          .addImm(255);
      Remaining -= 255;
    }
    if (Remaining > 0) {
      BuildMI(MBB, InsertPt, DL, TII->get(TC32::TADDSri8), DestReg)
          .addReg(DestReg)
          .addImm(Remaining);
    }
    return true;
  };

  switch (MI.getOpcode()) {
  case TC32::TMOVrr: {
    Register DestReg = MI.getOperand(0).getReg();
    if (materializeFrameAddr(MI, DestReg)) {
      MI.eraseFromParent();
      return true;
    }
    break;
  }
  case TC32::TLOADBrr: {
    if (FrameImm >= 0 && FrameImm <= 7) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(
          FrameImm == 0 ? TC32::TLOADBrr : TC32::TLOADBru3));
      MI.getOperand(FIOperandNum).ChangeToRegister(BaseReg, false);
      if (FrameImm != 0)
        MI.addOperand(MachineOperand::CreateImm(FrameImm));
      return false;
    }
    if (materializeByteAddr(FrameImm)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
      return false;
    }
    break;
  }
  case TC32::TSTOREBrr: {
    if (FrameImm >= 0 && FrameImm <= 7) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(
          FrameImm == 0 ? TC32::TSTOREBrr : TC32::TSTOREBru3));
      MI.getOperand(FIOperandNum).ChangeToRegister(BaseReg, false);
      if (FrameImm != 0)
        MI.addOperand(MachineOperand::CreateImm(FrameImm));
      return false;
    }
    if (materializeByteAddr(FrameImm)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
      return false;
    }
    break;
  }
  case TC32::TLOADBru3: {
    if (FrameImm >= 0 && FrameImm <= 7) {
      MI.getOperand(FIOperandNum).ChangeToRegister(BaseReg, false);
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(FrameImm);
      return false;
    }
    if (materializeByteAddr(FrameImm)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADBrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    break;
  }
  case TC32::TSTOREBru3: {
    if (FrameImm >= 0 && FrameImm <= 7) {
      MI.getOperand(FIOperandNum).ChangeToRegister(BaseReg, false);
      MI.getOperand(FIOperandNum + 1).ChangeToImmediate(FrameImm);
      return false;
    }
    if (materializeByteAddr(FrameImm)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREBrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
      MI.removeOperand(FIOperandNum + 1);
      return false;
    }
    break;
  }
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
    if (materializeFrameAddr(MI, TC32::R6)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
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
    if (materializeFrameAddr(MI, TC32::R6)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
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
    if (materializeFrameAddr(MI, TC32::R6)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
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
    if (materializeFrameAddr(MI, TC32::R6)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTORErr));
      MI.getOperand(FIOperandNum).ChangeToRegister(TC32::R6, false);
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
