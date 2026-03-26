#ifndef LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32MCTARGETDESC_H
#define LLVM_LIB_TARGET_TC32_MCTARGETDESC_TC32MCTARGETDESC_H

#include <memory>
#include "llvm/Support/DataTypes.h"

namespace llvm {

class MCAsmInfo;
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCInstPrinter;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;
class Triple;

MCCodeEmitter *createTC32MCCodeEmitter(const MCInstrInfo &MCII,
                                       MCContext &Ctx);
MCAsmBackend *createTC32AsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                   const MCRegisterInfo &MRI,
                                   const MCTargetOptions &Options);
MCInstPrinter *createTC32MCInstPrinter(const Triple &T, unsigned SyntaxVariant,
                                       const MCAsmInfo &MAI,
                                       const MCInstrInfo &MII,
                                       const MCRegisterInfo &MRI);
std::unique_ptr<MCObjectTargetWriter> createTC32ELFObjectWriter();

} // namespace llvm

#define GET_REGINFO_ENUM
#include "TC32GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "TC32GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "TC32GenSubtargetInfo.inc"

#endif
