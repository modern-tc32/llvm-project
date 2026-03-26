#include "TC32.h"
#include "TC32MCInstLower.h"
#include "TC32TargetMachine.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "TC32GenInstrInfo.inc"

#define DEBUG_TYPE "asm-printer"

namespace {

class TC32AsmPrinter : public AsmPrinter {
public:
  static char ID;

  struct LiteralPoolEntry {
    MCSymbol *Label;
    const MCExpr *Value;
  };

  TC32AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "TC32 Assembly Printer"; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    SetupMachineFunction(MF);
    LiteralPool.clear();
    emitFunctionBody();
    emitLiteralPool();
    return false;
  }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == TC32::TLOADaddr) {
      MCSymbol *InstLabel = OutContext.createTempSymbol();
      MCSymbol *PoolLabel = OutContext.createTempSymbol();
      OutStreamer->emitLabel(InstLabel);

      const MCExpr *PoolDiff = MCBinaryExpr::createSub(
          MCSymbolRefExpr::create(PoolLabel, OutContext),
          MCSymbolRefExpr::create(InstLabel, OutContext), OutContext);
      const MCExpr *ImmExpr = MCBinaryExpr::createAdd(
          PoolDiff, MCConstantExpr::create(-4, OutContext), OutContext);

      MCInst Load;
      Load.setOpcode(TC32::TLOADpcu8);
      Load.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
      Load.addOperand(MCOperand::createExpr(ImmExpr));
      EmitToStreamer(*OutStreamer, Load);

      TC32MCInstLower Lower(OutContext, *this);
      MCOperand Value = Lower.lowerOperand(MI->getOperand(1));
      LiteralPool.push_back({PoolLabel, Value.getExpr()});
      return;
    }

    TC32MCInstLower Lower(OutContext, *this);
    MCInst OutMI;
    Lower.lower(MI, OutMI);
    EmitToStreamer(*OutStreamer, OutMI);
  }

private:
  SmallVector<LiteralPoolEntry, 8> LiteralPool;

  void emitLiteralPool() {
    if (LiteralPool.empty())
      return;

    OutStreamer->emitValueToAlignment(Align(4));
    for (const LiteralPoolEntry &Entry : LiteralPool) {
      OutStreamer->emitLabel(Entry.Label);
      OutStreamer->emitValue(Entry.Value, 4);
    }
    LiteralPool.clear();
  }
};

} // namespace

char TC32AsmPrinter::ID = 0;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32AsmPrinter() {
  RegisterAsmPrinter<TC32AsmPrinter> X(getTheTC32Target());
}
