#ifndef LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32FIXUPKINDS_H
#define LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace TC32 {

enum Fixups {
  fixup_tc32_call = FirstTargetFixupKind,
  fixup_tc32_branch8,
  fixup_tc32_jump11,
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // namespace TC32
} // namespace llvm

#endif
