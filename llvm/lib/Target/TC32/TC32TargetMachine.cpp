#include "TC32TargetMachine.h"
#include "TC32TargetTransformInfo.h"
#include "TC32.h"
#include "TC32InstrInfo.h"
#include "TC32Subtarget.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/MathExtras.h"
#include <optional>

using namespace llvm;

#define GET_REGINFO_ENUM
#include "TC32GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "TC32GenInstrInfo.inc"

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32Target() {
  RegisterTargetMachine<TC32TargetMachine> X(getTheTC32Target());
}

static Reloc::Model
getEffectiveRelocModel(std::optional<Reloc::Model> RM, bool JIT) {
  if (!RM || JIT)
    return Reloc::Static;
  return *RM;
}

TC32TargetMachine::TC32TargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(
          T,
          "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32",
          TT, CPU, FS, Options, getEffectiveRelocModel(RM, JIT),
          getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(std::make_unique<TC32Subtarget>(TT, std::string(CPU),
                                                std::string(FS), *this)) {
  initAsmInfo();
}

namespace {

class TC32BranchRelaxationPass : public MachineFunctionPass {
public:
  static char ID;

  TC32BranchRelaxationPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 Branch Relaxation";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const auto *TII =
        static_cast<const TC32InstrInfo *>(MF.getSubtarget().getInstrInfo());
    const auto *TRI = MF.getSubtarget().getRegisterInfo();
    MachineRegisterInfo &MRI = MF.getRegInfo();
    bool Changed = false;

    auto isCondBranch = [](unsigned Opc) {
      switch (Opc) {
      case TC32::TJEQ:
      case TC32::TJNE:
      case TC32::TJCS:
      case TC32::TJCC:
      case TC32::TJGE:
      case TC32::TJLT:
      case TC32::TJHI:
      case TC32::TJLS:
      case TC32::TJGT:
      case TC32::TJLE:
        return true;
      default:
        return false;
      }
    };

    auto invertCondBranch = [](unsigned Opc) {
      switch (Opc) {
      case TC32::TJEQ: return TC32::TJNE;
      case TC32::TJNE: return TC32::TJEQ;
      case TC32::TJCS: return TC32::TJCC;
      case TC32::TJCC: return TC32::TJCS;
      case TC32::TJGE: return TC32::TJLT;
      case TC32::TJLT: return TC32::TJGE;
      case TC32::TJHI: return TC32::TJLS;
      case TC32::TJLS: return TC32::TJHI;
      case TC32::TJGT: return TC32::TJLE;
      case TC32::TJLE: return TC32::TJGT;
      default: llvm_unreachable("not a TC32 conditional branch");
      }
    };

    auto getInstSize = [&](const MachineInstr &MI, uint64_t Offset) {
      if (MI.getOpcode() == TC32::TLOADaddr) {
        (void)Offset;
        // TLOADaddr emits a 2-byte PC-relative load plus a 4-byte literal in a
        // nearby pool after the next barrier. Like ARM constant islands, the
        // real distance seen by a short conditional branch must budget not only
        // the instruction itself but also later literal-pool growth and
        // alignment between the branch and its destination. TC32 does not have
        // ARM's full island machinery yet, so keep a deliberately conservative
        // local estimate here to force CFG branch relaxation before MC layout.
        return 20u;
      }
      return TII->getInstSizeInBytes(MI);
    };

    auto findAvailableLowRegAtEnd = [&](MachineBasicBlock &MBB) -> Register {
      LivePhysRegs LiveRegs(*TRI);
      LiveRegs.addLiveOuts(MBB);
      for (MCPhysReg Reg : {TC32::R0, TC32::R1, TC32::R2, TC32::R3,
                            TC32::R4, TC32::R5, TC32::R6, TC32::R7}) {
        if (LiveRegs.available(MRI, Reg))
          return Reg;
      }
      return Register();
    };

    auto computeOffsets = [&](DenseMap<const MachineBasicBlock *, uint64_t> &BlockOffsets,
                              DenseMap<const MachineInstr *, uint64_t> &InstOffsets) {
      BlockOffsets.clear();
      InstOffsets.clear();
      uint64_t Offset = 0;
      for (const MachineBasicBlock &MBB : MF) {
        BlockOffsets[&MBB] = Offset;
        for (const MachineInstr &MI : MBB) {
          InstOffsets[&MI] = Offset;
          Offset += getInstSize(MI, Offset);
        }
      }
    };

    bool LocalChange = false;
    do {
      LocalChange = false;
      DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
      DenseMap<const MachineInstr *, uint64_t> InstOffsets;
      computeOffsets(BlockOffsets, InstOffsets);

      for (auto MBBIt = MF.begin(), MBBEnd = MF.end(); MBBIt != MBBEnd;) {
        MachineBasicBlock &MBB = *MBBIt++;
        MachineInstr *CondBr = nullptr;
        MachineInstr *UncondBr = nullptr;
        MachineInstr *UncondBrAddr = nullptr;

        for (MachineInstr &MI : llvm::reverse(MBB)) {
          if (MI.isDebugInstr())
            continue;
          unsigned Opc = MI.getOpcode();
          if (!CondBr && isCondBranch(Opc)) {
            CondBr = &MI;
            continue;
          }
          if (!UncondBr && Opc == TC32::TJ) {
            UncondBr = &MI;
            continue;
          }
          if (!UncondBr && Opc == TC32::TJEXr) {
            auto It = MI.getIterator();
            if (It != MBB.begin()) {
              auto Prev = std::prev(It);
              while (Prev->isDebugInstr()) {
                if (Prev == MBB.begin())
                  break;
                Prev = std::prev(Prev);
              }
              if (!Prev->isDebugInstr() && Prev->getOpcode() == TC32::TLOADaddr &&
                  Prev->getOperand(0).isReg() && MI.getOperand(0).isReg() &&
                  Prev->getOperand(0).getReg() == MI.getOperand(0).getReg()) {
                UncondBr = &MI;
                UncondBrAddr = &*Prev;
                continue;
              }
            }
          }
          if (UncondBrAddr == &MI)
            continue;
          if (UncondBr && !CondBr)
            continue;
          if (!MI.isTerminator())
            break;
        }

        if (UncondBr && UncondBr->getParent() == &MBB &&
            UncondBr->getOpcode() == TC32::TJ) {
          auto *JumpTarget = UncondBr->getOperand(0).getMBB();
          uint64_t JumpOff = InstOffsets.lookup(UncondBr);
          uint64_t JumpTargetOff = BlockOffsets.lookup(JumpTarget);
          int64_t JumpDelta =
              static_cast<int64_t>(JumpTargetOff) - static_cast<int64_t>(JumpOff);
          int64_t JumpImm = (JumpDelta - 4) >> 1;
          if (!isInt<11>(JumpImm)) {
            Register JumpReg = findAvailableLowRegAtEnd(MBB);
            if (!JumpReg)
              report_fatal_error("TC32 branch relaxation could not find dead low register");
            auto InsertPt = UncondBr->getIterator();
            BuildMI(MBB, InsertPt, UncondBr->getDebugLoc(),
                    TII->get(TC32::TLOADaddr), JumpReg)
                .addMBB(JumpTarget);
            BuildMI(MBB, InsertPt, UncondBr->getDebugLoc(), TII->get(TC32::TJEXr))
                .addReg(JumpReg);
            UncondBr->eraseFromParent();
            LocalChange = true;
            Changed = true;
            continue;
          }
        }

        if (!CondBr)
          continue;

        bool NeedsSplit = false;
        for (MachineBasicBlock::iterator I = std::next(CondBr->getIterator()),
                                         E = MBB.end();
             I != E; ++I) {
          if (I->isDebugInstr())
            continue;
          if (!I->isTerminator()) {
            NeedsSplit = true;
            break;
          }
        }

        if (NeedsSplit) {
          MachineBasicBlock *TrueMBB = CondBr->getOperand(0).getMBB();
          MachineBasicBlock *SplitMBB = MBB.splitAt(*CondBr);
          SplitMBB->removeSuccessor(TrueMBB);
          MBB.addSuccessor(TrueMBB);
          LocalChange = true;
          Changed = true;
          continue;
        }

        auto *TrueMBB = CondBr->getOperand(0).getMBB();
        uint64_t BrOffset = InstOffsets.lookup(CondBr);
        int64_t BrDelta =
            static_cast<int64_t>(BlockOffsets.lookup(TrueMBB)) - static_cast<int64_t>(BrOffset);
        int64_t BrImm = (BrDelta - 4) >> 1;

        MachineBasicBlock *FalseMBB = nullptr;
        if (UncondBr && UncondBr->getParent() == &MBB) {
          if (UncondBr->getOpcode() == TC32::TJ)
            FalseMBB = UncondBr->getOperand(0).getMBB();
          else if (UncondBrAddr)
            FalseMBB = UncondBrAddr->getOperand(1).getMBB();
        }
        else {
          auto NextIt = std::next(MBB.getIterator());
          if (NextIt != MF.end())
            FalseMBB = &*NextIt;
        }
        if (!FalseMBB)
          continue;

        if (UncondBr && UncondBr->getParent() == &MBB &&
            UncondBr->getOpcode() == TC32::TJ) {
          int64_t FalseDelta =
              static_cast<int64_t>(BlockOffsets.lookup(FalseMBB)) -
              static_cast<int64_t>(BrOffset);
          int64_t FalseImm = (FalseDelta - 4) >> 1;
          if (std::llabs(FalseImm) < std::llabs(BrImm)) {
            CondBr->setDesc(TII->get(invertCondBranch(CondBr->getOpcode())));
            CondBr->getOperand(0).setMBB(FalseMBB);
            UncondBr->getOperand(0).setMBB(TrueMBB);
            LocalChange = true;
            Changed = true;
            continue;
          }
        }

        if (isInt<8>(BrImm))
          continue;

        if (UncondBr && UncondBr->getParent() == &MBB &&
            UncondBr->getOpcode() == TC32::TJ) {
          int64_t FalseDelta =
              static_cast<int64_t>(BlockOffsets.lookup(FalseMBB)) -
              static_cast<int64_t>(BrOffset);
          int64_t FalseImm = (FalseDelta - 4) >> 1;
          if (isInt<8>(FalseImm)) {
            CondBr->setDesc(TII->get(invertCondBranch(CondBr->getOpcode())));
            CondBr->getOperand(0).setMBB(FalseMBB);
            UncondBr->getOperand(0).setMBB(TrueMBB);
            LocalChange = true;
            Changed = true;
            continue;
          }
        }

        MachineBasicBlock *ShimMBB =
            MF.CreateMachineBasicBlock(MBB.getBasicBlock());
        MF.insert(std::next(MBB.getIterator()), ShimMBB);

        unsigned OldOpc = CondBr->getOpcode();
        CondBr->setDesc(TII->get(invertCondBranch(OldOpc)));
        CondBr->getOperand(0).setMBB(ShimMBB);

        if (UncondBr && UncondBr->getParent() == &MBB) {
          if (UncondBr->getOpcode() == TC32::TJ)
            UncondBr->getOperand(0).setMBB(TrueMBB);
          else
            UncondBrAddr->getOperand(1).setMBB(TrueMBB);
        } else {
          BuildMI(MBB, std::next(CondBr->getIterator()), CondBr->getDebugLoc(),
                  TII->get(TC32::TJ))
              .addMBB(TrueMBB);
        }

        BuildMI(*ShimMBB, ShimMBB->end(), CondBr->getDebugLoc(),
                TII->get(TC32::TJ))
            .addMBB(FalseMBB);

        SmallVector<MachineBasicBlock *, 4> OldSuccs(MBB.successors());
        for (MachineBasicBlock *Succ : OldSuccs)
          MBB.removeSuccessor(Succ);
        MBB.addSuccessor(ShimMBB);
        MBB.addSuccessor(TrueMBB);
        ShimMBB->addSuccessor(FalseMBB);

        LocalChange = true;
        Changed = true;
      }
    } while (LocalChange);

    return Changed;
  }
};

char TC32BranchRelaxationPass::ID = 0;

class TC32CompareBranchRepairPass : public MachineFunctionPass {
public:
  static char ID;

  TC32CompareBranchRepairPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 Compare/Branch Repair";
  }

  static bool isCondBranch(const MachineInstr &MI) {
    switch (MI.getOpcode()) {
    case TC32::TJEQ:
    case TC32::TJNE:
    case TC32::TJCS:
    case TC32::TJCC:
    case TC32::TJGE:
    case TC32::TJLT:
    case TC32::TJHI:
    case TC32::TJLS:
    case TC32::TJGT:
    case TC32::TJLE:
      return true;
    default:
      return false;
    }
  }

  static bool isCompare(const MachineInstr &MI) {
    return MI.getOpcode() == TC32::TCMPri0 || MI.getOpcode() == TC32::TCMPrr;
  }

  static bool isLateCopy(const MachineInstr &MI) {
    return MI.getOpcode() == TC32::TMOVrr || MI.getOpcode() == TC32::TMOVNrr;
  }

  static bool definesCmpInput(const MachineInstr &Copy, const MachineInstr &Cmp) {
    if (Copy.getNumOperands() < 1 || !Copy.getOperand(0).isReg())
      return true;
    Register DefReg = Copy.getOperand(0).getReg();
    for (const MachineOperand &MO : Cmp.operands()) {
      if (!MO.isReg() || !MO.isUse())
        continue;
      if (MO.getReg() == DefReg)
        return true;
    }
    return false;
  }

  static bool usesRegBeforeDef(const MachineBasicBlock &MBB, Register Reg,
                               const TargetRegisterInfo *TRI) {
    for (const MachineInstr &MI : MBB) {
      if (MI.isDebugInstr())
        continue;
      for (const MachineOperand &MO : MI.operands()) {
        if (!MO.isReg())
          continue;
        Register MOReg = MO.getReg();
        if (!MOReg)
          continue;
        if (!TRI->regsOverlap(MOReg, Reg))
          continue;
        if (MO.isUse())
          return true;
        if (MO.isDef())
          return false;
      }
    }
    return false;
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    bool Changed = false;
    const auto *TRI = MF.getSubtarget().getRegisterInfo();

    for (MachineBasicBlock &MBB : MF) {
      for (auto BI = MBB.begin(), BE = MBB.end(); BI != BE; ++BI) {
        if (!isCondBranch(*BI))
          continue;

        SmallVector<MachineBasicBlock::iterator, 4> LateCopies;
        auto Scan = BI;

        while (Scan != MBB.begin()) {
          --Scan;
          if (Scan->isDebugInstr())
            continue;
          if (isLateCopy(*Scan)) {
            LateCopies.push_back(Scan);
            continue;
          }
          if (!isCompare(*Scan)) {
            LateCopies.clear();
          }
          break;
        }

        if (LateCopies.empty() || Scan == BI || !isCompare(*Scan))
          continue;

        bool Safe = true;
        for (auto It : LateCopies) {
          if (definesCmpInput(*It, *Scan)) {
            Safe = false;
            break;
          }
        }
        if (!Safe)
          continue;

        auto InsertPt = Scan;
        for (auto It = LateCopies.rbegin(); It != LateCopies.rend(); ++It) {
          MBB.splice(InsertPt, &MBB, *It);
        }
        Changed = true;
        continue;
      }

      for (auto BI = MBB.begin(), BE = MBB.end(); BI != BE; ++BI) {
        if (!isCondBranch(*BI))
          continue;

        auto CopyIt = BI;
        while (CopyIt != MBB.begin()) {
          --CopyIt;
          if (!CopyIt->isDebugInstr())
            break;
        }
        if (CopyIt == BI || !isLateCopy(*CopyIt))
          continue;

        auto CmpIt = CopyIt;
        while (CmpIt != MBB.begin()) {
          --CmpIt;
          if (!CmpIt->isDebugInstr())
            break;
        }
        if (CmpIt == CopyIt || !isCompare(*CmpIt) || !definesCmpInput(*CopyIt, *CmpIt))
          continue;

        auto FalseBrIt = std::next(BI);
        while (FalseBrIt != BE && FalseBrIt->isDebugInstr())
          ++FalseBrIt;
        if (FalseBrIt == BE || FalseBrIt->getOpcode() != TC32::TJ)
          continue;

        if (CopyIt->getNumOperands() < 1 || !CopyIt->getOperand(0).isReg())
          continue;
        Register DefReg = CopyIt->getOperand(0).getReg();
        MachineBasicBlock *TrueMBB = BI->getOperand(0).getMBB();
        if (!TrueMBB || usesRegBeforeDef(*TrueMBB, DefReg, TRI))
          continue;

        MBB.splice(FalseBrIt, &MBB, CopyIt);
        Changed = true;
      }
    }

    return Changed;
  }
};

class TC32CallMaskPass : public MachineFunctionPass {
public:
  static char ID;

  TC32CallMaskPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "TC32 Call Preserved Mask";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const auto *TRI = MF.getSubtarget().getRegisterInfo();
    const uint32_t *Mask = TRI->getCallPreservedMask(MF, CallingConv::C);
    if (!Mask)
      return false;

    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineInstr &MI : MBB) {
        if (!MI.isCall())
          continue;

        bool HasRegMask = false;
        for (const MachineOperand &MO : MI.operands()) {
          if (MO.isRegMask()) {
            HasRegMask = true;
            break;
          }
        }
        if (HasRegMask)
          continue;

        MI.addOperand(MF, MachineOperand::CreateRegMask(Mask));
        Changed = true;
      }
    }
    return Changed;
  }
};

char TC32CallMaskPass::ID = 0;
char TC32CompareBranchRepairPass::ID = 0;

class TC32PassConfig : public TargetPassConfig {
public:
  TC32PassConfig(TC32TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  TC32TargetMachine &getTC32TargetMachine() const {
    return getTM<TC32TargetMachine>();
  }

  void addIRPasses() override {
    addPass(createAtomicExpandLegacyPass());
    TargetPassConfig::addIRPasses();
  }

  FunctionPass *createTargetRegisterAllocator(bool Optimized) override {
    if (Optimized)
      return createGreedyRegisterAllocator();
    return createFastRegisterAllocator();
  }

  bool addInstSelector() override {
    addPass(createTC32ISelDag(getTC32TargetMachine(), getOptLevel()));
    return false;
  }

  void addPreRegAlloc() override { addPass(new TC32CallMaskPass()); }

  void addPreEmitPass() override {
    addPass(new TC32CompareBranchRepairPass());
    addPass(new TC32BranchRelaxationPass());
  }
};

} // namespace

TargetPassConfig *TC32TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new TC32PassConfig(*this, PM);
}

const TargetSubtargetInfo *TC32TargetMachine::getSubtargetImpl(const Function &F) const {
  (void)F;
  return Subtarget.get();
}

TargetTransformInfo
TC32TargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(std::make_unique<TC32TTIImpl>(this, F));
}
