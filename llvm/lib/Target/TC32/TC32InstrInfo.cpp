#include "TC32InstrInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "TC32GenInstrInfo.inc"

static int getFixedObjectOffsetAdjustment(const MachineFunction &MF) {
  const auto *TFL =
      static_cast<const TC32FrameLowering *>(MF.getSubtarget().getFrameLowering());
  return static_cast<int>(TFL->getFixedObjectBaseOffset(MF));
}

static bool isLowPhysReg(Register Reg) {
  return Reg.isPhysical() && TC32::LoGR32RegClass.contains(Reg);
}

static bool isLowValueReg(Register Reg, const MachineRegisterInfo &MRI) {
  if (isLowPhysReg(Reg))
    return true;
  if (!Reg.isVirtual())
    return false;
  return MRI.getRegClass(Reg) == &TC32::LoGR32RegClass;
}

TC32InstrInfo::TC32InstrInfo(const TC32Subtarget &STI)
    : TC32GenInstrInfo(STI, RI, TC32::ADJCALLSTACKDOWN, TC32::ADJCALLSTACKUP),
      RI() {}

bool TC32InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case TC32::TFRAMEADDR: {
    assert(MI.getOperand(0).isReg() && "unexpected TFRAMEADDR def");
    Register DstReg = MI.getOperand(0).getReg();
    MachineBasicBlock &MBB = *MI.getParent();
    MachineFunction &MF = *MBB.getParent();
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();

    int FI = MI.getOperand(1).getImm();
    int ExtraImm = MI.getOperand(2).getImm();
    bool Fixed = MFI.isFixedObjectIndex(FI);
    int Offset = MFI.getObjectOffset(FI);
    int StackSize = MFI.getStackSize();
    unsigned BaseReg = TFI->hasFP(MF) ? TC32::R7 : TC32::R13;
    int FrameImm = (Fixed ? Offset : Offset + StackSize) + ExtraImm;
    if (Fixed) {
      FrameImm += getFixedObjectOffsetAdjustment(MF);
    }

    if (BaseReg == TC32::R13 && FrameImm >= 0 && FrameImm <= 252 &&
        (FrameImm & 3) == 0) {
      BuildMI(MBB, MI, MI.getDebugLoc(), get(TC32::TADDdstspu8), DstReg)
          .addImm(FrameImm);
    } else {
      BuildMI(MBB, MI, MI.getDebugLoc(), get(TC32::TMOVrr), DstReg).addReg(BaseReg);
      int Remaining = FrameImm;
      while (Remaining > 255) {
        BuildMI(MBB, MI, MI.getDebugLoc(), get(TC32::TADDSri8), DstReg)
            .addReg(DstReg)
            .addImm(255);
        Remaining -= 255;
      }
      if (Remaining > 0) {
        BuildMI(MBB, MI, MI.getDebugLoc(), get(TC32::TADDSri8), DstReg)
            .addReg(DstReg)
            .addImm(Remaining);
      }
    }

    MI.eraseFromParent();
    return true;
  }
  default:
    return false;
  }
}

void TC32InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register DestReg,
                                Register SrcReg, bool KillSrc,
                                bool RenamableDest,
                                bool RenamableSrc) const {
  BuildMI(MBB, I, DL, get(TC32::TMOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

void TC32InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register SrcReg, bool IsKill,
                                        int FrameIndex,
                                        const TargetRegisterClass *RC,
                                        Register VReg,
                                        MachineInstr::MIFlag Flags) const {
  (void)VReg;
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  if (RC != &TC32::LoGR32RegClass && RC != &TC32::GR32RegClass)
    report_fatal_error("TC32 spill only supports GPRs");

  if (!isLowValueReg(SrcReg, MRI)) {
    Register SpillReg = MF.getProperties().hasNoVRegs()
                            ? Register(TC32::R6)
                            : MRI.createVirtualRegister(&TC32::LoGR32RegClass);
    assert((!SpillReg.isPhysical() || SrcReg != TC32::R6) &&
           "reserved low scratch register spilled unexpectedly");
    BuildMI(MBB, MI, DebugLoc(), get(TC32::TMOVrr), SpillReg)
        .addReg(SrcReg, getKillRegState(IsKill))
        .setMIFlags(Flags);
    SrcReg = SpillReg;
    IsKill = true;
  }

  BuildMI(MBB, MI, DebugLoc(), get(TC32::TSTORErr))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .setMIFlags(Flags);
}

void TC32InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MI,
                                         Register DestReg, int FrameIndex,
                                         const TargetRegisterClass *RC,
                                         Register VReg, unsigned SubReg,
                                         MachineInstr::MIFlag Flags) const {
  (void)VReg;
  (void)SubReg;
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  if (RC != &TC32::LoGR32RegClass && RC != &TC32::GR32RegClass)
    report_fatal_error("TC32 reload only supports GPRs");

  if (!isLowValueReg(DestReg, MRI)) {
    Register ReloadReg = MF.getProperties().hasNoVRegs()
                             ? Register(TC32::R6)
                             : MRI.createVirtualRegister(&TC32::LoGR32RegClass);
    assert((!ReloadReg.isPhysical() || DestReg != TC32::R6) &&
           "reserved low scratch register reloaded unexpectedly");
    BuildMI(MBB, MI, DebugLoc(), get(TC32::TLOADrr), ReloadReg)
        .addFrameIndex(FrameIndex)
        .setMIFlags(Flags);
    BuildMI(MBB, MI, DebugLoc(), get(TC32::TMOVrr), DestReg)
        .addReg(ReloadReg, getKillRegState(true))
        .setMIFlags(Flags);
    return;
  }

  BuildMI(MBB, MI, DebugLoc(), get(TC32::TLOADrr), DestReg)
      .addFrameIndex(FrameIndex)
      .setMIFlags(Flags);
}

unsigned TC32InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  return get(MI.getOpcode()).getSize();
}
