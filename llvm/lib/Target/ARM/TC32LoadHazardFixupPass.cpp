//===-- TC32LoadHazardFixupPass.cpp - TC32 load-use hazard fixup ----------===//
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
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-load-hazard-fixup"

namespace {

class TC32LoadHazardFixup : public MachineFunctionPass {
public:
  static char ID;

  TC32LoadHazardFixup() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 load-use hazard fixup";
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

char TC32LoadHazardFixup::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(TC32LoadHazardFixup, DEBUG_TYPE, "TC32 load-use hazard fixup",
                false, false)

static void insertTC32Nops(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator InsertBefore,
                           const DebugLoc &DL, const ARMBaseInstrInfo *TII,
                           unsigned Count) {
  for (unsigned I = 0; I < Count; ++I)
    BuildMI(MBB, InsertBefore, DL, TII->get(ARM::tTC32NOP));
}

static bool isTC32MultiLoad(unsigned Opcode) {
  return Opcode == ARM::tLDMIA || Opcode == ARM::tLDMIA_UPD;
}

static bool isTC32SingleLoad(const MachineInstr &MI) {
  return MI.mayLoad() && !MI.mayStore() && MI.getOpcode() != ARM::tPOP &&
         !isTC32MultiLoad(MI.getOpcode());
}

static bool readsSingleLoadDef(const MachineInstr &MI, const MachineInstr &Use,
                               const TargetRegisterInfo *TRI) {
  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isReg() || !MO.isDef() || MO.getReg() == ARM::NoRegister)
      continue;
    if (Use.readsRegister(MO.getReg(), TRI))
      return true;
  }

  return false;
}

static bool modifiesSingleLoadDef(const MachineInstr &MI,
                                  const MachineInstr &MaybeDef,
                                  const TargetRegisterInfo *TRI) {
  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isReg() || !MO.isDef() || MO.getReg() == ARM::NoRegister)
      continue;
    if (MaybeDef.modifiesRegister(MO.getReg(), TRI))
      return true;
  }

  return false;
}

bool TC32LoadHazardFixup::runOnMachineFunction(MachineFunction &MF) {
  if (!MF.getTarget().getTargetTriple().isTC32())
    return false;

  const ARMSubtarget &ST = MF.getSubtarget<ARMSubtarget>();
  const ARMBaseInstrInfo *TII = ST.getInstrInfo();
  const TargetRegisterInfo *TRI = ST.getRegisterInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (auto I = MBB.begin(); I != MBB.end(); ++I) {
      MachineInstr &MI = *I;
      const bool IsMultiLoad = isTC32MultiLoad(MI.getOpcode());

      if (!MI.mayLoad() && !IsMultiLoad && MI.getOpcode() != ARM::tPOP)
        continue;

      if (isTC32SingleLoad(MI)) {
        auto Consumer = std::next(I);
        unsigned Gap = 0;

        while (Consumer != MBB.end() && Gap < 2) {
          if (Consumer->isDebugInstr()) {
            ++Consumer;
            continue;
          }
          if (readsSingleLoadDef(MI, *Consumer, TRI)) {
            insertTC32Nops(MBB, Consumer, MI.getDebugLoc(), TII, 2 - Gap);
            Changed = true;
            break;
          }
          if (modifiesSingleLoadDef(MI, *Consumer, TRI))
            break;
          ++Gap;
          ++Consumer;
        }

        continue;
      }

      auto Consumer = std::next(I);
      unsigned Gap = 0;

      while (Consumer != MBB.end() && Gap < 2) {
        if (Consumer->isDebugInstr()) {
          ++Consumer;
          continue;
        }
        if (readsSingleLoadDef(MI, *Consumer, TRI)) {
          insertTC32Nops(MBB, Consumer, MI.getDebugLoc(), TII, 2 - Gap);
          Changed = true;
          break;
        }
        if (modifiesSingleLoadDef(MI, *Consumer, TRI))
          break;
        ++Gap;
        ++Consumer;
      }
    }
  }

  return Changed;
}

FunctionPass *llvm::createTC32LoadHazardFixupPass() {
  return new TC32LoadHazardFixup();
}
