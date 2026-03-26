#include "TC32InstrInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "TC32GenInstrInfo.inc"

TC32InstrInfo::TC32InstrInfo(const TC32Subtarget &STI)
    : TC32GenInstrInfo(STI, RI, TC32::ADJCALLSTACKDOWN, TC32::ADJCALLSTACKUP),
      RI() {}

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
  if (RC != &TC32::LoGR32RegClass && RC != &TC32::GR32RegClass)
    report_fatal_error("TC32 spill only supports GPRs");

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
  if (RC != &TC32::LoGR32RegClass && RC != &TC32::GR32RegClass)
    report_fatal_error("TC32 reload only supports GPRs");

  BuildMI(MBB, MI, DebugLoc(), get(TC32::TLOADrr), DestReg)
      .addFrameIndex(FrameIndex)
      .setMIFlags(Flags);
}

unsigned TC32InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  return get(MI.getOpcode()).getSize();
}
