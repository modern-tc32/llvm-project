#ifndef LLVM_LIB_TARGET_TC32_TC32TARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_TC32_TC32TARGETTRANSFORMINFO_H

#include "TC32Subtarget.h"
#include "TC32TargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"

namespace llvm {

class TC32TTIImpl final : public BasicTTIImplBase<TC32TTIImpl> {
  using BaseT = BasicTTIImplBase<TC32TTIImpl>;

  const TC32Subtarget *ST;
  const TC32TargetLowering *TLI;

  const TC32Subtarget *getST() const { return ST; }
  const TC32TargetLowering *getTLI() const { return TLI; }

  friend BaseT;

public:
  explicit TC32TTIImpl(const TC32TargetMachine *TM, const Function &F)
      : BaseT(TM, F.getDataLayout()),
        ST(static_cast<const TC32Subtarget *>(TM->getSubtargetImpl(F))),
        TLI(ST->getTargetLowering()) {}

  unsigned getInlineCallPenalty(const Function *F, const CallBase &Call,
                                unsigned DefaultCallPenalty) const override;
};

} // namespace llvm

#endif
