#include "TC32.h"
#include "TC32MCInstLower.h"
#include "TC32RegisterInfo.h"
#include "TC32TargetMachine.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Compiler.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

#define GET_REGINFO_ENUM
#include "TC32GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "TC32GenInstrInfo.inc"

#define DEBUG_TYPE "asm-printer"

namespace {

class TC32AsmPrinter : public AsmPrinter {
public:
  static char ID;

  TC32AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "TC32 Assembly Printer"; }

  bool doInitialization(Module &M) override {
    const DataLayout &DL = M.getDataLayout();
    for (GlobalVariable &GV : M.globals()) {
      if (GV.isDeclaration())
        continue;
      if (GV.getAlign().valueOrOne() >= Align(4))
        continue;
      if (DL.getTypeAllocSize(GV.getValueType()) < 4)
        continue;
      GV.setAlignment(Align(4));
    }
    return AsmPrinter::doInitialization(M);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    SetupMachineFunction(MF);
    emitFunctionBody();
    return false;
  }

private:
  void emitLongJump(const MCOperand &Dest, unsigned ScratchReg) {
    MCSymbol *PoolLabel = OutContext.createTempSymbol();
    MCSymbol *SkipLabel = OutContext.createTempSymbol();

    MCInst Load;
    Load.setOpcode(TC32::TLOADpcu8);
    Load.addOperand(MCOperand::createReg(ScratchReg));
    Load.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(PoolLabel, OutContext)));
    EmitToStreamer(*OutStreamer, Load);

    MCInst SkipPool;
    SkipPool.setOpcode(TC32::TJ);
    SkipPool.addOperand(
        MCOperand::createExpr(MCSymbolRefExpr::create(SkipLabel, OutContext)));
    EmitToStreamer(*OutStreamer, SkipPool);

    OutStreamer->emitValueToAlignment(Align(4));
    OutStreamer->emitLabel(PoolLabel);
    OutStreamer->emitValue(Dest.getExpr(), 4);
    OutStreamer->emitLabel(SkipLabel);

    MCInst Jump;
    Jump.setOpcode(TC32::TMOVrr);
    Jump.addOperand(MCOperand::createReg(TC32::R15));
    Jump.addOperand(MCOperand::createReg(ScratchReg));
    EmitToStreamer(*OutStreamer, Jump);
  }

public:

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == TC32::TLOADaddr) {
      MCSymbol *PoolLabel = OutContext.createTempSymbol();
      MCSymbol *SkipLabel = OutContext.createTempSymbol();

      MCInst Load;
      Load.setOpcode(TC32::TLOADpcu8);
      Load.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
      Load.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(PoolLabel, OutContext)));
      EmitToStreamer(*OutStreamer, Load);

      MCInst SkipPool;
      SkipPool.setOpcode(TC32::TJ);
      SkipPool.addOperand(
          MCOperand::createExpr(MCSymbolRefExpr::create(SkipLabel, OutContext)));
      EmitToStreamer(*OutStreamer, SkipPool);

      TC32MCInstLower Lower(OutContext, *this);
      MCOperand Value = Lower.lowerOperand(MI->getOperand(1));
      OutStreamer->emitValueToAlignment(Align(4));
      OutStreamer->emitLabel(PoolLabel);
      OutStreamer->emitValue(Value.getExpr(), 4);
      OutStreamer->emitLabel(SkipLabel);
      return;
    }

    if (MI->getOpcode() == TC32::TJ) {
      TC32MCInstLower Lower(OutContext, *this);
      MCOperand Dest = Lower.lowerOperand(MI->getOperand(0));
      emitLongJump(Dest, TC32::R6);
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

      emitLongJump(Dest, TC32::R6);

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
};

} // namespace

char TC32AsmPrinter::ID = 0;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32AsmPrinter() {
  RegisterAsmPrinter<TC32AsmPrinter> X(getTheTC32Target());
}
