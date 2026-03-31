#include "TC32TargetTransformInfo.h"

using namespace llvm;

void TC32TTIImpl::getUnrollingPreferences(
    Loop *L, ScalarEvolution &SE, TargetTransformInfo::UnrollingPreferences &UP,
    OptimizationRemarkEmitter *ORE) const {
  BaseT::getUnrollingPreferences(L, SE, UP, ORE);

  // TC32 is a tiny code-size-sensitive target. Generic loop unrolling often
  // explodes straight-line code before the backend has a chance to keep
  // literal loads and branches compact, which is especially damaging on O2.
  // Keep the default policy conservative unless the source explicitly forces
  // unrolling.
  UP.Threshold = 0;
  UP.PartialThreshold = 0;
  UP.UpperBound = false;
  UP.Partial = false;
  UP.Runtime = false;
  UP.UnrollRemainder = false;
  UP.DefaultUnrollRuntimeCount = 1;
  UP.FullUnrollMaxCount = 1;
  UP.OptSizeThreshold = 0;
  UP.PartialOptSizeThreshold = 0;
}
