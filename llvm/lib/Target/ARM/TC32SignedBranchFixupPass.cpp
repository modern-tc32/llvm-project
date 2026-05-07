//===-- TC32SignedBranchFixupPass.cpp - TC32 signed branch fixup ----------===//
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
    return "TC32 signed branch fixup";
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
                "TC32 signed branch fixup", false, false)

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

static bool needsTC32SignedBranchFixup(ARMCC::CondCodes CC) {
  return CC == ARMCC::GE || CC == ARMCC::PL;
}

static unsigned getUncondOpcode(unsigned CondOpcode) {
  return CondOpcode == ARM::tTC32Bcc32 ? ARM::tTC32B32 : ARM::tB;
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
  if (!needsTC32SignedBranchFixup(CC))
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

bool TC32SignedBranchFixup::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const ARMBaseInstrInfo *TII = ST.getInstrInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF)
    Changed |= rewriteTC32SignedBranch(MBB, TII);

  return Changed;
}

FunctionPass *llvm::createTC32SignedBranchFixupPass() {
  return new TC32SignedBranchFixup();
}
