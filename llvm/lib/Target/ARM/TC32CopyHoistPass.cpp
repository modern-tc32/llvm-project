//===-- TC32CopyHoistPass.cpp - Hoist copies out of flag windows ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// On TC32 a low-register copy writes N/Z, so a COPY sitting between a
// flag-defining instruction and the branch that consumes the flags makes
// copyPhysReg detour through a high register (two tmov instead of one).
// This pass moves a run of COPYs placed right after a CPSR-defining
// instruction above that instruction when doing so cannot change the
// values seen by it: the instruction must neither read nor write a copy's
// destination, and must not write a copy's source. Only the instruction
// itself lies between the old and new positions, so the copies still see
// the same source values.
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMSubtarget.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-copy-hoist"

namespace {

class TC32CopyHoist : public MachineFunctionPass {
public:
  static char ID;

  TC32CopyHoist() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 copy hoist out of flag windows";
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

char TC32CopyHoist::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(TC32CopyHoist, "tc32-copy-hoist",
                "TC32 copy hoist out of flag windows", false, false)

static bool definesCPSR(const MachineInstr &MI) {
  for (const MachineOperand &MO : MI.operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == ARM::CPSR)
      return true;
  return false;
}

// True if the copy run can be moved above D without changing what D or the
// copies observe.
static bool canHoistAbove(const MachineInstr &D,
                          ArrayRef<MachineInstr *> Run,
                          const TargetRegisterInfo &TRI) {
  for (const MachineInstr *C : Run) {
    Register Dst = C->getOperand(0).getReg();
    Register Src = C->getOperand(1).getReg();
    for (const MachineOperand &MO : D.operands()) {
      if (!MO.isReg() || MO.getReg() == ARM::CPSR)
        continue;
      Register R = MO.getReg();
      if (MO.isDef()) {
        if (TRI.regsOverlap(R, Dst) || TRI.regsOverlap(R, Src))
          return false;
      } else if (TRI.regsOverlap(R, Dst)) {
        return false;
      }
    }
  }
  return true;
}

bool TC32CopyHoist::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (auto I = MBB.begin(); I != MBB.end(); ++I) {
      if (!definesCPSR(*I))
        continue;

      // Collect the run of plain copies that directly follows D.
      SmallVector<MachineInstr *, 4> Run;
      auto RunEnd = std::next(I);
      while (RunEnd != MBB.end() && RunEnd->isCopy() &&
             RunEnd->getNumOperands() == 2 && !RunEnd->isDebugInstr()) {
        Run.push_back(&*RunEnd);
        ++RunEnd;
      }
      if (Run.empty() || !canHoistAbove(*I, Run, TRI))
        continue;

      LLVM_DEBUG(dbgs() << "Hoisting " << Run.size() << " copies above "
                        << *I);
      // Moves [next(I), RunEnd) in front of I. I stays valid and the loop
      // resumes at RunEnd.
      MBB.splice(I, &MBB, std::next(I), RunEnd);
      Changed = true;
    }
  }

  return Changed;
}

FunctionPass *llvm::createTC32CopyHoistPass() { return new TC32CopyHoist(); }
