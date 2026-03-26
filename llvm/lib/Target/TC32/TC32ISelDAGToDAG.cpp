#include "TC32.h"
#include "TC32ISelLowering.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32TargetMachine.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class TC32DAGToDAGISel : public SelectionDAGISel {
public:
  explicit TC32DAGToDAGISel(TC32TargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

  SDValue selectSetCCValue(SDLoc DL, ISD::CondCode CCVal, SDValue LHS,
                           SDValue RHS) {
    if (LHS.getValueType() != MVT::i32 || RHS.getValueType() != MVT::i32)
      return SDValue();

    auto isConst = [](SDValue V, int64_t &Value) {
      if (const auto *CN = dyn_cast<ConstantSDNode>(V)) {
        Value = CN->getSExtValue();
        return true;
      }
      return false;
    };

    int64_t LV, RV;
    bool HasLConst = isConst(LHS, LV);
    bool HasRConst = isConst(RHS, RV);

    auto emitEqZero = [&](SDValue Value) -> SDValue {
      MachineSDNode *Neg =
          CurDAG->getMachineNode(TC32::TNEGrr, DL, {MVT::i32, MVT::Glue}, {Value});
      return SDValue(CurDAG->getMachineNode(TC32::TADDCrr, DL, MVT::i32, Value,
                                            SDValue(Neg, 0), SDValue(Neg, 1)),
                     0);
    };

    auto emitNeZero = [&](SDValue Value) -> SDValue {
      MachineSDNode *Dec =
          CurDAG->getMachineNode(TC32::TSUBri1, DL, {MVT::i32, MVT::Glue}, {Value});
      return SDValue(CurDAG->getMachineNode(TC32::TSUBCrr, DL, MVT::i32, Value,
                                            SDValue(Dec, 0), SDValue(Dec, 1)),
                     0);
    };

    switch (CCVal) {
    case ISD::SETEQ:
    case ISD::SETNE: {
      SDValue Diff;
      if (HasLConst && LV == 0)
        Diff = RHS;
      else if (HasRConst && RV == 0)
        Diff = LHS;
      else if (!HasLConst && !HasRConst)
        Diff = SDValue(CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32, LHS, RHS), 0);
      else
        return SDValue();

      return CCVal == ISD::SETEQ ? emitEqZero(Diff) : emitNeZero(Diff);
    }
    case ISD::SETLT:
      if (HasRConst && RV == 0)
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, LHS), 0);
      if (HasLConst && LV == 0) {
        SDValue Neg = SDValue(CurDAG->getMachineNode(TC32::TMOVNrr, DL, MVT::i32, RHS), 0);
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, Neg), 0);
      }
      if (!HasLConst && !HasRConst) {
        SDValue GE = SDValue();
        SDValue SignL =
            SDValue(CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32, LHS), 0);
        SDValue SignR =
            SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, RHS), 0);
        MachineSDNode *Cmp =
            CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, LHS, RHS);
        GE = SDValue(CurDAG->getMachineNode(TC32::TADDCrr, DL, MVT::i32, SignL, SignR,
                                            SDValue(Cmp, 0)),
                     0);
        return emitEqZero(GE);
      }
      return SDValue();
    case ISD::SETGE:
      if (HasRConst && RV == 0) {
        SDValue Neg = SDValue(CurDAG->getMachineNode(TC32::TMOVNrr, DL, MVT::i32, LHS), 0);
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, Neg), 0);
      }
      if (HasLConst && LV == 0)
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, RHS), 0);
      if (!HasLConst && !HasRConst) {
        SDValue SignL =
            SDValue(CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32, LHS), 0);
        SDValue SignR =
            SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, RHS), 0);
        MachineSDNode *Cmp =
            CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, LHS, RHS);
        return SDValue(CurDAG->getMachineNode(TC32::TADDCrr, DL, MVT::i32, SignL,
                                              SignR, SDValue(Cmp, 0)),
                       0);
      }
      return SDValue();
    case ISD::SETGT:
      if (HasRConst && RV == -1) {
        SDValue Neg = SDValue(CurDAG->getMachineNode(TC32::TMOVNrr, DL, MVT::i32, LHS), 0);
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, Neg), 0);
      }
      if (HasLConst && LV == -1)
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, RHS), 0);
      if (!HasLConst && !HasRConst) {
        SDValue LE = SDValue();
        SDValue SignL =
            SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, LHS), 0);
        SDValue SignR =
            SDValue(CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32, RHS), 0);
        MachineSDNode *Cmp =
            CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, RHS, LHS);
        LE = SDValue(CurDAG->getMachineNode(TC32::TADDCrr, DL, MVT::i32, SignL, SignR,
                                            SDValue(Cmp, 0)),
                     0);
        return emitEqZero(LE);
      }
      return SDValue();
    case ISD::SETLE:
      if (HasRConst && RV == -1)
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, LHS), 0);
      if (HasLConst && LV == -1) {
        SDValue Neg = SDValue(CurDAG->getMachineNode(TC32::TMOVNrr, DL, MVT::i32, RHS), 0);
        return SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, Neg), 0);
      }
      if (!HasLConst && !HasRConst) {
        SDValue SignL =
            SDValue(CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32, LHS), 0);
        SDValue SignR =
            SDValue(CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32, RHS), 0);
        MachineSDNode *Cmp =
            CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, RHS, LHS);
        return SDValue(CurDAG->getMachineNode(TC32::TADDCrr, DL, MVT::i32, SignL,
                                              SignR, SDValue(Cmp, 0)),
                       0);
      }
      return SDValue();
    default:
      return SDValue();
    }
  }

  SDValue selectSetCC(SDNode *Node) {
    SDLoc DL(Node);
    auto *CC = dyn_cast<CondCodeSDNode>(Node->getOperand(2));
    if (!CC || Node->getValueType(0) != MVT::i32)
      return SDValue();
    return selectSetCCValue(DL, CC->get(), Node->getOperand(0), Node->getOperand(1));
  }

  void Select(SDNode *Node) override {
    SDLoc DL(Node);

    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }

    switch (Node->getOpcode()) {
    case ISD::EntryToken:
    case ISD::Register:
    case ISD::TokenFactor:
    case ISD::BasicBlock:
    case ISD::VALUETYPE:
    case ISD::CONDCODE:
    case ISD::TargetGlobalAddress:
    case ISD::TargetExternalSymbol:
      Node->setNodeId(-1);
      return;
    case ISD::CopyFromReg:
    case ISD::CopyToReg:
      Node->setNodeId(-1);
      return;
    case ISD::UNDEF:
      CurDAG->SelectNodeTo(Node, TargetOpcode::IMPLICIT_DEF,
                           Node->getValueType(0));
      return;
    case ISD::AssertZext:
      ReplaceNode(Node, Node->getOperand(0).getNode());
      return;
    case ISD::ZERO_EXTEND:
      if (Node->getValueType(0) == MVT::i32 &&
          Node->getOperand(0).getValueType() == MVT::i8) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32,
                                                 Node->getOperand(0)));
        return;
      }
      break;
    case ISD::BR: {
      ReplaceNode(Node, CurDAG->getMachineNode(TC32::TJ, DL, MVT::Other,
                                               Node->getOperand(1),
                                               Node->getOperand(0)));
      return;
    }
    case ISD::BR_CC: {
      SDValue Chain = Node->getOperand(0);
      auto *CC = dyn_cast<CondCodeSDNode>(Node->getOperand(1));
      if (!CC)
        break;

      SDValue LHS = Node->getOperand(2);
      SDValue RHS = Node->getOperand(3);
      SDValue Dest = Node->getOperand(4);
      MachineSDNode *Cmp = nullptr;

      unsigned BrOpc = 0;
      switch (CC->get()) {
      case ISD::SETEQ:
        BrOpc = TC32::TJEQ;
        break;
      case ISD::SETNE:
        BrOpc = TC32::TJNE;
        break;
      case ISD::SETGE:
        BrOpc = TC32::TJGE;
        break;
      case ISD::SETLT:
        BrOpc = TC32::TJLT;
        break;
      case ISD::SETGT:
        BrOpc = TC32::TJGT;
        break;
      case ISD::SETLE:
        BrOpc = TC32::TJLE;
        break;
      default:
        break;
      }
      if (BrOpc == 0)
        break;

      if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
        if (RC->isZero()) {
          Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
        } else if (RC->getSExtValue() == -1) {
          switch (CC->get()) {
          case ISD::SETGT:
            Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
            BrOpc = TC32::TJGE;
            break;
          case ISD::SETLE:
            Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
            BrOpc = TC32::TJLT;
            break;
          default:
            break;
          }
        }
      } else if (auto *LC = dyn_cast<ConstantSDNode>(LHS); LC && LC->isZero()) {
        Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, RHS);
      }
      if (!Cmp)
        Cmp = CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, LHS, RHS);

      ReplaceNode(Node, CurDAG->getMachineNode(BrOpc, DL, MVT::Other, Dest,
                                               Chain, SDValue(Cmp, 0)));
      return;
    }
    case ISD::BRCOND: {
      SDValue Chain = Node->getOperand(0);
      SDValue Cond = Node->getOperand(1);
      SDValue Dest = Node->getOperand(2);
      if (Cond.getOpcode() != ISD::SETCC)
        break;

      auto *CC = dyn_cast<CondCodeSDNode>(Cond.getOperand(2));
      if (!CC)
        break;

      SDValue LHS = Cond.getOperand(0);
      SDValue RHS = Cond.getOperand(1);
      MachineSDNode *Cmp = nullptr;

      unsigned BrOpc = 0;
      switch (CC->get()) {
      case ISD::SETEQ:
        BrOpc = TC32::TJEQ;
        break;
      case ISD::SETNE:
        BrOpc = TC32::TJNE;
        break;
      case ISD::SETGE:
        BrOpc = TC32::TJGE;
        break;
      case ISD::SETLT:
        BrOpc = TC32::TJLT;
        break;
      case ISD::SETGT:
        BrOpc = TC32::TJGT;
        break;
      case ISD::SETLE:
        BrOpc = TC32::TJLE;
        break;
      default:
        break;
      }
      if (BrOpc == 0)
        break;

      if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
        if (RC->isZero()) {
          Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
        } else if (RC->getSExtValue() == -1) {
          switch (CC->get()) {
          case ISD::SETGT:
            Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
            BrOpc = TC32::TJGE;
            break;
          case ISD::SETLE:
            Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
            BrOpc = TC32::TJLT;
            break;
          default:
            break;
          }
        }
      } else if (auto *LC = dyn_cast<ConstantSDNode>(LHS); LC && LC->isZero()) {
        Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, RHS);
      }
      if (!Cmp)
        Cmp = CurDAG->getMachineNode(TC32::TCMPrr, DL, MVT::Glue, LHS, RHS);

      ReplaceNode(Node, CurDAG->getMachineNode(BrOpc, DL, MVT::Other, Dest,
                                               Chain, SDValue(Cmp, 0)));
      return;
    }
    case ISD::Constant: {
      auto *CN = dyn_cast<ConstantSDNode>(Node);
      if (Node->getValueType(0) == MVT::i32 && CN && CN->getZExtValue() <= 255) {
        ReplaceNode(Node, CurDAG->getMachineNode(
                              TC32::TMOVi8, DL, MVT::i32,
                              CurDAG->getTargetConstant(CN->getZExtValue(), DL, MVT::i32)));
        return;
      }
      break;
    }
    case ISD::SETCC: {
      if (SDValue Res = selectSetCC(Node)) {
        ReplaceNode(Node, Res.getNode());
        return;
      }
      break;
    }
    case ISD::SIGN_EXTEND_INREG: {
      if (Node->getValueType(0) == MVT::i32 &&
          cast<VTSDNode>(Node->getOperand(1))->getVT() == MVT::i1) {
        SDValue One = SDValue(CurDAG->getMachineNode(
                                  TC32::TMOVi8, DL, MVT::i32,
                                  CurDAG->getTargetConstant(1, DL, MVT::i32)),
                              0);
        SDValue Zero = SDValue(CurDAG->getMachineNode(
                                   TC32::TMOVi8, DL, MVT::i32,
                                   CurDAG->getTargetConstant(0, DL, MVT::i32)),
                               0);
        SDValue Bit = SDValue(CurDAG->getMachineNode(TC32::TANDrr, DL, MVT::i32,
                                                     Node->getOperand(0), One),
                              0);
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32,
                                                 Zero, Bit));
        return;
      }
      break;
    }
    case ISD::SELECT_CC: {
      if (Node->getValueType(0) != MVT::i32)
        break;
      auto *CC = dyn_cast<CondCodeSDNode>(Node->getOperand(4));
      if (!CC)
        break;

      SDValue LHS = Node->getOperand(0);
      SDValue RHS = Node->getOperand(1);
      SDValue TrueVal = Node->getOperand(2);
      SDValue FalseVal = Node->getOperand(3);

      SDValue BoolVal = selectSetCCValue(DL, CC->get(), LHS, RHS);
      if (!BoolVal)
        break;

      auto materializeValue = [&](SDValue V) -> SDValue {
        if (auto *CN = dyn_cast<ConstantSDNode>(V)) {
          if (CN->getZExtValue() <= 255)
            return SDValue(CurDAG->getMachineNode(
                               TC32::TMOVi8, DL, MVT::i32,
                               CurDAG->getTargetConstant(CN->getZExtValue(), DL, MVT::i32)),
                           0);
        }
        return V;
      };

      TrueVal = materializeValue(TrueVal);
      FalseVal = materializeValue(FalseVal);

      SDValue Zero = SDValue(CurDAG->getMachineNode(
                                 TC32::TMOVi8, DL, MVT::i32,
                                 CurDAG->getTargetConstant(0, DL, MVT::i32)),
                             0);
      SDValue Mask =
          SDValue(CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32, Zero, BoolVal), 0);
      SDValue Diff =
          SDValue(CurDAG->getMachineNode(TC32::TXORrr, DL, MVT::i32, TrueVal, FalseVal), 0);
      SDValue Bits =
          SDValue(CurDAG->getMachineNode(TC32::TANDrr, DL, MVT::i32, Diff, Mask), 0);
      ReplaceNode(Node, CurDAG->getMachineNode(TC32::TXORrr, DL, MVT::i32, FalseVal, Bits));
      return;
    }
    case ISD::ADD: {
      if (Node->getValueType(0) == MVT::i32) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        if (auto *RC = dyn_cast<ConstantSDNode>(RHS); RC && RC->getSExtValue() == -1) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBri1, DL, MVT::i32, LHS));
          return;
        }
        if (auto *LC = dyn_cast<ConstantSDNode>(LHS); LC && LC->getSExtValue() == -1) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBri1, DL, MVT::i32, RHS));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TADDSrrr, DL, MVT::i32, LHS, RHS));
        return;
      }
      break;
    }
    case ISD::SUB: {
      if (Node->getValueType(0) == MVT::i32) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        ReplaceNode(
            Node, CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32, LHS, RHS));
        return;
      }
      break;
    }
    case ISD::AND: {
      if (Node->getValueType(0) == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TANDrr, DL, MVT::i32,
                                                 Node->getOperand(0),
                                                 Node->getOperand(1)));
        return;
      }
      break;
    }
    case ISD::OR: {
      if (Node->getValueType(0) == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TORrr, DL, MVT::i32,
                                                 Node->getOperand(0),
                                                 Node->getOperand(1)));
        return;
      }
      break;
    }
    case ISD::MUL: {
      if (Node->getValueType(0) == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TMULrr, DL, MVT::i32,
                                                 Node->getOperand(0),
                                                 Node->getOperand(1)));
        return;
      }
      break;
    }
    case ISD::XOR: {
      if (Node->getValueType(0) == MVT::i32) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getSExtValue() == -1) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TMOVNrr, DL, MVT::i32,
                                                   Node->getOperand(0)));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TXORrr, DL, MVT::i32,
                                                 Node->getOperand(0),
                                                 Node->getOperand(1)));
        return;
      }
      break;
    }
    case ISD::SHL: {
      if (Node->getValueType(0) == MVT::i32) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
      }
      break;
    }
    case ISD::SRL: {
      if (Node->getValueType(0) == MVT::i32) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() == 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTRi31, DL, MVT::i32,
                                                   Node->getOperand(0)));
          return;
        }
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTRi5, DL, MVT::i32,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
      }
      break;
    }
    case ISD::SRA: {
      if (Node->getValueType(0) == MVT::i32) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() == 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32,
                                                   Node->getOperand(0)));
          return;
        }
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TASRi5, DL, MVT::i32,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
      }
      break;
    }
    case ISD::LOAD: {
      auto *LD = cast<LoadSDNode>(Node);
      if (Node->getValueType(0) == MVT::i32 &&
          LD->getMemoryVT() == MVT::i8 &&
          LD->getExtensionType() == ISD::ZEXTLOAD &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBrr, DL,
                                                 {MVT::i32, MVT::Other},
                                                 {LD->getBasePtr(),
                                                  LD->getChain()}));
        return;
      }
      if (Node->getValueType(0) == MVT::i8 && LD->getMemoryVT() == MVT::i8 &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBrr, DL,
                                                 {MVT::i8, MVT::Other},
                                                 {LD->getBasePtr(),
                                                  LD->getChain()}));
        return;
      }
      if (Node->getValueType(0) == MVT::i32 && LD->getMemoryVT() == MVT::i32 &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        SDValue Base = LD->getBasePtr();
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADrr, DL,
                                                 {MVT::i32, MVT::Other},
                                                 {Base, LD->getChain()}));
        return;
      }
      if (Node->getValueType(0) == MVT::i32 &&
          LD->getBasePtr().getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(LD->getBasePtr())->getIndex();
        const MachineFrameInfo &MFI = CurDAG->getMachineFunction().getFrameInfo();
        bool Fixed = MFI.isFixedObjectIndex(FI);
        int Offset = MFI.getObjectOffset(FI);
        int StackSize = MFI.getStackSize();
        int Imm = Fixed ? Offset : Offset + StackSize;
        unsigned Opc = Fixed ? TC32::TLOADspu8 : TC32::TLOADfpu8;
        if (CurDAG->getSubtarget().getFrameLowering()->hasFP(
                CurDAG->getMachineFunction()) &&
            Fixed) {
          Imm += StackSize + 8;
          Opc = TC32::TLOADfpu8;
        }
        if (Imm >= 0 && Imm <= (Opc == TC32::TLOADspu8 ? 1020 : 252) &&
            (Imm & 3) == 0) {
          SDValue ImmVal = CurDAG->getTargetConstant(Imm, DL, MVT::i32);
          ReplaceNode(Node, CurDAG->getMachineNode(Opc, DL,
                                                   {MVT::i32, MVT::Other},
                                                   {ImmVal, LD->getChain()}));
          return;
        }
      }
      break;
    }
    case ISD::STORE: {
      auto *ST = cast<StoreSDNode>(Node);
      if (ST->getMemoryVT() == MVT::i8 &&
          ST->getBasePtr().getValueType() == MVT::i32 &&
          ST->getValue().getValueType() == MVT::i8) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
      }
      if (ST->getMemoryVT() == MVT::i8 &&
          ST->getBasePtr().getValueType() == MVT::i32 &&
          ST->getValue().getValueType() == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
      }
      if (ST->getValue().getValueType() == MVT::i32 &&
          ST->getMemoryVT() == MVT::i32 &&
          ST->getBasePtr().getValueType() == MVT::i32) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTORErr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
      }
      if (ST->getValue().getValueType() == MVT::i32 &&
          ST->getBasePtr().getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(ST->getBasePtr())->getIndex();
        const MachineFrameInfo &MFI = CurDAG->getMachineFunction().getFrameInfo();
        bool Fixed = MFI.isFixedObjectIndex(FI);
        int Offset = MFI.getObjectOffset(FI);
        int StackSize = MFI.getStackSize();
        int Imm = Fixed ? Offset : Offset + StackSize;
        unsigned Opc = Fixed ? TC32::TSTOREspu8 : TC32::TSTOREfpu8;
        if (CurDAG->getSubtarget().getFrameLowering()->hasFP(
                CurDAG->getMachineFunction()) &&
            Fixed) {
          Imm += StackSize + 8;
          Opc = TC32::TSTOREfpu8;
        }
        if (Imm >= 0 && Imm <= (Opc == TC32::TSTOREspu8 ? 1020 : 252) &&
            (Imm & 3) == 0) {
          SDValue ImmVal = CurDAG->getTargetConstant(Imm, DL, MVT::i32);
          ReplaceNode(Node, CurDAG->getMachineNode(Opc, DL, MVT::Other,
                                                   ST->getValue(), ImmVal,
                                                   ST->getChain()));
          return;
        }
      }
      break;
    }
    case TC32ISD::RET_FLAG: {
      SmallVector<SDValue, 2> Ops;
      Ops.push_back(Node->getOperand(0));
      if (Node->getNumOperands() > 1)
        Ops.push_back(Node->getOperand(1));
      unsigned RetOpc = Node->getNumOperands() > 1 ? TC32::TRET_R0 : TC32::TRET;
      ReplaceNode(Node, CurDAG->getMachineNode(RetOpc, DL, MVT::Other, Ops));
      return;
    }
    default:
      errs() << "Unhandled TC32 ISel node opcode " << Node->getOpcode() << "\n";
      Node->print(errs(), CurDAG);
      errs() << '\n';
      report_fatal_error("TC32 instruction selection is only implemented for empty void returns");
    }

    errs() << "Unhandled TC32 ISel node opcode " << Node->getOpcode() << "\n";
    Node->print(errs(), CurDAG);
    errs() << '\n';
    report_fatal_error("TC32 instruction selection does not handle this node yet");
  }
};

class TC32DAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;

  TC32DAGToDAGISelLegacy(TC32TargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISelLegacy(ID,
                               std::make_unique<TC32DAGToDAGISel>(TM, OptLevel)) {}

  StringRef getPassName() const override {
    return "TC32 DAG->DAG Pattern Instruction Selection";
  }
};

} // namespace

char TC32DAGToDAGISelLegacy::ID = 0;

FunctionPass *llvm::createTC32ISelDag(TC32TargetMachine &TM,
                                      CodeGenOptLevel OptLevel) {
  return new TC32DAGToDAGISelLegacy(TM, OptLevel);
}
