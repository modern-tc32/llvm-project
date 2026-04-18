//===-- TC32IRFixupPass.cpp - Fix IR for TC32 target ----------------------===//
//
// Replaces ARM-specific intrinsics and inline assembly that TC32 doesn't
// support with compatible alternatives. This eliminates the need for the
// external fix_ir.py script.
//
// Transformations:
//   - llvm.arm.ldrex -> plain load
//   - llvm.arm.strex -> store + i32 0 (always succeeds)
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

class TC32IRFixupPass : public FunctionPass {
public:
  static char ID;
  TC32IRFixupPass() : FunctionPass(ID) {}

  static constexpr uint32_t TC32RegIrqEnAddr = 0x800643;

  bool runOnFunction(Function &F) override {
    if (!F.getParent()->getTargetTriple().isTC32())
      return false;

    bool Changed = false;
    SmallVector<Instruction *, 16> ToErase;

    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CI = dyn_cast<CallInst>(&I);
        if (!CI)
          continue;

        if (auto *II = dyn_cast<IntrinsicInst>(CI)) {
          Changed |= handleIntrinsic(II, ToErase);
        } else if (CI->isInlineAsm()) {
          Changed |= handleInlineAsm(CI, ToErase);
        }
      }
    }

    for (auto *I : ToErase)
      I->eraseFromParent();

    return Changed;
  }

private:
  bool handleIntrinsic(IntrinsicInst *II,
                       SmallVectorImpl<Instruction *> &ToErase) {
    const DataLayout &DL = II->getModule()->getDataLayout();

    switch (II->getIntrinsicID()) {
    case Intrinsic::arm_ldrex: {
      // ldrex(ptr) -> load(ptr)
      IRBuilder<> Builder(II);
      Value *Ptr = II->getArgOperand(0);
      LoadInst *Load =
          Builder.CreateAlignedLoad(II->getType(), Ptr, DL.getABITypeAlign(II->getType()));
      II->replaceAllUsesWith(Load);
      ToErase.push_back(II);
      return true;
    }
    case Intrinsic::arm_strex: {
      // strex(value, ptr) -> store(value, ptr); i32 0
      // TC32 has no exclusive store; model this as an unconditional store that
      // always succeeds.
      IRBuilder<> Builder(II);
      Value *Val = II->getArgOperand(0);
      Value *Ptr = II->getArgOperand(1);
      StoreInst *Store =
          Builder.CreateAlignedStore(Val, Ptr, DL.getABITypeAlign(Val->getType()));
      (void)Store;
      II->replaceAllUsesWith(ConstantInt::get(II->getType(), 0));
      ToErase.push_back(II);
      return true;
    }
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

} // end anonymous namespace

char TC32IRFixupPass::ID = 0;

INITIALIZE_PASS(TC32IRFixupPass, DEBUG_TYPE,
                "TC32 IR fixup pass", false, false)

FunctionPass *llvm::createTC32IRFixupPass() {
  return new TC32IRFixupPass();
}
