#include "TC32InstPrinter.h"
#include "TC32MCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

#define PRINT_ALIAS_INSTR
#include "TC32GenAsmWriter.inc"

void TC32InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

void TC32InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  switch (MI->getOpcode()) {
  case TC32::TPUSH_R0_R1_R2_R3:
    O << "tpush\t{r0, r1, r2, r3}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPUSH_LR:
    O << "tpush\t{lr}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPUSH_R7_LR:
    O << "tpush\t{r7, lr}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPOP_R7:
    O << "tpop\t{r7}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPOP_R3:
    O << "tpop\t{r3}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPOP_PC:
    O << "tpop\t{pc}";
    printAnnotation(O, Annot);
    return;
  case TC32::TPOP_R7_PC:
    O << "tpop\t{r7, pc}";
    printAnnotation(O, Annot);
    return;
  case TC32::TADDCrr:
    O << "taddc\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TSUBCrr:
    O << "tsubc\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TMULrr:
    O << "tmul\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TANDrr:
    O << "tand\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TORrr:
    O << "tor\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TXORrr:
    O << "txor\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TRET_R0:
    O << "tmov\tpc, lr";
    printAnnotation(O, Annot);
    return;
  case TC32::TJEXr:
    O << "tjex\t";
    printOperand(MI, 0, O);
    printAnnotation(O, Annot);
    return;
  case TC32::TLOADrr:
    O << "tloadr\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TLOADpcu8:
    O << "tloadr\t";
    printOperand(MI, 0, O);
    O << ", [pc, #";
    printOperand(MI, 1, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TLOADru3:
    O << "tloadr\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << ", #";
    printOperand(MI, 2, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TSTORErr:
    O << "tstorer\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TLOADBrr:
    O << "tloadrb\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TLOADBru3:
    O << "tloadrb\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << ", #";
    printOperand(MI, 2, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TSTOREBrr:
    O << "tstorerb\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TSTOREru3:
    O << "tstorer\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << ", #";
    printOperand(MI, 2, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  case TC32::TSTOREBru3:
    O << "tstorerb\t";
    printOperand(MI, 0, O);
    O << ", [";
    printOperand(MI, 1, O);
    O << ", #";
    printOperand(MI, 2, O);
    O << "]";
    printAnnotation(O, Annot);
    return;
  default:
    break;
  }

  if (!printAliasInstr(MI, Address, O))
    printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void TC32InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    O << getRegisterName(Op.getReg());
    return;
  }
  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }
  if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
    return;
  }
  llvm_unreachable("unknown TC32 operand kind");
}
