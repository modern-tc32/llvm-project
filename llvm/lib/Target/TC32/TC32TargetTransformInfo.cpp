#include "TC32TargetTransformInfo.h"

using namespace llvm;

unsigned TC32TTIImpl::getInlineCallPenalty(const Function *F,
                                           const CallBase &Call,
                                           unsigned DefaultCallPenalty) const {
  (void)F;
  (void)Call;

  // TC32 has a very small instruction encoding space and relies heavily on
  // PC-relative literals and short branches. Favor keeping helper functions
  // outlined unless there is strong optimizer evidence to inline them.
  return DefaultCallPenalty * 4;
}
