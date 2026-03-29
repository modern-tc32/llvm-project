#include "TC32RegisterInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32FrameLowering.h"
#include "TC32Subtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static int getFixedObjectOffsetAdjustment(const MachineFunction &MF) {
  const auto *TFL =
      static_cast<const TC32FrameLowering *>(MF.getSubtarget().getFrameLowering());
  return static_cast<int>(TFL->getFixedObjectBaseOffset(MF));
}

#define GET_REGINFO_TARGET_DESC
#include "TC32GenRegisterInfo.inc"

TC32RegisterInfo::TC32RegisterInfo() : TC32GenRegisterInfo(TC32::R15) {}

const MCPhysReg *TC32RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  if (MF && !MF->getSubtarget<TC32Subtarget>().isR7Reserved(*MF))
    return CSR_TC32_SaveList;
  return CSR_TC32_NoR7_SaveList;
}

const uint32_t *TC32RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                       CallingConv::ID CC) const {
  (void)CC;
  if (!MF.getSubtarget<TC32Subtarget>().isR7Reserved(MF))
    return CSR_TC32_RegMask;
  return CSR_TC32_NoR7_RegMask;
}

BitVector TC32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(TC32::CPSR);
  if (MF.getSubtarget<TC32Subtarget>().isR7Reserved(MF))
    Reserved.set(TC32::R7);
  Reserved.set(TC32::R8);
  Reserved.set(TC32::R9);
  Reserved.set(TC32::R10);
  Reserved.set(TC32::R11);
  Reserved.set(TC32::R12);
  Reserved.set(TC32::R13);
  Reserved.set(TC32::R14);
  Reserved.set(TC32::R15);
  return Reserved;
}

const TargetRegisterClass *TC32RegisterInfo::getPointerRegClass(unsigned Kind) const {
  (void)Kind;
  return &TC32::LoGR32RegClass;
}

bool TC32RegisterInfo::requiresRegisterScavenging(
    const MachineFunction &MF) const {
  (void)MF;
  return true;
}

bool TC32RegisterInfo::requiresFrameIndexScavenging(
    const MachineFunction &MF) const {
  (void)MF;
  return false;
}

bool TC32RegisterInfo::requiresFrameIndexReplacementScavenging(
    const MachineFunction &MF) const {
  (void)MF;
  return true;
}

bool TC32RegisterInfo::useFPForScavengingIndex(
    const MachineFunction &MF) const {
  (void)MF;
  // TC32 has wide positive SP-relative accesses, while FP-relative accesses
  // are tighter. Keep the emergency spill slot near SP so scavenging can
  // always reach it without recursive frame-index materialization.
  return false;
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
  const bool IsScavengingFI = RS && RS->isScavengingFrameIndex(FI);

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
  if (IsScavengingFI) {
    assert(TFI->hasReservedCallFrame(MF) &&
           "cannot address TC32 scavenging slot from SP without reserved call frame");
    assert(!MFI.hasVarSizedObjects() &&
           "cannot address TC32 scavenging slot from SP with var-sized objects");
    BaseReg = TC32::R13;
  } else if (Fixed) {
    FrameImm += getFixedObjectOffsetAdjustment(MF);
  }

  auto getScratchReg = [&]() -> Register {
    assert(RS && "TC32 frame index elimination needs scavenging for far offsets");
    Register Scratch = RS->FindUnusedReg(&TC32::LoGR32RegClass);
    if (!Scratch)
      Scratch = RS->scavengeRegisterBackwards(TC32::LoGR32RegClass, II, false,
                                              SPAdj);
    if (!Scratch)
      report_fatal_error("unable to scavenge TC32 low scratch register");
    RS->setRegUsed(Scratch);
    return Scratch;
  };

  auto materializeByteAddr = [&](Register ScratchReg, int EffectiveImm) -> bool {
    if (EffectiveImm < 0 || EffectiveImm > 255)
      return false;

    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();
    if (BaseReg == TC32::R13 && (EffectiveImm & 3) == 0 && EffectiveImm <= 252) {
      BuildMI(MBB, II, DL, TII->get(TC32::TADDdstspu8), ScratchReg)
          .addImm(EffectiveImm);
    } else {
      BuildMI(MBB, II, DL, TII->get(TC32::TMOVrr), ScratchReg).addReg(BaseReg);
      BuildMI(MBB, II, DL, TII->get(TC32::TADDSri8), ScratchReg)
          .addReg(ScratchReg)
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

    if (BaseReg == TC32::R13 && FrameImm <= 252 && (FrameImm & 3) == 0) {
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
    Register ScratchReg = getScratchReg();
    if (materializeByteAddr(ScratchReg, FrameImm)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeByteAddr(ScratchReg, FrameImm)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeByteAddr(ScratchReg, FrameImm)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADBrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeByteAddr(ScratchReg, FrameImm)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTOREBrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeFrameAddr(MI, ScratchReg)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeFrameAddr(MI, ScratchReg)) {
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeFrameAddr(MI, ScratchReg)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TLOADrr));
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
    Register ScratchReg = getScratchReg();
    if (materializeFrameAddr(MI, ScratchReg)) {
      MI.setDesc(MF.getSubtarget().getInstrInfo()->get(TC32::TSTORErr));
      MI.getOperand(FIOperandNum).ChangeToRegister(ScratchReg, false);
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
