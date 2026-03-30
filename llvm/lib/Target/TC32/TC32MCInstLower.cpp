#include "TC32MCInstLower.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

MCOperand TC32MCInstLower::lowerOperand(const MachineOperand &MO) const {
  switch (MO.getType()) {
  default:
    llvm_unreachable("unsupported TC32 machine operand");
  case MachineOperand::MO_Register:
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx));
  case MachineOperand::MO_GlobalAddress: {
    const MCExpr *Expr =
        MCSymbolRefExpr::create(Printer.getSymbol(MO.getGlobal()), Ctx);
    if (int64_t Offset = MO.getOffset(); Offset != 0) {
      Expr = MCBinaryExpr::createAdd(
          Expr, MCConstantExpr::create(Offset, Ctx), Ctx);
    }
    return MCOperand::createExpr(Expr);
  }
  case MachineOperand::MO_ExternalSymbol:
    return MCOperand::createExpr(MCSymbolRefExpr::create(
        Printer.GetExternalSymbolSymbol(MO.getSymbolName()), Ctx));
  case MachineOperand::MO_JumpTableIndex:
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(Printer.GetJTISymbol(MO.getIndex()), Ctx));
  case MachineOperand::MO_BlockAddress:
    return MCOperand::createExpr(MCSymbolRefExpr::create(
        Printer.GetBlockAddressSymbol(MO.getBlockAddress()), Ctx));
  case MachineOperand::MO_ConstantPoolIndex: {
    const MCExpr *Expr = MCSymbolRefExpr::create(Printer.GetCPISymbol(MO.getIndex()), Ctx);
    if (int64_t Offset = MO.getOffset(); Offset != 0) {
      Expr = MCBinaryExpr::createAdd(
          Expr, MCConstantExpr::create(Offset, Ctx), Ctx);
    }
    return MCOperand::createExpr(Expr);
  }
  }
}

void TC32MCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  if (MI->getOpcode() == TC32::TJL || MI->getOpcode() == TC32::TJL_R0 ||
      MI->getOpcode() == TC32::TJL_R0_R1 || MI->getOpcode() == TC32::TJL_R0_R1_R2 ||
      MI->getOpcode() == TC32::TJL_R0_R1_R2_R3) {
    for (const MachineOperand &MO : MI->operands()) {
      if (MO.isImplicit() || MO.isRegMask())
        continue;
      switch (MO.getType()) {
      case MachineOperand::MO_GlobalAddress:
      case MachineOperand::MO_ExternalSymbol:
      case MachineOperand::MO_MachineBasicBlock:
      case MachineOperand::MO_BlockAddress:
        OutMI.addOperand(lowerOperand(MO));
        return;
      default:
        break;
      }
    }
    report_fatal_error("TC32 TJL is missing a call target operand");
  }

  if (MI->getOpcode() == TC32::TADDCrr || MI->getOpcode() == TC32::TSUBCrr ||
      MI->getOpcode() == TC32::TANDrr || MI->getOpcode() == TC32::TORrr ||
      MI->getOpcode() == TC32::TXORrr || MI->getOpcode() == TC32::TMULrr) {
    SmallVector<MCOperand, 4> Ops;
    unsigned Added = 0;
    for (const MachineOperand &MO : MI->operands()) {
      if (MO.isImplicit() || MO.isRegMask() ||
          (MO.isReg() && MO.getReg() == 0))
        continue;
      Ops.push_back(lowerOperand(MO));
      ++Added;
    }
    if (Added >= 3) {
      OutMI.addOperand(Ops[0]);
      OutMI.addOperand(Ops[2]);
    } else if (Added == 2) {
      OutMI.addOperand(Ops[0]);
      OutMI.addOperand(Ops[1]);
    }
    return;
  }

  for (const MachineOperand &MO : MI->operands()) {
    if (MO.isImplicit() || MO.isRegMask() ||
        (MO.isReg() && MO.getReg() == 0))
      continue;
    OutMI.addOperand(lowerOperand(MO));
  }
}
