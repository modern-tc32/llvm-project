#include "TC32InstrInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32Subtarget.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "TC32GenInstrInfo.inc"

TC32InstrInfo::TC32InstrInfo(const TC32Subtarget &STI)
    : TC32GenInstrInfo(STI, RI), RI() {}

void TC32InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register DestReg,
                                Register SrcReg, bool KillSrc,
                                bool RenamableDest,
                                bool RenamableSrc) const {
  BuildMI(MBB, I, DL, get(TC32::TMOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

unsigned TC32InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  return get(MI.getOpcode()).getSize();
}
