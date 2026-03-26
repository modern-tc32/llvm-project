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

      MCInst Load;
      Load.setOpcode(TC32::TLOADpcu8);
      Load.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
      Load.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(PoolLabel, OutContext)));
      EmitToStreamer(*OutStreamer, Load);

      TC32MCInstLower Lower(OutContext, *this);
      MCOperand Value = Lower.lowerOperand(MI->getOperand(1));
      LiteralPool.push_back({PoolLabel, Value.getExpr()});
      return;
    }

    switch (MI->getOpcode()) {
    case TC32::TJEQ:
    case TC32::TJNE:
    case TC32::TJGE:
    case TC32::TJLT:
    case TC32::TJGT:
    case TC32::TJLE: {
      unsigned InverseOpc = 0;
      switch (MI->getOpcode()) {
      case TC32::TJEQ: InverseOpc = TC32::TJNE; break;
      case TC32::TJNE: InverseOpc = TC32::TJEQ; break;
      case TC32::TJGE: InverseOpc = TC32::TJLT; break;
      case TC32::TJLT: InverseOpc = TC32::TJGE; break;
      case TC32::TJGT: InverseOpc = TC32::TJLE; break;
      case TC32::TJLE: InverseOpc = TC32::TJGT; break;
      default: llvm_unreachable("unexpected tc32 conditional branch opcode");
      }

      TC32MCInstLower Lower(OutContext, *this);
      MCOperand Dest = Lower.lowerOperand(MI->getOperand(0));
      MCSymbol *SkipLabel = OutContext.createTempSymbol();

      MCInst SkipBranch;
      SkipBranch.setOpcode(InverseOpc);
      SkipBranch.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(SkipLabel, OutContext)));
      EmitToStreamer(*OutStreamer, SkipBranch);

      MCInst Jump;
      Jump.setOpcode(TC32::TJ);
      Jump.addOperand(Dest);
      EmitToStreamer(*OutStreamer, Jump);

      OutStreamer->emitLabel(SkipLabel);
      return;
    }
    default:
      break;
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
