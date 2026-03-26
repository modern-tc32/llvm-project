#include "TC32.h"
#include "Targets.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

void TC32TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  DefineStd(Builder, "tc32", Opts);
  Builder.defineMacro("__TC32__");
  Builder.defineMacro("__tc32__");
  Builder.defineMacro("__ELF__");
}
