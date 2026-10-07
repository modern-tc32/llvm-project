//===-- Thumb1InstrInfo.cpp - Thumb-1 Instruction Information -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the Thumb-1 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "Thumb1InstrInfo.h"
#include "ARMSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/LiveRegUnits.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"

using namespace llvm;

Thumb1InstrInfo::Thumb1InstrInfo(const ARMSubtarget &STI)
    : ARMBaseInstrInfo(STI, RI), RI(STI) {}

/// Return the noop instruction to use for a noop.
MCInst Thumb1InstrInfo::getNop() const {
  return MCInstBuilder(ARM::tMOVr)
      .addReg(ARM::R8)
      .addReg(ARM::R8)
      .addImm(ARMCC::AL)
      .addReg(0);
}

unsigned Thumb1InstrInfo::getUnindexedOpcode(unsigned Opc) const {
  return 0;
}

void Thumb1InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL, Register DestReg,
                                  Register SrcReg, bool KillSrc,
                                  bool RenamableDest, bool RenamableSrc) const {
  // Need to check the arch.
  MachineFunction &MF = *MBB.getParent();
  const ARMSubtarget &st = MF.getSubtarget<ARMSubtarget>();

  assert(ARM::GPRRegClass.contains(DestReg, SrcReg) &&
         "Thumb1 can only copy GPR registers");

  if (st.hasV6Ops() || ARM::hGPRRegClass.contains(SrcReg) ||
      !ARM::tGPRRegClass.contains(DestReg))
    BuildMI(MBB, I, DL, get(ARM::tMOVr), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .add(predOps(ARMCC::AL));
  else {
    const TargetRegisterInfo *RegInfo = st.getRegisterInfo();
    LiveRegUnits UsedRegs(*RegInfo);
    UsedRegs.addLiveOuts(MBB);

    auto InstUpToI = MBB.end();
    while (InstUpToI != I) {
      // The pre-decrement is on purpose here.
      // We want to have the liveness right before I.
      --InstUpToI;
      if (InstUpToI->isDebugInstr())
        continue;
      UsedRegs.stepBackward(*InstUpToI);
    }

    if (UsedRegs.available(ARM::CPSR)) {
      BuildMI(MBB, I, DL, get(ARM::tMOVSr), DestReg)
          .addReg(SrcReg, getKillRegState(KillSrc))
          ->addRegisterDead(ARM::CPSR, RegInfo);
      return;
    }

    // Use high register to move source to destination
    // if movs is not an option.
    BitVector Allocatable = RegInfo->getAllocatableSet(
        MF, RegInfo->getRegClass(ARM::hGPRRegClassID));

    Register TmpReg = ARM::NoRegister;
    // Prefer R12 as it is known to not be preserved anyway
    if (UsedRegs.available(ARM::R12) && Allocatable.test(ARM::R12)) {
      TmpReg = ARM::R12;
    } else {
      for (Register Reg : Allocatable.set_bits()) {
        if (UsedRegs.available(Reg)) {
          TmpReg = Reg;
          break;
        }
      }
    }

    if (TmpReg) {
      BuildMI(MBB, I, DL, get(ARM::tMOVr), TmpReg)
          .addReg(SrcReg, getKillRegState(KillSrc))
          .add(predOps(ARMCC::AL));
      BuildMI(MBB, I, DL, get(ARM::tMOVr), DestReg)
          .addReg(TmpReg, getKillRegState(true))
          .add(predOps(ARMCC::AL));
      return;
    }

    // 'MOV lo, lo' is unpredictable on < v6, so use the stack to do it
    BuildMI(MBB, I, DL, get(ARM::tPUSH))
        .add(predOps(ARMCC::AL))
        .addReg(SrcReg, getKillRegState(KillSrc));
    BuildMI(MBB, I, DL, get(ARM::tPOP))
        .add(predOps(ARMCC::AL))
        .addReg(DestReg, getDefRegState(true));
  }
}

// PHI-elimination places the source copy right before the block terminators,
// which is after a flag-setting compare feeding the branch. On TC32 a low
// register copy writes N/Z, so after RA it would clobber the branch condition
// and copyPhysReg has to detour through a high register. Hoist the copy above
// the compare instead, when that is legal, so it becomes a single tmov.
MachineInstr *Thumb1InstrInfo::createPHISourceCopy(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator InsPt,
    const DebugLoc &DL, Register Src, unsigned SrcSubReg,
    Register Dst) const {
  if (!getSubtarget().getTargetTriple().isTC32() || SrcSubReg != 0 ||
      !Src.isVirtual() || !Dst.isVirtual())
    return TargetInstrInfo::createPHISourceCopy(MBB, InsPt, DL, Src, SrcSubReg,
                                                Dst);

  // Step back over copies already placed before the terminators, so that
  // several PHI copies for one edge all end up above the compare. A copy that
  // defines Src must stay where it is, or the new copy would read a stale Src.
  MachineBasicBlock::iterator Pos = InsPt;
  while (Pos != MBB.begin()) {
    const MachineInstr &Prev = *std::prev(Pos);
    if (!Prev.isCopy() || Prev.getOperand(0).getReg() == Src)
      break;
    --Pos;
  }
  if (Pos == MBB.begin())
    return TargetInstrInfo::createPHISourceCopy(MBB, InsPt, DL, Src, SrcSubReg,
                                                Dst);

  const MachineInstr &Cmp = *std::prev(Pos);
  if (Cmp.getOpcode() != ARM::tCMPi8 && Cmp.getOpcode() != ARM::tCMPr)
    return TargetInstrInfo::createPHISourceCopy(MBB, InsPt, DL, Src, SrcSubReg,
                                                Dst);
  // PHI elimination marks the copy as the last use of Src, so the compare
  // must not read Src (or the new destination) after the copy.
  for (const MachineOperand &MO : Cmp.operands())
    if (MO.isReg() && (MO.getReg() == Dst || MO.getReg() == Src))
      return TargetInstrInfo::createPHISourceCopy(MBB, InsPt, DL, Src,
                                                  SrcSubReg, Dst);

  return BuildMI(MBB, std::prev(Pos), DL, get(TargetOpcode::COPY), Dst)
      .addReg(Src);
}

void Thumb1InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator I,
                                          Register SrcReg, bool isKill, int FI,
                                          const TargetRegisterClass *RC,
                                          Register VReg,
                                          MachineInstr::MIFlag Flags) const {
  assert((RC == &ARM::tGPRRegClass ||
          (SrcReg.isPhysical() && isARMLowRegister(SrcReg))) &&
         "Unknown regclass!");

  if (RC == &ARM::tGPRRegClass ||
      (SrcReg.isPhysical() && isARMLowRegister(SrcReg))) {
    DebugLoc DL;
    if (I != MBB.end()) DL = I->getDebugLoc();

    MachineFunction &MF = *MBB.getParent();
    MachineFrameInfo &MFI = MF.getFrameInfo();
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getFixedStack(MF, FI), MachineMemOperand::MOStore,
        MFI.getObjectSize(FI), MFI.getObjectAlign(FI));
    BuildMI(MBB, I, DL, get(ARM::tSTRspi))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FI)
        .addImm(0)
        .addMemOperand(MMO)
        .add(predOps(ARMCC::AL));
  }
}

void Thumb1InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                           MachineBasicBlock::iterator I,
                                           Register DestReg, int FI,
                                           const TargetRegisterClass *RC,
                                           Register VReg, unsigned SubReg,
                                           MachineInstr::MIFlag Flags) const {
  assert((RC->hasSuperClassEq(&ARM::tGPRRegClass) ||
          (DestReg.isPhysical() && isARMLowRegister(DestReg))) &&
         "Unknown regclass!");

  if (RC->hasSuperClassEq(&ARM::tGPRRegClass) ||
      (DestReg.isPhysical() && isARMLowRegister(DestReg))) {
    DebugLoc DL;
    if (I != MBB.end()) DL = I->getDebugLoc();

    MachineFunction &MF = *MBB.getParent();
    MachineFrameInfo &MFI = MF.getFrameInfo();
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getFixedStack(MF, FI), MachineMemOperand::MOLoad,
        MFI.getObjectSize(FI), MFI.getObjectAlign(FI));
    BuildMI(MBB, I, DL, get(ARM::tLDRspi), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addMemOperand(MMO)
        .add(predOps(ARMCC::AL));
  }
}

void Thumb1InstrInfo::expandLoadStackGuard(
    MachineBasicBlock::iterator MI) const {
  MachineFunction &MF = *MI->getParent()->getParent();
  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const auto *GV = cast<GlobalValue>((*MI->memoperands_begin())->getValue());

  assert(MF.getFunction().getParent()->getStackProtectorGuard() != "tls" &&
         "TLS stack protector not supported for Thumb1 targets");

  unsigned Instr;
  if (!GV->isDSOLocal())
    Instr = ARM::tLDRLIT_ga_pcrel;
  else if (ST.genExecuteOnly() && ST.hasV8MBaselineOps())
    Instr = ARM::t2MOVi32imm;
  else if (ST.genExecuteOnly())
    Instr = ARM::tMOVi32imm;
  else
    Instr = ARM::tLDRLIT_ga_abs;
  expandLoadStackGuardBase(MI, Instr, ARM::tLDRi);
}

bool Thumb1InstrInfo::canCopyGluedNodeDuringSchedule(SDNode *N) const {
  // In Thumb1 the scheduler may need to schedule a cross-copy between GPRS and CPSR
  // but this is not always possible there, so allow the Scheduler to clone tADCS and tSBCS
  // even if they have glue.
  // FIXME. Actually implement the cross-copy where it is possible (post v6)
  // because these copies entail more spilling.
  unsigned Opcode = N->getMachineOpcode();
  if (Opcode == ARM::tADCS || Opcode == ARM::tSBCS)
    return true;

  return false;
}
