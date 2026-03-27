#include "TC32TargetMachine.h"
#include "TC32.h"
#include "TC32InstrInfo.h"
#include "TC32Subtarget.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
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
    bool Changed = false;

    auto isCondBranch = [](unsigned Opc) {
      switch (Opc) {
      case TC32::TJEQ:
      case TC32::TJNE:
      case TC32::TJGE:
      case TC32::TJLT:
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
      case TC32::TJGE: return TC32::TJLT;
      case TC32::TJLT: return TC32::TJGE;
      case TC32::TJGT: return TC32::TJLE;
      case TC32::TJLE: return TC32::TJGT;
      default: llvm_unreachable("not a TC32 conditional branch");
      }
    };

    auto getInstSize = [&](const MachineInstr &MI, uint64_t Offset) {
      if (MI.getOpcode() == TC32::TLOADaddr) {
        uint64_t AfterLoadAndJump = Offset + 4;
        uint64_t AlignPad = (4 - (AfterLoadAndJump & 0x3)) & 0x3;
        return static_cast<unsigned>(4 + AlignPad + 4);
      }
      return TII->getInstSizeInBytes(MI);
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

      for (MachineBasicBlock &MBB : MF) {
        MachineInstr *CondBr = nullptr;
        MachineInstr *UncondBr = nullptr;

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
          if (!MI.isTerminator())
            break;
        }

        if (!CondBr)
          continue;

        auto *TrueMBB = CondBr->getOperand(0).getMBB();
        uint64_t BrOffset = InstOffsets.lookup(CondBr);
        int64_t BrDelta =
            static_cast<int64_t>(BlockOffsets.lookup(TrueMBB)) - static_cast<int64_t>(BrOffset);
        int64_t BrImm = (BrDelta - 4) >> 1;
        if (isInt<8>(BrImm))
          continue;

        MachineBasicBlock *FalseMBB = nullptr;
        if (UncondBr && UncondBr->getParent() == &MBB)
          FalseMBB = UncondBr->getOperand(0).getMBB();
        else {
          auto NextIt = std::next(MBB.getIterator());
          if (NextIt != MF.end())
            FalseMBB = &*NextIt;
        }
        if (!FalseMBB)
          continue;

        uint64_t JumpOffset = BrOffset + getInstSize(*CondBr, BrOffset);
        uint64_t TrueOffset = BlockOffsets.lookup(TrueMBB);
        unsigned AddedSize = TII->get(TC32::TJ).getSize();
        if (!UncondBr)
          AddedSize += TII->get(TC32::TJ).getSize();
        if (TrueOffset > BrOffset)
          TrueOffset += AddedSize;
        int64_t JumpDelta =
            static_cast<int64_t>(TrueOffset) - static_cast<int64_t>(JumpOffset);
        int64_t JumpImm = (JumpDelta - 4) >> 1;
        if (!isInt<11>(JumpImm))
          continue;

        MachineBasicBlock *ShimMBB =
            MF.CreateMachineBasicBlock(MBB.getBasicBlock());
        MF.insert(std::next(MBB.getIterator()), ShimMBB);

        unsigned OldOpc = CondBr->getOpcode();
        CondBr->setDesc(TII->get(invertCondBranch(OldOpc)));
        CondBr->getOperand(0).setMBB(ShimMBB);

        if (UncondBr && UncondBr->getParent() == &MBB) {
          UncondBr->getOperand(0).setMBB(TrueMBB);
        } else {
          BuildMI(MBB, std::next(CondBr->getIterator()), CondBr->getDebugLoc(),
                  TII->get(TC32::TJ))
              .addMBB(TrueMBB);
        }

        BuildMI(*ShimMBB, ShimMBB->end(), CondBr->getDebugLoc(), TII->get(TC32::TJ))
            .addMBB(FalseMBB);

        SmallVector<MachineBasicBlock *, 4> OldSuccs(MBB.successors());
        for (MachineBasicBlock *Succ : OldSuccs)
          MBB.removeSuccessor(Succ);
        MBB.addSuccessor(ShimMBB);
        MBB.addSuccessor(TrueMBB);
        ShimMBB->addSuccessor(FalseMBB);

        LocalChange = true;
        Changed = true;
        break;
      }
    } while (LocalChange);

    return Changed;
  }
};

char TC32BranchRelaxationPass::ID = 0;

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

  bool addInstSelector() override {
    addPass(createTC32ISelDag(getTC32TargetMachine(), getOptLevel()));
    return false;
  }

  void addPreEmitPass() override { addPass(new TC32BranchRelaxationPass()); }
};

} // namespace

TargetPassConfig *TC32TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new TC32PassConfig(*this, PM);
}

const TargetSubtargetInfo *TC32TargetMachine::getSubtargetImpl(const Function &F) const {
  (void)F;
  return Subtarget.get();
}
