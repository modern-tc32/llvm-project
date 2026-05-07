//===-- TC32ImmediateExpandPass.cpp - TC32 immediate ALU expand -----------===//
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
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-immediate-expand"

namespace {

class TC32ImmediateExpand : public MachineFunctionPass {
public:
  static char ID;

  TC32ImmediateExpand() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 immediate ALU expand";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

char TC32ImmediateExpand::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(TC32ImmediateExpand, DEBUG_TYPE, "TC32 immediate ALU expand",
                false, false)

static unsigned getExpandedOpcode(unsigned Opcode) {
  switch (Opcode) {
  case ARM::tADDi3:
  case ARM::tADDi8:
    return ARM::tADDrr;
  case ARM::tSUBi3:
  case ARM::tSUBi8:
    return ARM::tSUBrr;
  default:
    return 0;
  }
}

bool TC32ImmediateExpand::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const ARMBaseInstrInfo *TII = ST.getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (auto I = MBB.begin(); I != MBB.end();) {
      MachineInstr &MI = *I++;
      unsigned ExpandedOpcode = getExpandedOpcode(MI.getOpcode());
      if (!ExpandedOpcode)
        continue;

      Register Tmp = MRI.createVirtualRegister(&ARM::tGPRRegClass);
      const DebugLoc &DL = MI.getDebugLoc();
      const int64_t Imm = MI.getOperand(3).getImm();
      const MachineOperand &Pred = MI.getOperand(4);
      const MachineOperand &PredReg = MI.getOperand(5);

      // TC32 immediate add/sub forms do not carry across all 32 bits on
      // hardware. Keep the assembler forms available, but do not emit them for
      // compiler-generated i32 arithmetic.
      BuildMI(MBB, MI, DL, TII->get(ARM::tMOVi8), Tmp)
          .addReg(ARM::CPSR, RegState::Define | RegState::Dead)
          .addImm(Imm)
          .add(Pred)
          .add(PredReg);

      BuildMI(MBB, MI, DL, TII->get(ExpandedOpcode), MI.getOperand(0).getReg())
          .addReg(ARM::CPSR,
                  RegState::Define | getDeadRegState(MI.getOperand(1).isDead()))
          .add(MI.getOperand(2))
          .addReg(Tmp, RegState::Kill)
          .add(Pred)
          .add(PredReg);

      MI.eraseFromParent();
      Changed = true;
    }
  }

  return Changed;
}

FunctionPass *llvm::createTC32ImmediateExpandPass() {
  return new TC32ImmediateExpand();
}
