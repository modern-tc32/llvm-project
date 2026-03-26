#include "TC32MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

TC32MCAsmInfo::TC32MCAsmInfo(const Triple &TT) {
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  IsLittleEndian = true;
  CommentString = "@";
  GlobalDirective = "\t.global\t";
  ZeroDirective = "\t.space\t";
  Data8bitsDirective = "\t.byte\t";
  Data16bitsDirective = "\t.short\t";
  Data32bitsDirective = "\t.long\t";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
}
