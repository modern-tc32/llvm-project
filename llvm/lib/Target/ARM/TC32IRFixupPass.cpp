//===-- TC32IRFixupPass.cpp - Fix IR for TC32 target ----------------------===//
//
// Replaces ARM-specific inline assembly that TC32 doesn't support with
// compatible alternatives, and rejects unsupported atomic operations. This
// eliminates the need for the external fix_ir.py script.
//
// Transformations:
//   - atomic IR / llvm.arm.ldrex / llvm.arm.strex -> explicit error
//   - llvm.arm.hint  -> erased
//   - inline asm "sev" -> erased
//   - inline asm "cpsid i" -> volatile store 0 to reg_irq_en
//   - inline asm "cpsie i" -> volatile store 1 to reg_irq_en
//   - inline asm "mrs ..., PRIMASK" -> "tmrss ..."
//   - inline asm "msr PRIMASK, ..." -> "tmssr ..."
//   - other inline asm "mrs ..." -> zero value / erased
//   - other inline asm "msr ..." -> erased
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsARM.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "tc32-ir-fixup"

namespace {
class TC32IRFixup {
public:
  static constexpr uint32_t TC32RegIrqEnAddr = 0x800643;

  bool rejectUnsupportedInstruction(Instruction &I,
                                    SmallVectorImpl<Instruction *> &ToErase,
                                    StringRef Msg) {
    I.getContext().emitError(&I, Msg);
    if (!I.getType()->isVoidTy())
      I.replaceAllUsesWith(PoisonValue::get(I.getType()));
    ToErase.push_back(&I);
    return true;
  }

  bool rejectAtomicIR(Instruction &I,
                      SmallVectorImpl<Instruction *> &ToErase) {
    if (auto *LI = dyn_cast<LoadInst>(&I)) {
      if (LI->isAtomic())
        return rejectUnsupportedInstruction(
            I, ToErase, "TC32 does not support atomic operations");
      return false;
    }

    if (auto *SI = dyn_cast<StoreInst>(&I)) {
      if (SI->isAtomic())
        return rejectUnsupportedInstruction(
            I, ToErase, "TC32 does not support atomic operations");
      return false;
    }

    if (isa<AtomicRMWInst>(I) || isa<AtomicCmpXchgInst>(I) || isa<FenceInst>(I))
      return rejectUnsupportedInstruction(
          I, ToErase, "TC32 does not support atomic operations");

    return false;
  }

  bool handleIntrinsic(IntrinsicInst *II,
                       SmallVectorImpl<Instruction *> &ToErase) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::arm_ldrex:
    case Intrinsic::arm_strex:
      return rejectUnsupportedInstruction(
          *II, ToErase, "TC32 does not support atomic operations");
    case Intrinsic::arm_hint: {
      // hint (sev, wfe, etc.) -> erase
      ToErase.push_back(II);
      return true;
    }
    default:
      return false;
    }
  }

  bool handleInlineAsm(CallInst *CI,
                       SmallVectorImpl<Instruction *> &ToErase) {
    auto *IA = cast<InlineAsm>(CI->getCalledOperand());
    StringRef AsmStr = IA->getAsmString();
    std::string LowerAsm = AsmStr.lower();

    if (AsmStr.starts_with("sev")) {
      // sev -> erase
      if (!CI->getType()->isVoidTy())
        CI->replaceAllUsesWith(UndefValue::get(CI->getType()));
      ToErase.push_back(CI);
      return true;
    }

    if (StringRef(LowerAsm).starts_with("cpsid ") ||
        StringRef(LowerAsm).starts_with("cpsie ")) {
      IRBuilder<> Builder(CI);
      Value *RegIrqEn = Builder.CreateIntToPtr(
          Builder.getInt32(TC32RegIrqEnAddr), PointerType::getUnqual(CI->getContext()));
      Value *Enabled = Builder.getInt8(StringRef(LowerAsm).starts_with("cpsie ") ? 1 : 0);
      Builder.CreateAlignedStore(Enabled, RegIrqEn, Align(1))->setVolatile(true);
      if (!CI->getType()->isVoidTy())
        CI->replaceAllUsesWith(UndefValue::get(CI->getType()));
      ToErase.push_back(CI);
      return true;
    }

    if (StringRef(LowerAsm).starts_with("mrs ")) {
      if (LowerAsm.find("primask") != std::string::npos) {
        InlineAsm *NewIA = InlineAsm::get(
            IA->getFunctionType(), "tmrss ${0}", IA->getConstraintString(),
            IA->hasSideEffects(), IA->isAlignStack(), IA->getDialect());
        CI->setCalledOperand(NewIA);
        return true;
      }

      // TC32 has no ARM system registers beyond the IRQ state register handled
      // above; model other reads as zero.
      if (!CI->getType()->isVoidTy())
        CI->replaceAllUsesWith(Constant::getNullValue(CI->getType()));
      ToErase.push_back(CI);
      return true;
    }

    if (StringRef(LowerAsm).starts_with("msr ")) {
      if (LowerAsm.find("primask") != std::string::npos) {
        InlineAsm *NewIA = InlineAsm::get(
            IA->getFunctionType(), "tmssr ${0}", IA->getConstraintString(),
            IA->hasSideEffects(), IA->isAlignStack(), IA->getDialect());
        CI->setCalledOperand(NewIA);
        return true;
      }

      // TC32 has no ARM system registers beyond the IRQ state register handled
      // above.
      if (!CI->getType()->isVoidTy())
        CI->replaceAllUsesWith(UndefValue::get(CI->getType()));
      ToErase.push_back(CI);
      return true;
    }

    return false;
  }
};

class TC32IRFixupPass : public FunctionPass {
public:
  static char ID;
  TC32IRFixupPass() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override { return runTC32IRFixup(F); }
};

} // end anonymous namespace

bool llvm::runTC32IRFixup(Function &F) {
  if (!F.getParent()->getTargetTriple().isTC32())
    return false;

  TC32IRFixup Fixup;
  bool Changed = false;
  SmallVector<Instruction *, 16> ToErase;

  for (auto &BB : F) {
    for (auto &I : BB) {
      Changed |= Fixup.rejectAtomicIR(I, ToErase);

      auto *CI = dyn_cast<CallInst>(&I);
      if (!CI)
        continue;

      if (Function *Callee = CI->getCalledFunction()) {
        StringRef Name = Callee->getName();
        if (Name.starts_with("__atomic_") || Name.starts_with("__sync_")) {
          Changed |= Fixup.rejectUnsupportedInstruction(
              *CI, ToErase, "TC32 does not support atomic operations");
          continue;
        }
      }

      if (auto *II = dyn_cast<IntrinsicInst>(CI)) {
        Changed |= Fixup.handleIntrinsic(II, ToErase);
      } else if (CI->isInlineAsm()) {
        Changed |= Fixup.handleInlineAsm(CI, ToErase);
      }
    }
  }

  for (auto *I : ToErase)
    I->eraseFromParent();

  return Changed;
}

char TC32IRFixupPass::ID = 0;

INITIALIZE_PASS(TC32IRFixupPass, DEBUG_TYPE,
                "TC32 IR fixup pass", false, false)

FunctionPass *llvm::createTC32IRFixupPass() {
  return new TC32IRFixupPass();
}
