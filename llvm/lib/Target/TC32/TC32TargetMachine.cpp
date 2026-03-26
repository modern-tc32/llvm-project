#include "TC32TargetMachine.h"
#include "TC32.h"
#include "TC32Subtarget.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/PassRegistry.h"
#include <optional>

using namespace llvm;

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
};

} // namespace

TargetPassConfig *TC32TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new TC32PassConfig(*this, PM);
}

const TargetSubtargetInfo *TC32TargetMachine::getSubtargetImpl(const Function &F) const {
  (void)F;
  return Subtarget.get();
}
