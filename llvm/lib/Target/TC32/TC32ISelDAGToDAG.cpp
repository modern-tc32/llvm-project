#include "TC32.h"
#include "TC32ISelLowering.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32TargetMachine.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;

namespace {

static bool matchBasePlusConst(SDValue Ptr, SDValue &Base, int64_t &Imm) {
  if (Ptr.getOpcode() != ISD::ADD)
    return false;
  if (auto *RC = dyn_cast<ConstantSDNode>(Ptr.getOperand(1))) {
    Base = Ptr.getOperand(0);
    Imm = RC->getSExtValue();
    return true;
  }
  if (auto *LC = dyn_cast<ConstantSDNode>(Ptr.getOperand(0))) {
    Base = Ptr.getOperand(1);
    Imm = LC->getSExtValue();
    return true;
  }
  return false;
}

static bool matchFrameIndexPlusConst(SDValue Ptr, int &FI, int64_t &Imm) {
  Imm = 0;
  if (Ptr.getOpcode() == ISD::FrameIndex || Ptr.getOpcode() == ISD::TargetFrameIndex) {
    FI = cast<FrameIndexSDNode>(Ptr)->getIndex();
    return true;
  }
  if (Ptr.getOpcode() != ISD::ADD)
    return false;

  auto match = [&](SDValue MaybeFI, SDValue MaybeConst) {
    if (MaybeFI.getOpcode() != ISD::FrameIndex &&
        MaybeFI.getOpcode() != ISD::TargetFrameIndex)
      return false;
    auto *CN = dyn_cast<ConstantSDNode>(MaybeConst);
    if (!CN)
      return false;
    FI = cast<FrameIndexSDNode>(MaybeFI)->getIndex();
    Imm = CN->getSExtValue();
    return true;
  };

  return match(Ptr.getOperand(0), Ptr.getOperand(1)) ||
         match(Ptr.getOperand(1), Ptr.getOperand(0));
}

static bool isCopyFromReg(SDValue V, unsigned Reg) {
  if (V.getOpcode() != ISD::CopyFromReg)
    return false;
  auto *R = dyn_cast<RegisterSDNode>(V.getOperand(1));
  return R && R->getReg() == Reg;
}

static bool matchSPPlusConst(SDValue Ptr, int64_t &Imm) {
  Imm = 0;
  if (isCopyFromReg(Ptr, TC32::R13))
    return true;
  if (Ptr.getOpcode() != ISD::ADD)
    return false;

  auto match = [&](SDValue MaybeSP, SDValue MaybeConst) {
    if (!isCopyFromReg(MaybeSP, TC32::R13))
      return false;
    auto *CN = dyn_cast<ConstantSDNode>(MaybeConst);
    if (!CN)
      return false;
    Imm = CN->getSExtValue();
    return true;
  };

  return match(Ptr.getOperand(0), Ptr.getOperand(1)) ||
         match(Ptr.getOperand(1), Ptr.getOperand(0));
}

static bool getFrameIndexReference(const SelectionDAG *DAG, int FI, int64_t ExtraImm,
                                   unsigned &BaseReg, int &Imm) {
  const MachineFrameInfo &MFI = DAG->getMachineFunction().getFrameInfo();
  bool Fixed = MFI.isFixedObjectIndex(FI);
  int Offset = MFI.getObjectOffset(FI);
  int StackSize = MFI.getStackSize();

  BaseReg = DAG->getSubtarget().getFrameLowering()->hasFP(
                DAG->getMachineFunction())
                ? TC32::R7
                : TC32::R13;
  Imm = (Fixed ? Offset : Offset + StackSize) + ExtraImm;
  if (DAG->getSubtarget().getFrameLowering()->hasFP(DAG->getMachineFunction()) &&
      Fixed) {
    BaseReg = TC32::R7;
    Imm += StackSize + 8;
  }
  return Imm >= 0 && Imm <= 255;
}

static bool getI8FrameIndexOperands(SelectionDAG *DAG, int FI, int64_t ExtraImm,
                                    SDValue &Base, SDValue &ImmVal, const SDLoc &DL) {
  if (ExtraImm < 0 || ExtraImm > 7)
    return false;
  Base = DAG->getTargetFrameIndex(FI, MVT::i32);
  ImmVal = DAG->getTargetConstant(ExtraImm, DL, MVT::i32);
  return true;
}

static SDValue stripBoolCasts(SDValue V) {
  while (true) {
    switch (V.getOpcode()) {
    case ISD::ZERO_EXTEND:
    case ISD::SIGN_EXTEND:
    case ISD::ANY_EXTEND:
    case ISD::TRUNCATE:
      V = V.getOperand(0);
      continue;
    default:
      return V;
    }
  }
}

static bool matchConstInt(SDValue V, uint64_t Value) {
  if (const auto *CN = dyn_cast<ConstantSDNode>(V))
    return CN->getZExtValue() == Value;
  return false;
}

static SDValue stripBoolCond(SDValue V, bool &Invert) {
  while (true) {
    switch (V.getOpcode()) {
    case ISD::ZERO_EXTEND:
    case ISD::SIGN_EXTEND:
    case ISD::ANY_EXTEND:
    case ISD::TRUNCATE:
      V = V.getOperand(0);
      continue;
    case ISD::AND:
      if (matchConstInt(V.getOperand(0), 1)) {
        V = V.getOperand(1);
        continue;
      }
      if (matchConstInt(V.getOperand(1), 1)) {
        V = V.getOperand(0);
        continue;
      }
      return V;
    case ISD::XOR:
      if (matchConstInt(V.getOperand(0), 1)) {
        Invert = !Invert;
        V = V.getOperand(1);
        continue;
      }
      if (matchConstInt(V.getOperand(1), 1)) {
        Invert = !Invert;
        V = V.getOperand(0);
        continue;
      }
      return V;
    default:
      return V;
    }
  }
}

static bool decodeBooleanSelectCC(SDValue V, SDValue &LHS, SDValue &RHS,
                                  ISD::CondCode &CCVal, bool &Invert) {
  V = stripBoolCasts(V);

  while (true) {
    switch (V.getOpcode()) {
    case ISD::ZERO_EXTEND:
    case ISD::SIGN_EXTEND:
    case ISD::ANY_EXTEND:
    case ISD::TRUNCATE:
      V = V.getOperand(0);
      continue;
    case ISD::XOR:
      if (matchConstInt(V.getOperand(0), 1)) {
        Invert = !Invert;
        V = V.getOperand(1);
        continue;
      }
      if (matchConstInt(V.getOperand(1), 1)) {
        Invert = !Invert;
        V = V.getOperand(0);
        continue;
      }
      return false;
    case ISD::AND:
      if (matchConstInt(V.getOperand(0), 1)) {
        V = V.getOperand(1);
        continue;
      }
      if (matchConstInt(V.getOperand(1), 1)) {
        V = V.getOperand(0);
        continue;
      }
      return false;
    default:
      break;
    }
    break;
  }

  if (V.getOpcode() == ISD::SETCC) {
    auto *CC = dyn_cast<CondCodeSDNode>(V.getOperand(2));
    if (!CC)
      return false;
    LHS = V.getOperand(0);
    RHS = V.getOperand(1);
    CCVal = CC->get();
    return true;
  }

  if (V.getOpcode() != ISD::SELECT_CC)
    return false;

  auto *CC = dyn_cast<CondCodeSDNode>(V.getOperand(4));
  if (!CC)
    return false;

  SDValue TrueVal = V.getOperand(2);
  SDValue FalseVal = V.getOperand(3);
  if (matchConstInt(TrueVal, 1) && matchConstInt(FalseVal, 0)) {
    LHS = V.getOperand(0);
    RHS = V.getOperand(1);
    CCVal = CC->get();
    return true;
  }
  if (matchConstInt(TrueVal, 0) && matchConstInt(FalseVal, 1)) {
    LHS = V.getOperand(0);
    RHS = V.getOperand(1);
    CCVal = CC->get();
    Invert = !Invert;
    return true;
  }

  return false;
}

static SDValue materializeConstant(SelectionDAG *DAG, const SDLoc &DL,
                                   uint32_t Value) {
  auto getImm = [&](uint32_t Imm) {
    return DAG->getTargetConstant(Imm, DL, MVT::i32);
  };

  auto constrainToLoRegClass = [&](SDValue V) {
    SDValue RC = DAG->getTargetConstant(TC32::LoGR32RegClassID, DL, MVT::i32);
    return SDValue(
        DAG->getMachineNode(TargetOpcode::COPY_TO_REGCLASS, DL, MVT::i32, V, RC),
        0);
  };

  if (Value <= 255)
    return constrainToLoRegClass(
        SDValue(DAG->getMachineNode(TC32::TMOVi8, DL, MVT::i32, getImm(Value)), 0));

  auto getMaterializationBytes = [&](uint32_t C) {
    SmallVector<uint8_t, 4> Bytes;
    for (int Shift = 24; Shift >= 0; Shift -= 8) {
      uint8_t Byte = static_cast<uint8_t>((C >> Shift) & 0xff);
      if (Bytes.empty() && Byte == 0)
        continue;
      Bytes.push_back(Byte);
    }
    if (Bytes.empty())
      Bytes.push_back(0);
    return Bytes;
  };

  SmallVector<uint8_t, 4> Bytes = getMaterializationBytes(Value);
  ArrayRef<uint8_t> ChosenBytes = Bytes;

  SDValue Cur = constrainToLoRegClass(SDValue(
      DAG->getMachineNode(TC32::TMOVi8, DL, MVT::i32, getImm(ChosenBytes[0])), 0));
  for (unsigned I = 1; I < ChosenBytes.size(); ++I) {
    Cur = SDValue(DAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, Cur, getImm(8)), 0);
    if (ChosenBytes[I] != 0)
      Cur = SDValue(
          DAG->getMachineNode(TC32::TADDSri8, DL, MVT::i32, Cur, getImm(ChosenBytes[I])), 0);
  }
  return Cur;
}

class TC32DAGToDAGISel : public SelectionDAGISel {
public:
  explicit TC32DAGToDAGISel(TC32TargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

  bool SelectFrameAddr(SDValue N, SDValue &Base, SDValue &Off) {
    int FI = 0;
    int64_t Imm = 0;
    if (!matchFrameIndexPlusConst(N, FI, Imm))
      return false;
    Base = CurDAG->getTargetFrameIndex(FI, MVT::i32);
    Off = CurDAG->getTargetConstant(Imm, SDLoc(N), MVT::i32);
    return true;
  }

  bool normalizeUnsignedCompare(const SDLoc &DL, ISD::CondCode &CCVal,
                                SDValue &LHS, SDValue &RHS) {
    ISD::CondCode SignedCC;
    switch (CCVal) {
    case ISD::SETULT:
      SignedCC = ISD::SETLT;
      break;
    case ISD::SETULE:
      SignedCC = ISD::SETLE;
      break;
    case ISD::SETUGT:
      SignedCC = ISD::SETGT;
      break;
    case ISD::SETUGE:
      SignedCC = ISD::SETGE;
      break;
    default:
      return false;
    }

    SDValue Bias = materializeConstant(CurDAG, DL, 0x80000000u);
    LHS = emitTwoAddressRR(TC32::TXORrr, DL, MVT::i32, LHS, Bias);
    RHS = emitTwoAddressRR(TC32::TXORrr, DL, MVT::i32, RHS, Bias);
    CCVal = SignedCC;
    return true;
  }

  static unsigned getBranchOpcodeForCond(ISD::CondCode CCVal) {
    switch (CCVal) {
    case ISD::SETEQ:
      return TC32::TJEQ;
    case ISD::SETNE:
      return TC32::TJNE;
    case ISD::SETUGE:
      return TC32::TJCS;
    case ISD::SETULT:
      return TC32::TJCC;
    case ISD::SETUGT:
      return TC32::TJHI;
    case ISD::SETULE:
      return TC32::TJLS;
    case ISD::SETGE:
      return TC32::TJGE;
    case ISD::SETLT:
      return TC32::TJLT;
    case ISD::SETGT:
      return TC32::TJGT;
    case ISD::SETLE:
      return TC32::TJLE;
    default:
      return 0;
    }
  }

  SDValue ensureValueInRegister(SDValue Value, const SDLoc &DL) {
    if (const auto *CN = dyn_cast<ConstantSDNode>(Value))
      return materializeConstant(CurDAG, DL, static_cast<uint32_t>(CN->getZExtValue()));
    return Value;
  }

  static bool isSignedCompare(ISD::CondCode CCVal) {
    switch (CCVal) {
    case ISD::SETLT:
    case ISD::SETLE:
    case ISD::SETGT:
    case ISD::SETGE:
      return true;
    default:
      return false;
    }
  }

  SDValue extendIntegerForCompare(SDValue Value, const SDLoc &DL,
                                  bool SignExtend) {
    EVT VT = Value.getValueType();
    if (VT == MVT::i32)
      return ensureValueInRegister(Value, DL);

    unsigned Bits = 0;
    if (VT == MVT::i1)
      Bits = 1;
    else if (VT == MVT::i8)
      Bits = 8;
    else if (VT == MVT::i16)
      Bits = 16;
    else
      return ensureValueInRegister(Value, DL);

    if (const auto *CN = dyn_cast<ConstantSDNode>(Value)) {
      uint64_t Raw = CN->getZExtValue();
      uint32_t ExtValue = 0;
      if (SignExtend) {
        switch (Bits) {
        case 1:
          ExtValue = static_cast<uint32_t>(static_cast<int32_t>(SignExtend64<1>(Raw)));
          break;
        case 8:
          ExtValue = static_cast<uint32_t>(static_cast<int32_t>(SignExtend64<8>(Raw)));
          break;
        case 16:
          ExtValue = static_cast<uint32_t>(static_cast<int32_t>(SignExtend64<16>(Raw)));
          break;
        default:
          llvm_unreachable("unexpected compare width");
        }
      } else {
        ExtValue = static_cast<uint32_t>(Raw & maskTrailingOnes<uint64_t>(Bits));
      }
      return materializeConstant(CurDAG, DL, ExtValue);
    }

    SDValue Wide = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, Value), 0);
    unsigned Shift = 32 - Bits;
    if (Shift == 0)
      return Wide;
    SDValue ShiftImm = CurDAG->getTargetConstant(Shift, DL, MVT::i32);
    SDValue Shl =
        SDValue(CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, Wide, ShiftImm), 0);
    unsigned ShrOpc = SignExtend ? TC32::TASRi5 : TC32::TSHFTRi5;
    return SDValue(CurDAG->getMachineNode(ShrOpc, DL, MVT::i32, Shl, ShiftImm), 0);
  }

  void prepareCompareOperands(const SDLoc &DL, ISD::CondCode CCVal,
                              SDValue &LHS, SDValue &RHS) {
    bool SignExtend = isSignedCompare(CCVal);
    LHS = extendIntegerForCompare(LHS, DL, SignExtend);
    RHS = extendIntegerForCompare(RHS, DL, SignExtend);
  }

  SDValue emitTwoAddressRR(unsigned Opcode, const SDLoc &DL, EVT VT, SDValue LHS,
                           SDValue RHS) {
    LHS = ensureValueInRegister(LHS, DL);
    RHS = ensureValueInRegister(RHS, DL);
    return SDValue(CurDAG->getMachineNode(Opcode, DL, VT, LHS, RHS), 0);
  }

  SDValue emitTwoAddressRRGlue(unsigned Opcode, const SDLoc &DL, EVT VT,
                               SDValue LHS, SDValue RHS, SDValue Glue) {
    LHS = ensureValueInRegister(LHS, DL);
    RHS = ensureValueInRegister(RHS, DL);
    return SDValue(CurDAG->getMachineNode(Opcode, DL, VT, LHS, RHS, Glue), 0);
  }

  SDValue emitByteLoad(SDValue Ptr, SDValue Chain, unsigned Offset,
                       const SDLoc &DL) {
    SDValue Base;
    int64_t Imm = 0;
    int FI = 0;
    if (matchFrameIndexPlusConst(Ptr, FI, Imm) && Imm + Offset >= 0 && Imm + Offset <= 7) {
      SDValue FIBase, FIImm;
      if (getI8FrameIndexOperands(CurDAG, FI, Imm + Offset, FIBase, FIImm, DL))
        return SDValue(CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                              {MVT::i32, MVT::Other},
                                              {FIBase, FIImm, Chain}),
                       0);
    }
    if (matchBasePlusConst(Ptr, Base, Imm) && Imm + Offset >= 0 && Imm + Offset <= 7) {
      return SDValue(CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                            {MVT::i32, MVT::Other},
                                            {Base,
                                             CurDAG->getTargetConstant(Imm + Offset, DL,
                                                                       MVT::i32),
                                             Chain}),
                     0);
    }

    if (Offset == 0) {
      return SDValue(CurDAG->getMachineNode(TC32::TLOADBrr, DL,
                                            {MVT::i32, MVT::Other},
                                            {Ptr, Chain}),
                     0);
    }

    return SDValue(CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                          {MVT::i32, MVT::Other},
                                          {Ptr, CurDAG->getTargetConstant(Offset, DL, MVT::i32),
                                           Chain}),
                   0);
  }

  SDValue emitByteStore(SDValue Value, SDValue Ptr, SDValue Chain, unsigned Offset,
                        const SDLoc &DL) {
    SDValue Base;
    int64_t Imm = 0;
    int FI = 0;
    if (matchFrameIndexPlusConst(Ptr, FI, Imm) && Imm + Offset >= 0 && Imm + Offset <= 7) {
      SDValue FIBase, FIImm;
      if (getI8FrameIndexOperands(CurDAG, FI, Imm + Offset, FIBase, FIImm, DL))
        return SDValue(CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other,
                                              {Value, FIBase, FIImm, Chain}),
                       0);
    }
    if (matchBasePlusConst(Ptr, Base, Imm) && Imm + Offset >= 0 && Imm + Offset <= 7) {
      return SDValue(CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other,
                                            {Value, Base,
                                             CurDAG->getTargetConstant(Imm + Offset, DL,
                                                                       MVT::i32),
                                             Chain}),
                     0);
    }

    if (Offset == 0) {
      return SDValue(CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                            {Value, Ptr, Chain}),
                     0);
    }

    return SDValue(CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other,
                                          {Value, Ptr,
                                           CurDAG->getTargetConstant(Offset, DL, MVT::i32),
                                           Chain}),
                   0);
  }

  bool trySelectUnalignedI32Load(SDNode *Node, LoadSDNode *LD, const SDLoc &DL) {
    if (Node->getValueType(0) != MVT::i32 || LD->getMemoryVT() != MVT::i32 ||
        LD->getAlign().value() >= 4 || LD->getBasePtr().getValueType() != MVT::i32)
      return false;

    SDValue Ptr = LD->getBasePtr();
    SDValue Load0 = emitByteLoad(Ptr, LD->getChain(), 0, DL);
    SDValue Load1 = emitByteLoad(Ptr, Load0.getValue(1), 1, DL);
    SDValue Load2 = emitByteLoad(Ptr, Load1.getValue(1), 2, DL);
    SDValue Load3 = emitByteLoad(Ptr, Load2.getValue(1), 3, DL);

    auto Imm = [&](unsigned Value) {
      return CurDAG->getTargetConstant(Value, DL, MVT::i32);
    };

    SDValue Byte1Shifted = SDValue(
        CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, SDValue(Load1.getNode(), 0),
                               Imm(8)),
        0);
    SDValue Byte2Shifted = SDValue(
        CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, SDValue(Load2.getNode(), 0),
                               Imm(16)),
        0);
    SDValue Byte3Shifted = SDValue(
        CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, SDValue(Load3.getNode(), 0),
                               Imm(24)),
        0);

    SDValue Acc = emitTwoAddressRR(TC32::TORrr, DL, MVT::i32, SDValue(Load0.getNode(), 0),
                                   Byte1Shifted);
    Acc = emitTwoAddressRR(TC32::TORrr, DL, MVT::i32, Acc, Byte2Shifted);
    Acc = emitTwoAddressRR(TC32::TORrr, DL, MVT::i32, Acc, Byte3Shifted);

    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, 0), Acc);
    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, 1), Load3.getValue(1));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool trySelectExtI16Load(SDNode *Node, LoadSDNode *LD, const SDLoc &DL) {
    if (Node->getValueType(0) != MVT::i32 || LD->getMemoryVT() != MVT::i16 ||
        (LD->getExtensionType() != ISD::ZEXTLOAD &&
         LD->getExtensionType() != ISD::EXTLOAD) ||
        LD->getBasePtr().getValueType() != MVT::i32)
      return false;

    SDValue Ptr = LD->getBasePtr();
    SDValue Load0 = emitByteLoad(Ptr, LD->getChain(), 0, DL);
    SDValue Load1 = emitByteLoad(Ptr, Load0.getValue(1), 1, DL);

    SDValue Byte1Shifted = SDValue(
        CurDAG->getMachineNode(TC32::TSHFTLi5, DL, MVT::i32, SDValue(Load1.getNode(), 0),
                               CurDAG->getTargetConstant(8, DL, MVT::i32)),
        0);
    SDValue Acc = emitTwoAddressRR(TC32::TORrr, DL, MVT::i32, SDValue(Load0.getNode(), 0),
                                   Byte1Shifted);

    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, 0), Acc);
    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, 1), Load1.getValue(1));
    CurDAG->RemoveDeadNode(Node);
    return true;
  }

  bool trySelectUnalignedI32Store(StoreSDNode *ST, const SDLoc &DL) {
    if (ST->getValue().getValueType() != MVT::i32 || ST->getMemoryVT() != MVT::i32 ||
        ST->getAlign().value() >= 4 || ST->getBasePtr().getValueType() != MVT::i32)
      return false;

    auto Imm = [&](unsigned Value) {
      return CurDAG->getTargetConstant(Value, DL, MVT::i32);
    };

    SDValue Value = ST->getValue();
    SDValue Byte1 = SDValue(CurDAG->getMachineNode(TC32::TSHFTRi5, DL, MVT::i32, Value,
                                                   Imm(8)),
                            0);
    SDValue Byte2 = SDValue(CurDAG->getMachineNode(TC32::TSHFTRi5, DL, MVT::i32, Value,
                                                   Imm(16)),
                            0);
    SDValue Byte3 = SDValue(CurDAG->getMachineNode(TC32::TSHFTRi5, DL, MVT::i32, Value,
                                                   Imm(24)),
                            0);

    SDValue Chain0 = emitByteStore(Value, ST->getBasePtr(), ST->getChain(), 0, DL);
    SDValue Chain1 = emitByteStore(Byte1, ST->getBasePtr(), Chain0, 1, DL);
    SDValue Chain2 = emitByteStore(Byte2, ST->getBasePtr(), Chain1, 2, DL);
    SDValue Chain3 = emitByteStore(Byte3, ST->getBasePtr(), Chain2, 3, DL);

    CurDAG->ReplaceAllUsesOfValueWith(SDValue(ST, 0), Chain3);
    CurDAG->RemoveDeadNode(ST);
    return true;
  }

  SDValue selectSetCCValue(SDLoc DL, ISD::CondCode CCVal, SDValue LHS,
                           SDValue RHS) {
    prepareCompareOperands(DL, CCVal, LHS, RHS);

    unsigned BrOpc = getBranchOpcodeForCond(CCVal);
    if (!BrOpc)
      return SDValue();

    SDValue FalseVal = materializeConstant(CurDAG, DL, 0);
    SDValue TrueVal = materializeConstant(CurDAG, DL, 1);
    SDValue Ops[] = {FalseVal, TrueVal, LHS, RHS,
                     CurDAG->getTargetConstant(BrOpc, DL, MVT::i32)};
    return SDValue(CurDAG->getMachineNode(TC32::TSELECTCC, DL, MVT::i32, Ops), 0);
  }

  SDValue trySelectCompareSignOrOne(const SDLoc &DL, EVT VT, SDValue LHS,
                                    SDValue RHS, SDValue TrueVal,
                                    SDValue FalseVal, ISD::CondCode CCVal) {
    if (VT != MVT::i32)
      return SDValue();

    auto matchConst = [](SDValue V, int32_t Value) {
      if (const auto *CN = dyn_cast<ConstantSDNode>(V))
        return CN->getSExtValue() == Value;
      return false;
    };

    bool ReverseOperands = false;
    switch (CCVal) {
    case ISD::SETULT:
      if (!matchConst(TrueVal, -1) || !matchConst(FalseVal, 1))
        return SDValue();
      break;
    case ISD::SETUGT:
      if (!matchConst(TrueVal, -1) || !matchConst(FalseVal, 1))
        return SDValue();
      ReverseOperands = true;
      break;
    default:
      return SDValue();
    }

    if (ReverseOperands)
      std::swap(LHS, RHS);

    LHS = extendIntegerForCompare(LHS, DL, false);
    RHS = extendIntegerForCompare(RHS, DL, false);

    LHS = ensureValueInRegister(LHS, DL);
    RHS = ensureValueInRegister(RHS, DL);

    SDValue Diff = SDValue(CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32,
                                                  LHS, RHS),
                           0);
    SDValue Sign =
        SDValue(CurDAG->getMachineNode(TC32::TASRi31, DL, MVT::i32, Diff), 0);
    return emitTwoAddressRR(TC32::TORrr, DL, MVT::i32, Sign,
                            materializeConstant(CurDAG, DL, 1));
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
    auto getTargetImm = [&](const SDValue &Op) -> SDValue {
      if (auto *CN = dyn_cast<ConstantSDNode>(Op))
        return CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i32);
      if (Op.getOpcode() == ISD::TargetConstant)
        return Op;
      report_fatal_error("TC32 expected constant call frame adjustment");
    };

    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }

    if (auto *LD = dyn_cast<LoadSDNode>(Node)) {
      if (trySelectUnalignedI32Load(Node, LD, DL))
        return;
      if (trySelectExtI16Load(Node, LD, DL))
        return;

      SDValue Base;
      int64_t Imm;
      int FI;
      if (Node->getValueType(0) == MVT::i32 && LD->getMemoryVT() == MVT::i8 &&
          (LD->getExtensionType() == ISD::ZEXTLOAD ||
           LD->getExtensionType() == ISD::EXTLOAD) &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        if (matchFrameIndexPlusConst(LD->getBasePtr(), FI, Imm)) {
          SDValue FIBase, FIImm;
          if (getI8FrameIndexOperands(CurDAG, FI, Imm, FIBase, FIImm, DL)) {
            ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                                     {MVT::i32, MVT::Other},
                                                     {FIBase, FIImm,
                                                      LD->getChain()}));
            return;
          }
        }
        if (matchBasePlusConst(LD->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 7) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                                   {MVT::i32, MVT::Other},
                                                   {Base,
                                                    CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                                    LD->getChain()}));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBrr, DL,
                                                 {MVT::i32, MVT::Other},
                                                 {LD->getBasePtr(),
                                                  LD->getChain()}));
        return;
      }
    }

    switch (Node->getOpcode()) {
    case ISD::EntryToken:
    case ISD::Register:
    case ISD::RegisterMask:
    case ISD::TokenFactor:
    case ISD::BasicBlock:
    case ISD::VALUETYPE:
    case ISD::CONDCODE:
    case ISD::TargetConstant:
    case ISD::TargetGlobalAddress:
    case ISD::TargetExternalSymbol:
      Node->setNodeId(-1);
      return;
    case ISD::CopyFromReg:
    case ISD::CopyToReg:
      Node->setNodeId(-1);
      return;
    case ISD::FAKE_USE:
      CurDAG->SelectNodeTo(Node, TargetOpcode::FAKE_USE, Node->getValueType(0),
                           Node->getOperand(1), Node->getOperand(0));
      return;
    case ISD::LIFETIME_START:
    case ISD::LIFETIME_END:
      ReplaceNode(Node, Node->getOperand(0).getNode());
      return;
    case ISD::CALLSEQ_START: {
      SDValue Ops[] = {getTargetImm(Node->getOperand(1)),
                       getTargetImm(Node->getOperand(2)), Node->getOperand(0)};
      ReplaceNode(Node, CurDAG->getMachineNode(TC32::ADJCALLSTACKDOWN, DL,
                                               {MVT::Other, MVT::Glue}, Ops));
      return;
    }
    case ISD::CALLSEQ_END: {
      SmallVector<SDValue, 4> Ops = {getTargetImm(Node->getOperand(1)),
                                     getTargetImm(Node->getOperand(2)),
                                     Node->getOperand(0)};
      if (Node->getNumOperands() > 3)
        Ops.push_back(Node->getOperand(3));
      ReplaceNode(Node, CurDAG->getMachineNode(TC32::ADJCALLSTACKUP, DL,
                                               {MVT::Other, MVT::Glue}, Ops));
      return;
    }
    case ISD::UNDEF:
      CurDAG->SelectNodeTo(Node, TargetOpcode::IMPLICIT_DEF,
                           Node->getValueType(0));
      return;
    case ISD::FREEZE:
      ReplaceNode(Node, Node->getOperand(0).getNode());
      return;
    case ISD::AssertSext:
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
    case ISD::ANY_EXTEND:
      if (Node->getValueType(0) == MVT::i32 &&
          (Node->getOperand(0).getValueType() == MVT::i8 ||
           Node->getOperand(0).getValueType() == MVT::i16)) {
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32,
                                                 Node->getOperand(0)));
        return;
      }
      break;
    case ISD::TRUNCATE:
      if (Node->getValueType(0) == MVT::i8 &&
          Node->getOperand(0).getValueType() == MVT::i32) {
        ReplaceNode(Node, Node->getOperand(0).getNode());
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
      ISD::CondCode CCVal = CC->get();

      prepareCompareOperands(DL, CCVal, LHS, RHS);
      unsigned BrOpc = getBranchOpcodeForCond(CCVal);
      if (BrOpc == 0)
        break;

      if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
        if (RC->isZero()) {
          Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
        } else if (RC->getSExtValue() == -1) {
          switch (CCVal) {
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
      SDValue Dest = Node->getOperand(2);
      bool Invert = false;
      SDValue Cond = stripBoolCasts(Node->getOperand(1));
      SDValue LHS;
      SDValue RHS;
      ISD::CondCode CCVal;

      if (decodeBooleanSelectCC(Node->getOperand(1), LHS, RHS, CCVal, Invert)) {
        MachineSDNode *Cmp = nullptr;
        if (Invert)
          CCVal = ISD::getSetCCInverse(CCVal, LHS.getValueType());

        prepareCompareOperands(DL, CCVal, LHS, RHS);
        unsigned BrOpc = getBranchOpcodeForCond(CCVal);
        if (BrOpc == 0)
          break;

        if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
          if (RC->isZero()) {
            Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
          } else if (RC->getSExtValue() == -1) {
            switch (CCVal) {
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

      bool RecoveredInvert = false;
      SDValue Recovered = stripBoolCond(Node->getOperand(1), RecoveredInvert);
      if (Recovered.getOpcode() == ISD::SETCC) {
        Cond = Recovered;
        Invert = RecoveredInvert;
      }
      if (Cond.getOpcode() != ISD::SETCC) {
        Cond = ensureValueInRegister(Cond, DL);
        MachineSDNode *Cmp =
            CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, Cond);
        unsigned BrOpc = Invert ? TC32::TJEQ : TC32::TJNE;
        ReplaceNode(Node, CurDAG->getMachineNode(BrOpc, DL, MVT::Other, Dest,
                                                 Chain, SDValue(Cmp, 0)));
        return;
      }

      auto *CC = dyn_cast<CondCodeSDNode>(Cond.getOperand(2));
      if (!CC)
        break;

      MachineSDNode *Cmp = nullptr;
      LHS = Cond.getOperand(0);
      RHS = Cond.getOperand(1);
      CCVal = CC->get();
      if (Invert)
        CCVal = ISD::getSetCCInverse(CCVal, Cond.getOperand(0).getValueType());

      prepareCompareOperands(DL, CCVal, LHS, RHS);
      unsigned BrOpc = getBranchOpcodeForCond(CCVal);
      if (BrOpc == 0)
        break;

      if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
        if (RC->isZero()) {
          Cmp = CurDAG->getMachineNode(TC32::TCMPri0, DL, MVT::Glue, LHS);
        } else if (RC->getSExtValue() == -1) {
          switch (CCVal) {
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
      if ((Node->getValueType(0) == MVT::i32 || Node->getValueType(0) == MVT::i8) && CN) {
        SDValue Val = materializeConstant(CurDAG, DL,
                                          static_cast<uint32_t>(CN->getZExtValue()));
        if (Node->getValueType(0) == MVT::i8)
          ReplaceNode(
              Node,
              CurDAG->getMachineNode(
                  TargetOpcode::COPY_TO_REGCLASS, DL, MVT::i8,
                  SDValue(CurDAG->getMachineNode(
                              TC32::TMOVi8, DL, MVT::i8,
                              CurDAG->getTargetConstant(CN->getZExtValue() & 0xff,
                                                        DL, MVT::i32)),
                          0),
                  CurDAG->getTargetConstant(TC32::LoGR32RegClassID, DL, MVT::i32)));
        else
          ReplaceNode(Node, Val.getNode());
        return;
      }
      break;
    }
    case TC32ISD::FRAMEADDR: {
      auto *FIN = dyn_cast<ConstantSDNode>(Node->getOperand(0));
      auto *OffN = dyn_cast<ConstantSDNode>(Node->getOperand(1));
      if (!FIN || !OffN)
        break;
      SDValue FI = CurDAG->getTargetConstant(FIN->getSExtValue(), DL, MVT::i32);
      SDValue Off = CurDAG->getTargetConstant(OffN->getSExtValue(), DL, MVT::i32);
      ReplaceNode(Node, CurDAG->getMachineNode(TC32::TFRAMEADDR, DL, MVT::i32, FI,
                                               Off));
      return;
    }
    case ISD::GlobalAddress: {
      auto *GA = cast<GlobalAddressSDNode>(Node);
      SDValue Addr = CurDAG->getTargetGlobalAddress(GA->getGlobal(), DL, MVT::i32,
                                                    GA->getOffset());
      CurDAG->SelectNodeTo(Node, TC32::TLOADaddr, MVT::i32, Addr);
      return;
    }
    case ISD::ExternalSymbol: {
      auto *ES = cast<ExternalSymbolSDNode>(Node);
      SDValue Addr = CurDAG->getTargetExternalSymbol(ES->getSymbol(), MVT::i32);
      CurDAG->SelectNodeTo(Node, TC32::TLOADaddr, MVT::i32, Addr);
      return;
    }
    case ISD::SETCC: {
      if (Node->hasOneUse()) {
        SDNode *User = (*Node->use_begin()).getUser();
        switch (User->getOpcode()) {
        case ISD::SELECT:
        case ISD::SELECT_CC:
        case ISD::BRCOND:
        case ISD::BR_CC:
          break;
        default:
          User = nullptr;
          break;
        }
        if (User)
          break;
      }
      if (SDValue Res = selectSetCC(Node)) {
        ReplaceNode(Node, Res.getNode());
        return;
      }
      break;
    }
    case ISD::SELECT_CC: {
      EVT VT = Node->getValueType(0);
      if (VT != MVT::i32 && VT != MVT::i8)
        break;

      auto *CC = dyn_cast<CondCodeSDNode>(Node->getOperand(4));
      if (!CC)
        break;

      SDValue LHS = Node->getOperand(0);
      SDValue RHS = Node->getOperand(1);
      SDValue TrueVal = Node->getOperand(2);
      SDValue FalseVal = Node->getOperand(3);
      ISD::CondCode CCVal = CC->get();

      if (SDValue Res = trySelectCompareSignOrOne(DL, VT, LHS, RHS, TrueVal,
                                                  FalseVal, CCVal)) {
        ReplaceNode(Node, Res.getNode());
        return;
      }

      prepareCompareOperands(DL, CCVal, LHS, RHS);

      unsigned BrOpc = getBranchOpcodeForCond(CCVal);
      if (!BrOpc)
        break;

      auto materializeValue = [&](SDValue V) -> SDValue {
        if (auto *CN = dyn_cast<ConstantSDNode>(V))
          return materializeConstant(CurDAG, DL,
                                     static_cast<uint32_t>(CN->getZExtValue()));
        if (V.getValueType() == MVT::i8)
          return SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, V), 0);
        return ensureValueInRegister(V, DL);
      };

      TrueVal = materializeValue(TrueVal);
      FalseVal = materializeValue(FalseVal);

      SDValue Ops[] = {FalseVal, TrueVal, LHS, RHS,
                       CurDAG->getTargetConstant(BrOpc, DL, MVT::i32)};
      SDValue Res32 =
          SDValue(CurDAG->getMachineNode(TC32::TSELECTCC, DL, MVT::i32, Ops), 0);
      if (VT == MVT::i8)
        Res32 = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i8, Res32), 0);
      ReplaceNode(Node, Res32.getNode());
      return;
    }
    case ISD::SELECT: {
      EVT VT = Node->getValueType(0);
      if (VT != MVT::i32 && VT != MVT::i8)
        break;

      bool Invert = false;
      SDValue Cond = stripBoolCasts(Node->getOperand(0));
      SDValue LHS;
      SDValue RHS;
      ISD::CondCode CCVal = ISD::SETNE;
      bool RecoveredInvert = false;
      SDValue Recovered = stripBoolCond(Node->getOperand(0), RecoveredInvert);
      if (decodeBooleanSelectCC(Node->getOperand(0), LHS, RHS, CCVal, Invert)) {
        Cond = SDValue();
      } else if (Recovered.getOpcode() == ISD::SETCC) {
        Cond = Recovered;
        Invert = RecoveredInvert;
      }
      SDValue TrueVal = Node->getOperand(1);
      SDValue FalseVal = Node->getOperand(2);

      unsigned BrOpc = 0;

      if (LHS.getNode()) {
        if (SDValue Res = trySelectCompareSignOrOne(DL, VT, LHS, RHS, TrueVal,
                                                    FalseVal, CCVal)) {
          ReplaceNode(Node, Res.getNode());
          return;
        }
        if (Invert)
          CCVal = ISD::getSetCCInverse(CCVal, LHS.getValueType());
        prepareCompareOperands(DL, CCVal, LHS, RHS);
        BrOpc = getBranchOpcodeForCond(CCVal);
      } else if (Cond.getOpcode() == ISD::SETCC) {
        auto *CC = dyn_cast<CondCodeSDNode>(Cond.getOperand(2));
        if (!CC)
          break;
        CCVal = CC->get();
        if (Invert)
          CCVal = ISD::getSetCCInverse(CCVal, Cond.getOperand(0).getValueType());
        LHS = Cond.getOperand(0);
        RHS = Cond.getOperand(1);
        if (SDValue Res = trySelectCompareSignOrOne(DL, VT, LHS, RHS, TrueVal,
                                                    FalseVal, CCVal)) {
          ReplaceNode(Node, Res.getNode());
          return;
        }
        prepareCompareOperands(DL, CCVal, LHS, RHS);
        BrOpc = getBranchOpcodeForCond(CCVal);
      } else {
        if (Cond.getValueType() == MVT::i1)
          Cond = CurDAG->getNode(ISD::ZERO_EXTEND, DL, MVT::i32, Cond);
        else if (Cond.getValueType() == MVT::i8)
          Cond = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, Cond), 0);
        else
          Cond = ensureValueInRegister(Cond, DL);
        LHS = Cond;
        RHS = materializeConstant(CurDAG, DL, 0);
        BrOpc = Invert ? TC32::TJEQ : TC32::TJNE;
      }

      if (!BrOpc)
        break;

      auto materializeValue = [&](SDValue V) -> SDValue {
        if (auto *CN = dyn_cast<ConstantSDNode>(V))
          return materializeConstant(CurDAG, DL,
                                     static_cast<uint32_t>(CN->getZExtValue()));
        if (V.getValueType() == MVT::i8)
          return SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, V), 0);
        return ensureValueInRegister(V, DL);
      };

      TrueVal = materializeValue(TrueVal);
      FalseVal = materializeValue(FalseVal);

      SDValue Ops[] = {FalseVal, TrueVal, LHS, RHS,
                       CurDAG->getTargetConstant(BrOpc, DL, MVT::i32)};
      SDValue Res32 =
          SDValue(CurDAG->getMachineNode(TC32::TSELECTCC, DL, MVT::i32, Ops), 0);
      if (VT == MVT::i8)
        Res32 = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i8, Res32), 0);
      ReplaceNode(Node, Res32.getNode());
      return;
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
        SDValue Bit =
            emitTwoAddressRR(TC32::TANDrr, DL, MVT::i32, Node->getOperand(0), One);
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBrrr, DL, MVT::i32,
                                                 Zero, Bit));
        return;
      }
      if (Node->getValueType(0) == MVT::i32 &&
          cast<VTSDNode>(Node->getOperand(1))->getVT() == MVT::i16) {
        SDValue Shl = SDValue(CurDAG->getMachineNode(
                                  TC32::TSHFTLi5, DL, MVT::i32,
                                  Node->getOperand(0),
                                  CurDAG->getTargetConstant(16, DL, MVT::i32)),
                              0);
        ReplaceNode(Node, CurDAG->getMachineNode(
                              TC32::TASRi5, DL, MVT::i32, Shl,
                              CurDAG->getTargetConstant(16, DL, MVT::i32)));
        return;
      }
      break;
    }
    case ISD::ADD: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        if (auto *RC = dyn_cast<ConstantSDNode>(RHS); RC && RC->getSExtValue() == -1) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBri1, DL, VT, LHS));
          return;
        }
        if (auto *LC = dyn_cast<ConstantSDNode>(LHS); LC && LC->getSExtValue() == -1) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBri1, DL, VT, RHS));
          return;
        }
        LHS = ensureValueInRegister(LHS, DL);
        RHS = ensureValueInRegister(RHS, DL);
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TADDSrrr, DL, VT, LHS, RHS));
        return;
      }
      break;
    }
    case ISD::SUB: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        LHS = ensureValueInRegister(LHS, DL);
        RHS = ensureValueInRegister(RHS, DL);
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSUBrrr, DL, VT, LHS, RHS));
        return;
      }
      break;
    }
    case ISD::AND: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);

        auto foldNestedConstAnd = [&](SDValue &Value, SDValue &Mask) {
          auto *OuterC = dyn_cast<ConstantSDNode>(Mask);
          if (!OuterC || Value.getOpcode() != ISD::AND)
            return false;

          SDValue InnerLHS = Value.getOperand(0);
          SDValue InnerRHS = Value.getOperand(1);
          auto *InnerCRHS = dyn_cast<ConstantSDNode>(InnerRHS);
          auto *InnerCLHS = dyn_cast<ConstantSDNode>(InnerLHS);
          const ConstantSDNode *InnerC = InnerCRHS ? InnerCRHS : InnerCLHS;
          if (!InnerC)
            return false;

          SDValue InnerValue = InnerCRHS ? InnerLHS : InnerRHS;
          uint64_t FoldedMask = OuterC->getZExtValue() & InnerC->getZExtValue();
          Value = InnerValue;
          Mask = CurDAG->getConstant(FoldedMask, DL, VT);
          return true;
        };

        foldNestedConstAnd(LHS, RHS);
        foldNestedConstAnd(RHS, LHS);
        ReplaceNode(Node, emitTwoAddressRR(TC32::TANDrr, DL, VT, LHS, RHS).getNode());
        return;
      }
      break;
    }
    case ISD::OR: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        ReplaceNode(Node, emitTwoAddressRR(TC32::TORrr, DL, VT, LHS, RHS).getNode());
        return;
      }
      break;
    }
    case ISD::MUL: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        ReplaceNode(Node, emitTwoAddressRR(TC32::TMULrr, DL, VT, LHS, RHS).getNode());
        return;
      }
      break;
    }
    case ISD::XOR: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        SDValue LHS = Node->getOperand(0);
        SDValue RHS = Node->getOperand(1);
        ReplaceNode(Node, emitTwoAddressRR(TC32::TXORrr, DL, VT, LHS, RHS).getNode());
        return;
      }
      break;
    }
    case ISD::SIGN_EXTEND: {
      EVT VT = Node->getValueType(0);
      EVT SrcVT = Node->getOperand(0).getValueType();
      if (VT == MVT::i32 && (SrcVT == MVT::i8 || SrcVT == MVT::i16)) {
        unsigned Shift = SrcVT == MVT::i8 ? 24 : 16;
        SDValue Value = Node->getOperand(0);
        if (SrcVT != MVT::i32)
          Value = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, Value), 0);
        SDValue Shl = SDValue(CurDAG->getMachineNode(
                                  TC32::TSHFTLi5, DL, MVT::i32, Value,
                                  CurDAG->getTargetConstant(Shift, DL, MVT::i32)),
                              0);
        ReplaceNode(Node, CurDAG->getMachineNode(
                              TC32::TASRi5, DL, MVT::i32, Shl,
                              CurDAG->getTargetConstant(Shift, DL, MVT::i32)));
        return;
      }
      break;
    }
    case ISD::SHL: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTLi5, DL, VT,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
        if (VT == MVT::i32) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TVARSHL, DL, VT,
                                                   Node->getOperand(0),
                                                   ensureValueInRegister(Node->getOperand(1), DL)));
          return;
        }
      }
      break;
    }
    case ISD::SRL: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() == 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTRi31, DL, VT,
                                                   Node->getOperand(0)));
          return;
        }
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSHFTRi5, DL, VT,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
        if (VT == MVT::i32) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TVARSRL, DL, VT,
                                                   Node->getOperand(0),
                                                   ensureValueInRegister(Node->getOperand(1), DL)));
          return;
        }
      }
      break;
    }
    case ISD::SRA: {
      EVT VT = Node->getValueType(0);
      if (VT == MVT::i32 || VT == MVT::i8) {
        auto *RC = dyn_cast<ConstantSDNode>(Node->getOperand(1));
        if (RC && RC->getZExtValue() == 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TASRi31, DL, VT,
                                                   Node->getOperand(0)));
          return;
        }
        if (RC && RC->getZExtValue() <= 31) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TASRi5, DL, VT,
                                                   Node->getOperand(0),
                                                   CurDAG->getTargetConstant(
                                                       RC->getZExtValue(), DL, MVT::i32)));
          return;
        }
        if (VT == MVT::i32) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TVARSRA, DL, VT,
                                                   Node->getOperand(0),
                                                   ensureValueInRegister(Node->getOperand(1), DL)));
          return;
        }
      }
      break;
    }
    case ISD::LOAD: {
      auto *LD = cast<LoadSDNode>(Node);
      SDValue Base;
      int64_t Imm;
      int FI;
      if (Node->getValueType(0) == MVT::i32 && LD->getMemoryVT() == MVT::i32 &&
          matchSPPlusConst(LD->getBasePtr(), Imm) &&
          Imm >= 0 && Imm <= 1020 && (Imm & 3) == 0) {
        ReplaceNode(Node, CurDAG->getMachineNode(
                              TC32::TLOADspu8, DL, {MVT::i32, MVT::Other},
                              {CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                               LD->getChain()}));
        return;
      }
      if (Node->getValueType(0) == MVT::i8 && LD->getMemoryVT() == MVT::i8 &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        if (matchFrameIndexPlusConst(LD->getBasePtr(), FI, Imm)) {
          SDValue FIBase, FIImm;
          if (getI8FrameIndexOperands(CurDAG, FI, Imm, FIBase, FIImm, DL)) {
            ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                                     {MVT::i8, MVT::Other},
                                                     {FIBase, FIImm,
                                                      LD->getChain()}));
            return;
          }
        }
        if (matchBasePlusConst(LD->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 7) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBru3, DL,
                                                   {MVT::i8, MVT::Other},
                                                   {Base,
                                                    CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                                    LD->getChain()}));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADBrr, DL,
                                                 {MVT::i8, MVT::Other},
                                                 {LD->getBasePtr(),
                                                  LD->getChain()}));
        return;
      }
      if (Node->getValueType(0) == MVT::i32 && LD->getMemoryVT() == MVT::i32 &&
          LD->getBasePtr().getValueType() == MVT::i32) {
        if (matchFrameIndexPlusConst(LD->getBasePtr(), FI, Imm) &&
            Imm >= 0 && Imm <= 28 && (Imm & 3) == 0) {
          SDValue FIBase = CurDAG->getTargetFrameIndex(FI, MVT::i32);
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADru3, DL,
                                                   {MVT::i32, MVT::Other},
                                                   {FIBase,
                                                    CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                                    LD->getChain()}));
          return;
        }
        if (LD->getBasePtr().getOpcode() == ISD::FrameIndex) {
          int DirectFI = cast<FrameIndexSDNode>(LD->getBasePtr())->getIndex();
          SDValue FIBase = CurDAG->getTargetFrameIndex(DirectFI, MVT::i32);
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADrr, DL,
                                                   {MVT::i32, MVT::Other},
                                                   {FIBase, LD->getChain()}));
          return;
        }
        if (matchBasePlusConst(LD->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 28 && (Imm & 3) == 0) {
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADru3, DL,
                                                   {MVT::i32, MVT::Other},
                                                   {Base,
                                                    CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                                    LD->getChain()}));
          return;
        }
        Base = LD->getBasePtr();
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TLOADrr, DL,
                                                 {MVT::i32, MVT::Other},
                                                 {Base, LD->getChain()}));
        return;
      }
      break;
    }
    case ISD::STORE: {
      auto *ST = cast<StoreSDNode>(Node);
      if (trySelectUnalignedI32Store(ST, DL))
        return;

      SDValue Base;
      int64_t Imm;
      int FI;
      if (ST->getMemoryVT() == MVT::i16 &&
          (ST->getValue().getValueType() == MVT::i16 ||
           ST->getValue().getValueType() == MVT::i32) &&
          ST->getBasePtr().getValueType() == MVT::i32) {
        SDValue Value32 = ST->getValue();
        if (Value32.getValueType() == MVT::i16)
          Value32 = SDValue(CurDAG->getMachineNode(TC32::TMOVrr, DL, MVT::i32, Value32), 0);

        SDValue LowByte = Value32;
        SDValue HighByte =
            SDValue(CurDAG->getMachineNode(TC32::TSHFTRi5, DL, MVT::i32, Value32,
                                          CurDAG->getTargetConstant(8, DL, MVT::i32)),
                    0);

        if (matchBasePlusConst(ST->getBasePtr(), Base, Imm) && Imm >= 0 && Imm <= 6) {
          SmallVector<SDValue, 4> FirstOps = {
              LowByte, Base, CurDAG->getTargetConstant(Imm, DL, MVT::i32), ST->getChain()};
          SDValue First =
              SDValue(CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other, FirstOps), 0);
          SmallVector<SDValue, 4> SecondOps = {
              HighByte, Base, CurDAG->getTargetConstant(Imm + 1, DL, MVT::i32), First};
          ReplaceNode(Node,
                      CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other, SecondOps));
          return;
        }

        SDValue First = SDValue(
            CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                   LowByte, ST->getBasePtr(), ST->getChain()),
            0);
        SmallVector<SDValue, 4> SecondOps = {HighByte, ST->getBasePtr(),
                                             CurDAG->getTargetConstant(1, DL, MVT::i32), First};
        ReplaceNode(Node,
                    CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other, SecondOps));
        return;
      }
      if (ST->getValue().getValueType() == MVT::i32 &&
          ST->getMemoryVT() == MVT::i32 && matchSPPlusConst(ST->getBasePtr(), Imm) &&
          Imm >= 0 && Imm <= 1020 && (Imm & 3) == 0) {
        ReplaceNode(Node, CurDAG->getMachineNode(
                              TC32::TSTOREspu8, DL, MVT::Other, ST->getValue(),
                              CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                              ST->getChain()));
        return;
      }
      if (ST->getMemoryVT() == MVT::i8 &&
          ST->getBasePtr().getValueType() == MVT::i32 &&
          ST->getValue().getValueType() == MVT::i8) {
        if (matchFrameIndexPlusConst(ST->getBasePtr(), FI, Imm)) {
          SDValue FIBase, FIImm;
          if (getI8FrameIndexOperands(CurDAG, FI, Imm, FIBase, FIImm, DL)) {
            SmallVector<SDValue, 4> Ops = {ST->getValue(), FIBase, FIImm,
                                           ST->getChain()};
            ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBru3, DL,
                                                     MVT::Other, Ops));
            return;
          }
        }
        if (matchBasePlusConst(ST->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 7) {
          SmallVector<SDValue, 4> Ops = {ST->getValue(), Base,
                                         CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                         ST->getChain()};
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other, Ops));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
      }
      if (ST->getMemoryVT() == MVT::i8 &&
          ST->getBasePtr().getValueType() == MVT::i32 &&
          ST->getValue().getValueType() == MVT::i32) {
        if (matchFrameIndexPlusConst(ST->getBasePtr(), FI, Imm)) {
          SDValue FIBase, FIImm;
          if (getI8FrameIndexOperands(CurDAG, FI, Imm, FIBase, FIImm, DL)) {
            SmallVector<SDValue, 4> Ops = {ST->getValue(), FIBase, FIImm,
                                           ST->getChain()};
            ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBru3, DL,
                                                     MVT::Other, Ops));
            return;
          }
        }
        if (matchBasePlusConst(ST->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 7) {
          SmallVector<SDValue, 4> Ops = {ST->getValue(), Base,
                                         CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                         ST->getChain()};
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBru3, DL, MVT::Other, Ops));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREBrr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
      }
      if (ST->getValue().getValueType() == MVT::i32 &&
          ST->getMemoryVT() == MVT::i32 &&
          ST->getBasePtr().getValueType() == MVT::i32) {
        if (matchFrameIndexPlusConst(ST->getBasePtr(), FI, Imm) &&
            Imm >= 0 && Imm <= 28 && (Imm & 3) == 0) {
          SDValue FIBase = CurDAG->getTargetFrameIndex(FI, MVT::i32);
          SmallVector<SDValue, 4> Ops = {ST->getValue(), FIBase,
                                         CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                         ST->getChain()};
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREru3, DL, MVT::Other, Ops));
          return;
        }
        if (ST->getBasePtr().getOpcode() == ISD::FrameIndex) {
          int DirectFI = cast<FrameIndexSDNode>(ST->getBasePtr())->getIndex();
          SDValue FIBase = CurDAG->getTargetFrameIndex(DirectFI, MVT::i32);
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTORErr, DL, MVT::Other,
                                                   ST->getValue(), FIBase,
                                                   ST->getChain()));
          return;
        }
        if (matchBasePlusConst(ST->getBasePtr(), Base, Imm) &&
            Imm >= 0 && Imm <= 28 && (Imm & 3) == 0) {
          SmallVector<SDValue, 4> Ops = {ST->getValue(), Base,
                                         CurDAG->getTargetConstant(Imm, DL, MVT::i32),
                                         ST->getChain()};
          ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTOREru3, DL, MVT::Other, Ops));
          return;
        }
        ReplaceNode(Node, CurDAG->getMachineNode(TC32::TSTORErr, DL, MVT::Other,
                                                 ST->getValue(), ST->getBasePtr(),
                                                 ST->getChain()));
        return;
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
      if (Node->getOpcode() == ISD::BUILD_VECTOR) {
        errs() << "Full BUILD_VECTOR context:\n";
        Node->printrFull(errs(), CurDAG);
        errs() << '\n';
      }
      report_fatal_error("TC32 instruction selection is only implemented for empty void returns");
    }
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
