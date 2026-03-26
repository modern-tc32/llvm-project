#ifndef LLVM_LIB_TARGET_TC32_TC32MCINSTLOWER_H
#define LLVM_LIB_TARGET_TC32_TC32MCINSTLOWER_H

#include "llvm/MC/MCContext.h"

namespace llvm {

class AsmPrinter;
class MachineInstr;
class MCInst;
class MCOperand;
class MachineOperand;
class MCSymbol;

class TC32MCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

  MCOperand lowerOperand(const MachineOperand &MO) const;

public:
  TC32MCInstLower(MCContext &Ctx, AsmPrinter &Printer)
      : Ctx(Ctx), Printer(Printer) {}

  void lower(const MachineInstr *MI, MCInst &OutMI) const;
};

} // namespace llvm

#endif
