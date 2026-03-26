#include "TC32InstPrinter.h"
#include "TC32MCTargetDesc.h"
#include "TC32MCAsmInfo.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "TC32GenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "TC32GenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "TC32GenSubtargetInfo.inc"

static MCInstrInfo *createTC32MCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitTC32MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createTC32MCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  InitTC32MCRegisterInfo(X, TC32::R15);
  return X;
}

static MCSubtargetInfo *createTC32MCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  StringRef CPUName = CPU.empty() ? "generic" : CPU;
  return createTC32MCSubtargetInfoImpl(TT, CPUName, CPUName, FS);
}

static MCAsmInfo *createTC32MCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &TO) {
  auto *MAI = new TC32MCAsmInfo(TT);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(
      nullptr, MRI.getDwarfRegNum(TC32::R13, true), 0);
  MAI->addInitialFrameState(Inst);
  return MAI;
}

MCInstPrinter *llvm::createTC32MCInstPrinter(const Triple &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new TC32InstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32TargetMC() {
  Target &T = getTheTC32Target();

  RegisterMCAsmInfoFn X(T, createTC32MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createTC32MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createTC32MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createTC32MCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createTC32MCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createTC32MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createTC32AsmBackend);
}
