//===-- TC32DistinctDstRegFixupPass.cpp - TC32 add/sub dst fixup ----------===//
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
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-distinct-dst-reg-fixup"

namespace {

class TC32DistinctDstRegFixup : public MachineFunctionPass {
public:
  static char ID;

  TC32DistinctDstRegFixup() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 distinct-destination add/sub fixup";
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

char TC32DistinctDstRegFixup::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(TC32DistinctDstRegFixup, "tc32-distinct-dst-reg-fixup",
                "TC32 distinct-destination add/sub fixup", false, false)

static bool isUnsafeDistinctDstRegOp(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case ARM::tADDrr:
  case ARM::tSUBrr:
    return MI.getOperand(0).getReg() != MI.getOperand(2).getReg();
  default:
    return false;
  }
}

bool TC32DistinctDstRegFixup::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const ARMBaseInstrInfo *TII = ST.getInstrInfo();
  const TargetRegisterInfo *TRI = ST.getRegisterInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (auto I = MBB.begin(); I != MBB.end();) {
      MachineInstr &MI = *I++;
      auto InsertPt = MI.getIterator();
      if (!isUnsafeDistinctDstRegOp(MI))
        continue;

      const DebugLoc &DL = MI.getDebugLoc();
      Register Dst = MI.getOperand(0).getReg();
      Register LHS = MI.getOperand(2).getReg();
      Register RHS = MI.getOperand(3).getReg();

      if (MI.getOpcode() == ARM::tADDrr && Dst == RHS) {
        const bool LHSKill = MI.getOperand(2).isKill();
        const bool RHSKill = MI.getOperand(3).isKill();
        MI.getOperand(2).setReg(RHS);
        MI.getOperand(2).setIsKill(RHSKill);
        MI.getOperand(3).setReg(LHS);
        MI.getOperand(3).setIsKill(LHSKill);
        Changed = true;
        continue;
      }

      if (Dst != RHS) {
        BuildMI(MBB, InsertPt, DL, TII->get(ARM::tMOVSr), Dst)
            .addReg(LHS, getKillRegState(MI.getOperand(2).isKill()))
            ->addRegisterDead(ARM::CPSR, TRI);
        MI.getOperand(2).setReg(Dst);
        MI.getOperand(2).setIsKill(false);
        Changed = true;
        continue;
      }

      assert(MI.getOpcode() == ARM::tSUBrr &&
             "only tc32 sub with dst==rhs needs the carry rewrite");

      // Rewrite `dst = lhs - dst` as `dst = lhs + (~dst) + 1` without
      // introducing an extra scratch register. We materialize the carry-in
      // with `cmp dst, dst`, which reliably sets C=1.
      BuildMI(MBB, InsertPt, DL, TII->get(ARM::tMVN), Dst)
          .addReg(ARM::CPSR, RegState::Define | RegState::Dead)
          .addReg(RHS, getKillRegState(MI.getOperand(3).isKill()))
          .add(predOps(ARMCC::AL));
      BuildMI(MBB, InsertPt, DL, TII->get(ARM::tCMPr))
          .addReg(Dst)
          .addReg(Dst)
          .add(predOps(ARMCC::AL));
      BuildMI(MBB, InsertPt, DL, TII->get(ARM::tADC), Dst)
          .addReg(ARM::CPSR, RegState::Define |
                              getDeadRegState(MI.getOperand(1).isDead()))
          .addReg(Dst)
          .addReg(LHS, getKillRegState(MI.getOperand(2).isKill()))
          .add(predOps(ARMCC::AL));
      MI.eraseFromParent();
      Changed = true;
      continue;
    }
  }

  return Changed;
}

FunctionPass *llvm::createTC32DistinctDstRegFixupPass() {
  return new TC32DistinctDstRegFixup();
}
