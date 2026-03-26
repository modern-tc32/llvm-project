#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_TC32_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_TC32_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY TC32TargetInfo : public TargetInfo {
  std::string CPU = "generic";

public:
  TC32TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    BigEndian = false;
    TLSSupported = false;
    NoAsmVariants = true;
    IntWidth = IntAlign = 32;
    LongWidth = LongAlign = 32;
    LongLongWidth = LongLongAlign = 64;
    PointerWidth = PointerAlign = 32;
    SuitableAlign = 32;
    FloatWidth = FloatAlign = 32;
    DoubleWidth = DoubleAlign = 32;
    LongDoubleWidth = LongDoubleAlign = 32;
    LongDoubleFormat = &llvm::APFloat::IEEEsingle();
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    WCharType = SignedInt;
    WIntType = UnsignedInt;
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 32;
    resetDataLayout("e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-"
                    "f32:32:32-f64:32:32-n32-S32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override {
    static const char *const GCCRegNames[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "r8", "r9", "r10", "r11", "r12", "r13", "sp", "lr"};
    return llvm::ArrayRef(GCCRegNames);
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return {};
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    switch (*Name) {
    default:
      return false;
    case 'r':
      Info.setAllowsRegister();
      return true;
    case 'I':
    case 'J':
    case 'K':
      Info.setRequiresImmediate();
      return true;
    }
  }

  bool hasFeature(StringRef Feature) const override {
    return Feature == "tc32";
  }

  bool isValidCPUName(StringRef Name) const override {
    return llvm::StringSwitch<bool>(Name).Case("generic", true).Default(false);
  }

  bool setCPU(const std::string &Name) override {
    CPU = Name;
    return isValidCPUName(Name);
  }
};

} // namespace targets
} // namespace clang

#endif
