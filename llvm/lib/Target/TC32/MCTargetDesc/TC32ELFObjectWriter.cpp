#include "TC32FixupKinds.h"
#include "TC32MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class TC32ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  TC32ELFObjectWriter()
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, ELF::ELFOSABI_TC32,
                                ELF::EM_TC32,
                                /*HasRelocationAddend=*/false) {}

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    (void)Target;
    (void)IsPCRel;
    switch (Fixup.getKind()) {
    case FK_Data_4:
      return ELF::R_ARM_ABS32;
    case TC32::fixup_tc32_call:
      return ELF::R_ARM_THM_CALL;
    default:
      llvm_unreachable("TC32 relocations are not implemented for this fixup");
    }
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter> llvm::createTC32ELFObjectWriter() {
  return std::make_unique<TC32ELFObjectWriter>();
}
