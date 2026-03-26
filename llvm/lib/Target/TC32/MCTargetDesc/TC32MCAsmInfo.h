#ifndef LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32MCASMINFO_H
#define LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class TC32MCAsmInfo : public MCAsmInfoELF {
public:
  explicit TC32MCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif
