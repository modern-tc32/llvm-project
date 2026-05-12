//===-- TC32SignedBranchFixupPass.cpp - TC32 branch condition fixup -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMBaseInstrInfo.h"
#include "ARMSubtarget.h"
#include "Utils/ARMBaseInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-signed-branch-fixup"

namespace {

class TC32SignedBranchFixup : public MachineFunctionPass {
public:
  static char ID;

  TC32SignedBranchFixup() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 branch condition fixup";
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

char TC32SignedBranchFixup::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(TC32SignedBranchFixup, DEBUG_TYPE,
                "TC32 branch condition fixup", false, false)

static bool isTC32UncondBranch(const MachineInstr &MI) {
  return MI.getOpcode() == ARM::tB || MI.getOpcode() == ARM::tTC32B32;
}

static MachineBasicBlock::iterator
skipDebugBackward(MachineBasicBlock &MBB, MachineBasicBlock::iterator I) {
  while (I != MBB.begin()) {
    --I;
    if (!I->isDebugInstr())
      return I;
  }
  return MBB.end();
}

static bool needsTC32BranchConditionFixup(ARMCC::CondCodes CC) {
  return CC == ARMCC::GE || CC == ARMCC::PL || CC == ARMCC::LS;
}

static unsigned getUncondOpcode(unsigned CondOpcode) {
  return CondOpcode == ARM::tTC32Bcc32 ? ARM::tTC32B32 : ARM::tB;
}

static unsigned getTC32BlockSizeInBytes(const MachineBasicBlock &MBB,
                                        const ARMBaseInstrInfo *TII) {
  unsigned Size = 0;
  for (const MachineInstr &MI : MBB) {
    if (MI.isMetaInstruction())
      continue;
    Size += TII->getInstSizeInBytes(MI);
  }
  return Size;
}

static bool rewriteTC32SignedBranch(MachineBasicBlock &MBB,
                                    const ARMBaseInstrInfo *TII) {
  if (MBB.empty())
    return false;

  MachineBasicBlock::iterator Last = MBB.getLastNonDebugInstr();
  if (Last == MBB.end())
    return false;

  MachineInstr *Uncond = nullptr;
  MachineInstr *Cond = nullptr;
  if (isTC32UncondBranch(*Last)) {
    Uncond = &*Last;
    MachineBasicBlock::iterator Prev = skipDebugBackward(MBB, Last);
    if (Prev == MBB.end())
      return false;
    Last = Prev;
  }

  if (Last->getOpcode() != ARM::tBcc && Last->getOpcode() != ARM::tTC32Bcc32)
    return false;

  Cond = &*Last;
  ARMCC::CondCodes CC =
      static_cast<ARMCC::CondCodes>(Cond->getOperand(1).getImm());
  if (!needsTC32BranchConditionFixup(CC))
    return false;

  if (Uncond) {
    MachineBasicBlock *Taken = Cond->getOperand(0).getMBB();
    MachineBasicBlock *Fallthrough = Uncond->getOperand(0).getMBB();
    Cond->getOperand(0).setMBB(Fallthrough);
    Cond->getOperand(1).setImm(ARMCC::getOppositeCondition(CC));
    Uncond->getOperand(0).setMBB(Taken);
    return true;
  }

  MachineBasicBlock *Fallthrough = MBB.getFallThrough();
  if (!Fallthrough)
    return false;

  MachineBasicBlock *Taken = Cond->getOperand(0).getMBB();
  Cond->getOperand(0).setMBB(Fallthrough);
  Cond->getOperand(1).setImm(ARMCC::getOppositeCondition(CC));
  MachineInstrBuilder MIB =
      BuildMI(MBB, std::next(Cond->getIterator()), Cond->getDebugLoc(),
              TII->get(getUncondOpcode(Cond->getOpcode())))
          .addMBB(Taken);
  if (Cond->getOpcode() == ARM::tBcc)
    MIB.add(predOps(ARMCC::AL));
  return true;
}

static bool padTC32ZeroOffsetBranch(MachineBasicBlock &MBB,
                                    const ARMBaseInstrInfo *TII) {
  if (MBB.empty())
    return false;

  MachineBasicBlock::iterator Last = MBB.getLastNonDebugInstr();
  if (Last == MBB.end() || Last->getOpcode() != ARM::tBcc)
    return false;

  MachineBasicBlock *Target = Last->getOperand(0).getMBB();
  MachineBasicBlock *Fallthrough = MBB.getFallThrough();
  if (!Target || !Fallthrough)
    return false;

  auto Next = Fallthrough->getIterator();
  ++Next;
  if (Next == Fallthrough->getParent()->end() || &*Next != Target)
    return false;

  unsigned FallthroughSize = getTC32BlockSizeInBytes(*Fallthrough, TII);
  if (FallthroughSize != 0 && FallthroughSize != 2)
    return false;

  MachineBasicBlock::iterator InsertPt = Fallthrough->getFirstTerminator();
  if (InsertPt == Fallthrough->end())
    InsertPt = Fallthrough->end();
  BuildMI(*Fallthrough, InsertPt, Last->getDebugLoc(), TII->get(ARM::tTC32NOP));
  return true;
}

static bool removeTC32FallthroughBranch(MachineBasicBlock &MBB) {
  if (MBB.empty())
    return false;

  MachineBasicBlock::iterator Last = MBB.getLastNonDebugInstr();
  if (Last == MBB.end() || Last->getOpcode() != ARM::tBcc)
    return false;

  MachineBasicBlock *Fallthrough = MBB.getFallThrough();
  if (!Fallthrough || Last->getOperand(0).getMBB() != Fallthrough)
    return false;

  Last->eraseFromParent();
  return true;
}

bool TC32SignedBranchFixup::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const ARMBaseInstrInfo *TII = ST.getInstrInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF)
    Changed |= rewriteTC32SignedBranch(MBB, TII);

  for (MachineBasicBlock &MBB : MF)
    Changed |= removeTC32FallthroughBranch(MBB);

  for (MachineBasicBlock &MBB : MF)
    Changed |= padTC32ZeroOffsetBranch(MBB, TII);

  return Changed;
}

FunctionPass *llvm::createTC32SignedBranchFixupPass() {
  return new TC32SignedBranchFixup();
}
