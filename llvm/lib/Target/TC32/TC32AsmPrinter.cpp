#include "TC32.h"
#include "TC32MCInstLower.h"
#include "TC32RegisterInfo.h"
#include "TC32TargetMachine.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAssembler.h"
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

  void emitFunctionEntryLabel() override {
    if (TM.getTargetTriple().isOSBinFormatELF()) {
      if (MCAssembler *Asm = OutStreamer->getAssemblerPtr())
        Asm->setIsThumbFunc(CurrentFnSym);
    }
    AsmPrinter::emitFunctionEntryLabel();
  }

  void emitFunctionBodyStart() override { PendingAddrLiterals.clear(); }

  void emitFunctionBodyEnd() override {
    if (!PendingAddrLiterals.empty()) {
      OutStreamer->emitValueToAlignment(Align(4));
      for (const PendingAddrLiteral &Literal : PendingAddrLiterals) {
        OutStreamer->emitLabel(Literal.Label);
        OutStreamer->emitValue(Literal.Value, 4);
      }
      PendingAddrLiterals.clear();
    }
    AsmPrinter::emitFunctionBodyEnd();
  }

private:
  struct PendingAddrLiteral {
    MCSymbol *Label;
    const MCExpr *Value;
  };

  SmallVector<PendingAddrLiteral, 8> PendingAddrLiterals;

  const MCExpr *getAddrLiteralValue(const MachineInstr *MI,
                                    const MachineOperand &MO) {
    if (MO.getType() == MachineOperand::MO_ConstantPoolIndex) {
      const auto &Constants =
          MI->getMF()->getConstantPool()->getConstants();
      const MachineConstantPoolEntry &Entry = Constants[MO.getIndex()];
      if (!Entry.isMachineConstantPoolEntry()) {
        if (const auto *CI = dyn_cast<ConstantInt>(Entry.Val.ConstVal))
          return MCConstantExpr::create(CI->getZExtValue() + MO.getOffset(),
                                        OutContext);
      }
    }

    TC32MCInstLower Lower(OutContext, *this);
    MCOperand Value = Lower.lowerOperand(MO);
    if (!Value.isExpr())
      report_fatal_error("TC32 TLOADaddr expects expression operand");
    return Value.getExpr();
  }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == TC32::TLOADaddr) {
      MCSymbol *PoolLabel = OutContext.createTempSymbol();

      MCInst Load;
      Load.setOpcode(TC32::TLOADpcu8);
      Load.addOperand(MCOperand::createReg(MI->getOperand(0).getReg()));
      Load.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(PoolLabel, OutContext)));
      EmitToStreamer(*OutStreamer, Load);

      PendingAddrLiterals.push_back(
          {PoolLabel, getAddrLiteralValue(MI, MI->getOperand(1))});
      return;
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
