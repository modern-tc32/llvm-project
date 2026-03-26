#include "TC32FrameLowering.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

TC32FrameLowering::TC32FrameLowering(const TC32Subtarget &STI)
    : TargetFrameLowering(StackGrowsDown, Align(4), 0, Align(4)) {
  (void)STI;
}

void TC32FrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  if (!hasFP(MF))
    return;

  MachineBasicBlock::iterator I = MBB.begin();
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TPUSH_R7_LR));

  int StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize > 0)
    BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TSUBspu8))
        .addImm(StackSize);

  BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TADDdstspu8),
          TC32::R7)
      .addImm(0);
}

void TC32FrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  if (!hasFP(MF))
    return;

  MachineBasicBlock::iterator I = MBB.getFirstTerminator();
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TMOVrr),
          TC32::R13)
      .addReg(TC32::R7);

  int StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize > 0)
    BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TADDspu8))
        .addImm(StackSize);

  if (I != MBB.end() &&
      (I->getOpcode() == TC32::TRET || I->getOpcode() == TC32::TRET_R0))
    I = MBB.erase(I);

  BuildMI(MBB, I, DL, MF.getSubtarget().getInstrInfo()->get(TC32::TPOP_R7_PC));
}

bool TC32FrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo().hasVarSizedObjects();
}

MachineBasicBlock::iterator TC32FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  if (!hasReservedCallFrame(MF)) {
    int64_t Amount = MI->getOperand(0).getImm();
    DebugLoc DL = MI->getDebugLoc();

    if (Amount != 0) {
      unsigned Opc = MI->getOpcode() == TC32::ADJCALLSTACKDOWN
                         ? TC32::TSUBspu8
                         : TC32::TADDspu8;
      BuildMI(MBB, MI, DL, MF.getSubtarget().getInstrInfo()->get(Opc))
          .addImm(Amount);
    }
  }

  return MBB.erase(MI);
}

bool TC32FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.getStackSize() != 0 || MFI.hasCalls();
}
