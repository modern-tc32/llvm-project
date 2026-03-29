#include "TC32ISelLowering.h"
#include "TC32InstrInfo.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32Subtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

TC32TargetLowering::TC32TargetLowering(const TargetMachine &TM,
                                       const TC32Subtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &TC32::LoGR32RegClass);
  addRegisterClass(MVT::i8, &TC32::LoGR32RegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(TC32::R13);
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setMinFunctionAlignment(Align(2));
  setPrefFunctionAlignment(Align(2));
  setMaxAtomicSizeInBitsSupported(0);
  setOperationAction(ISD::Constant, MVT::i32, Legal);
  setOperationAction(ISD::Constant, MVT::i8, Legal);
  setOperationAction(ISD::LOAD, MVT::i32, Custom);
  setOperationAction(ISD::STORE, MVT::i32, Custom);
  setOperationAction(ISD::ADD, MVT::i32, Legal);
  setOperationAction(ISD::ADD, MVT::i8, Legal);
  setOperationAction(ISD::SUB, MVT::i32, Legal);
  setOperationAction(ISD::SUB, MVT::i8, Legal);
  setOperationAction(ISD::AND, MVT::i32, Legal);
  setOperationAction(ISD::AND, MVT::i8, Legal);
  setOperationAction(ISD::OR, MVT::i32, Legal);
  setOperationAction(ISD::OR, MVT::i8, Legal);
  setOperationAction(ISD::XOR, MVT::i32, Legal);
  setOperationAction(ISD::XOR, MVT::i8, Legal);
  setOperationAction(ISD::MUL, MVT::i32, Legal);
  setOperationAction(ISD::MUL, MVT::i8, Legal);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i8, Promote);
  setOperationAction(ISD::MULHS, MVT::i8, Promote);
  setOperationAction(ISD::UMUL_LOHI, MVT::i8, Promote);
  setOperationAction(ISD::SMUL_LOHI, MVT::i8, Promote);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::SHL, MVT::i32, Legal);
  setOperationAction(ISD::SHL, MVT::i8, Custom);
  setOperationAction(ISD::SRL, MVT::i32, Legal);
  setOperationAction(ISD::SRL, MVT::i8, Custom);
  setOperationAction(ISD::SRA, MVT::i32, Legal);
  setOperationAction(ISD::SRA, MVT::i8, Custom);
  setOperationAction(ISD::SDIV, MVT::i32, LibCall);
  setOperationAction(ISD::UDIV, MVT::i32, LibCall);
  setOperationAction(ISD::SREM, MVT::i32, LibCall);
  setOperationAction(ISD::UREM, MVT::i32, LibCall);
  setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::SDIV, MVT::i8, Promote);
  setOperationAction(ISD::UDIV, MVT::i8, Promote);
  setOperationAction(ISD::SREM, MVT::i8, Promote);
  setOperationAction(ISD::UREM, MVT::i8, Promote);
  setOperationAction(ISD::SDIVREM, MVT::i8, Promote);
  setOperationAction(ISD::UDIVREM, MVT::i8, Promote);
  auto setBuiltinLibcall = [&](RTLIB::Libcall Call, StringRef Name) {
    for (RTLIB::LibcallImpl Impl :
         RTLIB::RuntimeLibcallsInfo::lookupLibcallImplName(Name)) {
      setLibcallImpl(Call, Impl);
      return;
    }
  };
  setBuiltinLibcall(RTLIB::SDIV_I32, "__divsi3");
  setBuiltinLibcall(RTLIB::UDIV_I32, "__udivsi3");
  setBuiltinLibcall(RTLIB::SREM_I32, "__modsi3");
  setBuiltinLibcall(RTLIB::UREM_I32, "__umodsi3");
  setBuiltinLibcall(RTLIB::SHL_I32, "__ashlsi3");
  setBuiltinLibcall(RTLIB::MEMSET, "memset");
  setBuiltinLibcall(RTLIB::MEMCPY, "memcpy");
  setBuiltinLibcall(RTLIB::MEMMOVE, "memmove");
  setBuiltinLibcall(RTLIB::MEMCMP, "memcmp");
  setBuiltinLibcall(RTLIB::BCMP, "bcmp");
  MaxStoresPerMemset = 8;
  MaxStoresPerMemsetOptSize = 8;
  MaxStoresPerMemcpy = 8;
  MaxStoresPerMemcpyOptSize = 8;
  MaxStoresPerMemmove = 8;
  MaxStoresPerMemmoveOptSize = 8;
  MaxLoadsPerMemcmp = 0;
  MaxLoadsPerMemcmpOptSize = 0;
  setOperationAction(ISD::AssertSext, MVT::i32, Legal);
  setOperationAction(ISD::AssertZext, MVT::i32, Legal);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i8, Custom);
  setOperationAction(ISD::SELECT, MVT::i32, Custom);
  setOperationAction(ISD::SELECT, MVT::i8, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::i8, Legal);
  setOperationAction(ISD::BRCOND, MVT::Other, Legal);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::i8, Custom);
  setOperationAction(ISD::FrameIndex, MVT::i32, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i8, MVT::i1, Promote);
  setLoadExtAction(ISD::EXTLOAD, MVT::i8, MVT::i1, Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i8, Legal);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i8, Expand);
  setLoadExtAction(ISD::EXTLOAD, MVT::i32, MVT::i8, Expand);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i16, Legal);
  setLoadExtAction(ISD::EXTLOAD, MVT::i32, MVT::i16, Legal);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i16, Expand);
  setTruncStoreAction(MVT::i32, MVT::i16, Legal);
  setTruncStoreAction(MVT::i32, MVT::i8, Legal);
}

static SDValue normalizeIncomingScalarArgI32(SelectionDAG &DAG, const SDLoc &DL,
                                             SDValue Arg, EVT VT,
                                             const ISD::ArgFlagsTy &Flags) {
  if (VT != MVT::i8 && VT != MVT::i16)
    return Arg;

  if (!Flags.isSExt()) {
    uint32_t Mask = VT == MVT::i8 ? 0xffu : 0xffffu;
    return DAG.getNode(ISD::AND, DL, MVT::i32, Arg,
                       DAG.getConstant(Mask, DL, MVT::i32));
  }

  unsigned Shift = VT == MVT::i8 ? 24 : 16;
  SDValue ShiftAmt = DAG.getConstant(Shift, DL, MVT::i32);
  Arg = DAG.getNode(ISD::SHL, DL, MVT::i32, Arg, ShiftAmt);
  return DAG.getNode(ISD::SRA, DL, MVT::i32, Arg, ShiftAmt);
}

const char *TC32TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case TC32ISD::FRAMEADDR:
    return "TC32ISD::FRAMEADDR";
  case TC32ISD::CALL:
    return "TC32ISD::CALL";
  case TC32ISD::RET_FLAG:
    return "TC32ISD::RET_FLAG";
  default:
    return nullptr;
  }
}

static SDValue lowerI8Shift(SDValue Op, SelectionDAG &DAG) {
  SDLoc DL(Op);
  const unsigned Opcode = Op.getOpcode();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);

  auto ConstI8 = [&](unsigned Value) {
    return DAG.getConstant(Value & 0xffu, DL, MVT::i8);
  };
  auto ConstI32 = [&](unsigned Value) {
    return DAG.getConstant(Value, DL, MVT::i32);
  };

  SDValue LHS32 = Opcode == ISD::SRA
                      ? DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, LHS)
                      : DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, LHS);
  auto emitShift = [&](unsigned Shift) {
    SDValue Shifted32 = DAG.getNode(Opcode, DL, MVT::i32, LHS32, ConstI32(Shift));
    return DAG.getNode(ISD::TRUNCATE, DL, MVT::i8, Shifted32);
  };

  if (auto *RC = dyn_cast<ConstantSDNode>(RHS)) {
    unsigned Shift = RC->getZExtValue();
    if (Shift >= 8) {
      if (Opcode == ISD::SRA)
        return emitShift(7);
      return ConstI8(0);
    }
    return emitShift(Shift);
  }

  SDValue Default = Opcode == ISD::SRA ? emitShift(7) : ConstI8(0);
  SDValue Res = Default;
  for (int I = 7; I >= 0; --I) {
    SDValue Shifted8 = emitShift(I);
    Res = DAG.getNode(ISD::SELECT_CC, DL, MVT::i8, RHS, ConstI8(I), Shifted8,
                      Res, DAG.getCondCode(ISD::SETEQ));
  }
  return Res;
}

static SDValue lowerUnalignedI32Load(SDValue Op, SelectionDAG &DAG) {
  auto *LD = cast<LoadSDNode>(Op);
  if (LD->getMemoryVT() != MVT::i32 || LD->getAlign().value() >= 4 ||
      LD->getAddressingMode() != ISD::UNINDEXED)
    return SDValue();

  SDLoc DL(Op);
  SDValue Ptr = LD->getBasePtr();
  SDValue Chain = LD->getChain();

  auto loadByte = [&](SDValue CurChain, unsigned Offset) {
    SDValue BytePtr = Ptr;
    if (Offset)
      BytePtr = DAG.getObjectPtrOffset(DL, Ptr, TypeSize::getFixed(Offset));
    return DAG.getExtLoad(ISD::ZEXTLOAD, DL, MVT::i32, CurChain, BytePtr,
                          LD->getPointerInfo().getWithOffset(Offset), MVT::i8,
                          LD->getAlign(), LD->getMemOperand()->getFlags(),
                          LD->getAAInfo());
  };

  SDValue B0 = loadByte(Chain, 0);
  SDValue B1 = loadByte(B0.getValue(1), 1);
  SDValue B2 = loadByte(B1.getValue(1), 2);
  SDValue B3 = loadByte(B2.getValue(1), 3);

  SDValue Result = B0;
  Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result,
                       DAG.getNode(ISD::SHL, DL, MVT::i32, B1,
                                   DAG.getConstant(8, DL, MVT::i32)));
  Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result,
                       DAG.getNode(ISD::SHL, DL, MVT::i32, B2,
                                   DAG.getConstant(16, DL, MVT::i32)));
  Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result,
                       DAG.getNode(ISD::SHL, DL, MVT::i32, B3,
                                   DAG.getConstant(24, DL, MVT::i32)));

  if (LD->getValueType(0) != MVT::i32)
    Result = DAG.getNode(ISD::ANY_EXTEND, DL, LD->getValueType(0), Result);

  return DAG.getMergeValues({Result, B3.getValue(1)}, DL);
}

static SDValue lowerUnalignedI32Store(SDValue Op, SelectionDAG &DAG) {
  auto *ST = cast<StoreSDNode>(Op);
  if (ST->getMemoryVT() != MVT::i32 || ST->getAlign().value() >= 4 ||
      ST->getAddressingMode() != ISD::UNINDEXED)
    return SDValue();

  SDLoc DL(Op);
  SDValue Ptr = ST->getBasePtr();
  SDValue Val = ST->getValue();
  SDValue Chain = ST->getChain();

  auto storeByte = [&](SDValue CurChain, SDValue ByteVal, unsigned Offset) {
    SDValue BytePtr = Ptr;
    if (Offset)
      BytePtr = DAG.getObjectPtrOffset(DL, Ptr, TypeSize::getFixed(Offset));
    return DAG.getTruncStore(CurChain, DL, ByteVal, BytePtr,
                             ST->getPointerInfo().getWithOffset(Offset),
                             MVT::i8, ST->getAlign(),
                             ST->getMemOperand()->getFlags(), ST->getAAInfo());
  };

  SDValue B1 = DAG.getNode(ISD::SRL, DL, MVT::i32, Val,
                           DAG.getConstant(8, DL, MVT::i32));
  SDValue B2 = DAG.getNode(ISD::SRL, DL, MVT::i32, Val,
                           DAG.getConstant(16, DL, MVT::i32));
  SDValue B3 = DAG.getNode(ISD::SRL, DL, MVT::i32, Val,
                           DAG.getConstant(24, DL, MVT::i32));

  Chain = storeByte(Chain, Val, 0);
  Chain = storeByte(Chain, B1, 1);
  Chain = storeByte(Chain, B2, 2);
  Chain = storeByte(Chain, B3, 3);
  return Chain;
}

static bool matchConstS32(SDValue V, int32_t Value) {
  if (auto *CN = dyn_cast<ConstantSDNode>(V))
    return CN->getSExtValue() == Value;
  return false;
}

static SDValue extendForCompare(SelectionDAG &DAG, const SDLoc &DL, SDValue V,
                                ISD::CondCode CC) {
  EVT VT = V.getValueType();
  if (VT == MVT::i32)
    return V;

  if (ISD::isSignedIntSetCC(CC))
    return DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, V);

  if (ISD::isUnsignedIntSetCC(CC))
    return DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, V);

  return DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i32, V);
}

static SDValue lowerCompareSelectToSignOrOne(SelectionDAG &DAG, const SDLoc &DL,
                                             EVT VT, SDValue LHS, SDValue RHS,
                                             ISD::CondCode CC, SDValue TrueVal,
                                             SDValue FalseVal) {
  if (VT != MVT::i32)
    return SDValue();

  bool ReturnNegOneOnTrue = false;
  bool ReverseOperands = false;
  switch (CC) {
  case ISD::SETULT:
    if (!matchConstS32(TrueVal, -1) || !matchConstS32(FalseVal, 1))
      return SDValue();
    ReturnNegOneOnTrue = true;
    break;
  case ISD::SETUGT:
    if (!matchConstS32(TrueVal, -1) || !matchConstS32(FalseVal, 1))
      return SDValue();
    ReturnNegOneOnTrue = true;
    ReverseOperands = true;
    break;
  default:
    return SDValue();
  }

  if (!ReturnNegOneOnTrue)
    return SDValue();

  SDValue A = extendForCompare(DAG, DL, ReverseOperands ? RHS : LHS, CC);
  SDValue B = extendForCompare(DAG, DL, ReverseOperands ? LHS : RHS, CC);
  SDValue Diff = DAG.getNode(ISD::SUB, DL, MVT::i32, A, B);
  SDValue Sign = DAG.getNode(ISD::SRA, DL, MVT::i32, Diff,
                             DAG.getConstant(31, DL, MVT::i32));
  return DAG.getNode(ISD::OR, DL, MVT::i32, Sign,
                     DAG.getConstant(1, DL, MVT::i32));
}

static SDValue stripBooleanCastsForSelect(SDValue V) {
  while (true) {
    switch (V.getOpcode()) {
    case ISD::ZERO_EXTEND:
    case ISD::SIGN_EXTEND:
    case ISD::ANY_EXTEND:
    case ISD::TRUNCATE:
      V = V.getOperand(0);
      continue;
    case ISD::AND:
      if (matchConstS32(V.getOperand(0), 1)) {
        V = V.getOperand(1);
        continue;
      }
      if (matchConstS32(V.getOperand(1), 1)) {
        V = V.getOperand(0);
        continue;
      }
      return V;
    default:
      return V;
    }
  }
}


SDValue TC32TargetLowering::LowerBR_CC(SDValue Op,
                                       SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  auto *CCNode = cast<CondCodeSDNode>(Op.getOperand(1));
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  ISD::CondCode CC = CCNode->get();

  EVT CmpVT = LHS.getValueType();
  if (CmpVT == MVT::i8 || CmpVT == MVT::i16) {
    LHS = extendForCompare(DAG, DL, LHS, CC);
    RHS = extendForCompare(DAG, DL, RHS, CC);
  }

  SDValue Cond = DAG.getSetCC(DL, MVT::i32, LHS, RHS, CC);
  return DAG.getNode(ISD::BRCOND, DL, MVT::Other, Chain, Cond, Dest);
}


MachineBasicBlock *
TC32TargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                MachineBasicBlock *MBB) const {
  MachineFunction *MF = MBB->getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  const TargetRegisterClass *RC = &TC32::LoGR32RegClass;
  const DebugLoc DL = MI.getDebugLoc();

  if (MI.getOpcode() == TC32::TSELECTCC) {
    auto getBranchOpcode = [](int64_t BrCC) -> unsigned {
      switch (BrCC) {
      case TC32::TJEQ:
      case TC32::TJNE:
      case TC32::TJCS:
      case TC32::TJCC:
      case TC32::TJGE:
      case TC32::TJLT:
      case TC32::TJHI:
      case TC32::TJLS:
      case TC32::TJGT:
      case TC32::TJLE:
        return static_cast<unsigned>(BrCC);
      default:
        llvm_unreachable("unexpected tc32 branch opcode");
      }
    };

    Register Dst = MI.getOperand(0).getReg();
    Register FalseVal = MI.getOperand(1).getReg();
    Register TrueVal = MI.getOperand(2).getReg();
    Register LHS = MI.getOperand(3).getReg();
    Register RHS = MI.getOperand(4).getReg();
    unsigned BrOpc = getBranchOpcode(MI.getOperand(5).getImm());

    MachineBasicBlock *ThisMBB = MBB;
    MachineFunction::iterator It = ++ThisMBB->getIterator();
    MachineBasicBlock *FalseMBB =
        MF->CreateMachineBasicBlock(ThisMBB->getBasicBlock());
    MachineBasicBlock *SinkMBB =
        MF->CreateMachineBasicBlock(ThisMBB->getBasicBlock());
    MF->insert(It, FalseMBB);
    MF->insert(It, SinkMBB);

    MachineBasicBlock::iterator MII(MI);
    SinkMBB->splice(SinkMBB->begin(), ThisMBB, std::next(MII), ThisMBB->end());
    SinkMBB->transferSuccessorsAndUpdatePHIs(ThisMBB);

    ThisMBB->addSuccessor(FalseMBB);
    ThisMBB->addSuccessor(SinkMBB);
    FalseMBB->addSuccessor(SinkMBB);

    BuildMI(*ThisMBB, MII, DL, TII.get(TC32::TCMPrr)).addReg(LHS).addReg(RHS);
    BuildMI(*ThisMBB, MII, DL, TII.get(BrOpc)).addMBB(SinkMBB);

    BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dst)
        .addReg(FalseVal)
        .addMBB(FalseMBB)
        .addReg(TrueVal)
        .addMBB(ThisMBB);

    MI.eraseFromParent();
    return SinkMBB;
  }

  unsigned ShiftOpc;
  switch (MI.getOpcode()) {
  case TC32::TVARSHL:
    ShiftOpc = TC32::TSHFTLi5;
    break;
  case TC32::TVARSRL:
    ShiftOpc = TC32::TSHFTRi5;
    break;
  case TC32::TVARSRA:
    ShiftOpc = TC32::TASRi5;
    break;
  default:
    llvm_unreachable("unexpected custom inserter opcode");
  }

  Register Dst = MI.getOperand(0).getReg();
  Register Src = MI.getOperand(1).getReg();
  Register Amt = MI.getOperand(2).getReg();

  MachineBasicBlock *OrigMBB = MBB;
  MachineBasicBlock *LoopMBB = MF->CreateMachineBasicBlock(OrigMBB->getBasicBlock());
  MachineBasicBlock *DoneMBB = MF->CreateMachineBasicBlock(OrigMBB->getBasicBlock());

  auto InsertPos = std::next(MachineFunction::iterator(OrigMBB));
  MF->insert(InsertPos, LoopMBB);
  MF->insert(std::next(MachineFunction::iterator(LoopMBB)), DoneMBB);

  MachineBasicBlock::iterator MII(MI);
  MachineBasicBlock::iterator NextMII = std::next(MII);
  DoneMBB->splice(DoneMBB->begin(), OrigMBB, NextMII, OrigMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(OrigMBB);

  OrigMBB->addSuccessor(DoneMBB);
  OrigMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(DoneMBB);

  Register LoopVal = MRI.createVirtualRegister(RC);
  Register LoopCnt = MRI.createVirtualRegister(RC);
  Register ShiftedVal = MRI.createVirtualRegister(RC);
  Register DecCnt = MRI.createVirtualRegister(RC);

  BuildMI(*OrigMBB, MII, DL, TII.get(TC32::TCMPri0)).addReg(Amt);
  BuildMI(*OrigMBB, MII, DL, TII.get(TC32::TJEQ)).addMBB(DoneMBB);
  BuildMI(*OrigMBB, MII, DL, TII.get(TC32::TJ)).addMBB(LoopMBB);

  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), LoopVal)
      .addReg(Src)
      .addMBB(OrigMBB)
      .addReg(ShiftedVal)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), LoopCnt)
      .addReg(Amt)
      .addMBB(OrigMBB)
      .addReg(DecCnt)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(ShiftOpc), ShiftedVal)
      .addReg(LoopVal)
      .addImm(1);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TC32::TSUBri1), DecCnt)
      .addReg(LoopCnt);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TC32::TCMPri0)).addReg(DecCnt);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TC32::TJNE)).addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TC32::TJ)).addMBB(DoneMBB);

  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dst)
      .addReg(Src)
      .addMBB(OrigMBB)
      .addReg(ShiftedVal)
      .addMBB(LoopMBB);

  MI.eraseFromParent();
  return DoneMBB;
}

SDValue TC32TargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDLoc DL(Op);

  switch (Op.getOpcode()) {
  case ISD::LOAD:
    return lowerUnalignedI32Load(Op, DAG);
  case ISD::STORE:
    return lowerUnalignedI32Store(Op, DAG);
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::SELECT: {
    EVT VT = Op.getValueType();
    if (VT != MVT::i32 && VT != MVT::i8)
      break;

    SDValue Cond = Op.getOperand(0);
    SDValue TrueVal = Op.getOperand(1);
    SDValue FalseVal = Op.getOperand(2);

    SDValue RawCond = stripBooleanCastsForSelect(Cond);
    if (RawCond.getOpcode() == ISD::SETCC) {
      if (auto *CC = dyn_cast<CondCodeSDNode>(RawCond.getOperand(2))) {
        if (SDValue Res = lowerCompareSelectToSignOrOne(
                DAG, DL, VT, RawCond.getOperand(0), RawCond.getOperand(1),
                CC->get(), TrueVal, FalseVal))
          return Res;
      }
    }

    if (Cond.getOpcode() == ISD::SETCC) {
      if (auto *CC = dyn_cast<CondCodeSDNode>(Cond.getOperand(2))) {
        if (SDValue Res = lowerCompareSelectToSignOrOne(
                DAG, DL, VT, Cond.getOperand(0), Cond.getOperand(1), CC->get(),
                TrueVal, FalseVal))
          return Res;
      }
      return DAG.getNode(ISD::SELECT_CC, DL, VT, Cond.getOperand(0),
                         Cond.getOperand(1), TrueVal, FalseVal,
                         Cond.getOperand(2));
    }

    EVT CondVT = Cond.getValueType();
    if (CondVT == MVT::i1 || CondVT == MVT::i8 || CondVT == MVT::i32) {
      SDValue Zero = DAG.getConstant(0, DL, CondVT);
      return DAG.getNode(ISD::SELECT_CC, DL, VT, Cond, Zero, TrueVal,
                         FalseVal, DAG.getCondCode(ISD::SETNE));
    }
    break;
  }
  case ISD::SETCC: {
    EVT VT = Op.getValueType();
    if (VT != MVT::i32 && VT != MVT::i8)
      break;

    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    SDValue CC = Op.getOperand(2);
    if (LHS.getValueType() == MVT::i8 || LHS.getValueType() == MVT::i16) {
      auto *CCNode = dyn_cast<CondCodeSDNode>(CC);
      if (!CCNode)
        break;
      LHS = extendForCompare(DAG, DL, LHS, CCNode->get());
      RHS = extendForCompare(DAG, DL, RHS, CCNode->get());
    }
    SDValue One = DAG.getConstant(1, DL, VT);
    SDValue Zero = DAG.getConstant(0, DL, VT);
    return DAG.getNode(ISD::SELECT_CC, DL, VT, LHS, RHS, One, Zero, CC);
  }
  case ISD::FrameIndex: {
    int FI = cast<FrameIndexSDNode>(Op)->getIndex();
    return DAG.getNode(TC32ISD::FRAMEADDR, DL, MVT::i32,
                       DAG.getConstant(FI, DL, MVT::i32),
                       DAG.getConstant(0, DL, MVT::i32));
  }
  case ISD::VASTART: {
    MachineFunction &MF = DAG.getMachineFunction();
    const unsigned NumFixedArgs = MF.getFunction().arg_size();
    const EVT PtrVT = getPointerTy(DAG.getDataLayout());
    const int FI =
        MF.getFrameInfo().CreateFixedObject(4, static_cast<int>(NumFixedArgs * 4), true);
    SDValue FR = DAG.getFrameIndex(FI, PtrVT);
    const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
    return DAG.getStore(Op.getOperand(0), DL, FR, Op.getOperand(1),
                        MachinePointerInfo(SV));
  }
  case ISD::SHL: {
    if (Op.getValueType() == MVT::i8)
      return lowerI8Shift(Op, DAG);
    break;
  }
  case ISD::SRL:
  case ISD::SRA: {
    if (Op.getValueType() == MVT::i8)
      return lowerI8Shift(Op, DAG);
    break;
  }
  default:
    break;
  }

  return SDValue();
}

SDValue TC32TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  (void)CallConv;
  static const MCPhysReg ArgRegs[] = {TC32::R0, TC32::R1, TC32::R2, TC32::R3};

  MachineFunction &MF = DAG.getMachineFunction();
  for (unsigned I = 0; I < Ins.size(); ++I) {
    if (Ins[I].VT != MVT::i32 && Ins[I].VT != MVT::i16 && Ins[I].VT != MVT::i8)
      report_fatal_error("TC32 only supports i32/i16/i8 arguments right now");
    if (I < std::size(ArgRegs)) {
      Register VReg = MF.addLiveIn(ArgRegs[I], &TC32::LoGR32RegClass);
      SDValue Arg = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
      if (Ins[I].VT == MVT::i8 || Ins[I].VT == MVT::i16) {
        SDValue NarrowArg =
            normalizeIncomingScalarArgI32(DAG, dl, Arg, Ins[I].VT, Ins[I].Flags);
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, Ins[I].VT, NarrowArg));
      }
      else
        InVals.push_back(Arg);
      Chain = Arg.getValue(1);
    } else {
      int FI = MF.getFrameInfo().CreateFixedObject(
          4, (IsVarArg ? 16 : 0) + (I - std::size(ArgRegs)) * 4,
          /*IsImmutable=*/true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      SDValue Load =
          DAG.getLoad(MVT::i32, dl, Chain, FIN, MachinePointerInfo::getFixedStack(MF, FI));
      if (Ins[I].VT == MVT::i8 || Ins[I].VT == MVT::i16) {
        SDValue NarrowArg =
            normalizeIncomingScalarArgI32(DAG, dl, Load, Ins[I].VT, Ins[I].Flags);
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, Ins[I].VT, NarrowArg));
      }
      else
        InVals.push_back(Load);
      Chain = Load.getValue(1);
    }
  }
  return Chain;
}

SDValue TC32TargetLowering::LowerCall(CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDValue Chain = CLI.Chain;
  const SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;

  static const MCPhysReg ArgRegs[] = {TC32::R0, TC32::R1, TC32::R2, TC32::R3};

  if (CLI.IsVarArg)
    report_fatal_error("TC32 varargs calls are not implemented yet");
  CLI.IsTailCall = false;

  SDValue Callee = CLI.Callee;
  EVT PtrVT = getPointerTy(DAG.getDataLayout());
  bool IsIndirectCall = false;
  switch (Callee.getOpcode()) {
  case ISD::GlobalAddress: {
    auto *GAN = cast<GlobalAddressSDNode>(Callee);
    Callee = DAG.getTargetGlobalAddress(GAN->getGlobal(), DL, PtrVT, GAN->getOffset());
    break;
  }
  case ISD::ExternalSymbol: {
    auto *ESN = cast<ExternalSymbolSDNode>(Callee);
    Callee = DAG.getTargetExternalSymbol(ESN->getSymbol(), PtrVT);
    break;
  }
  case ISD::TargetGlobalAddress:
  case ISD::TargetExternalSymbol:
    break;
  default:
    IsIndirectCall = true;
    break;
  }

  MachineFunction &MF = DAG.getMachineFunction();
  MF.getFrameInfo().setHasCalls(true);
  unsigned StackArgBytes =
      Outs.size() > std::size(ArgRegs) ? (Outs.size() - std::size(ArgRegs)) * 4 : 0;
  Chain = DAG.getCALLSEQ_START(Chain, StackArgBytes, 0, DL);

  SmallVector<std::pair<Register, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;

  for (unsigned I = 0; I < Outs.size(); ++I) {
    if (Outs[I].VT != MVT::i32 && Outs[I].VT != MVT::i16 && Outs[I].VT != MVT::i8)
      report_fatal_error("TC32 only supports i32/i16/i8 call arguments right now");
    SDValue ArgVal = OutVals[I];
    if (Outs[I].VT == MVT::i8 || Outs[I].VT == MVT::i16)
      ArgVal = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, ArgVal);

    if (I < std::size(ArgRegs)) {
      RegsToPass.push_back({ArgRegs[I], ArgVal});
      continue;
    }

    if (!StackPtr.getNode())
      StackPtr = DAG.getCopyFromReg(Chain, DL, TC32::R13, PtrVT);
    unsigned StackOffset = (I - std::size(ArgRegs)) * 4;
    SDValue Addr = StackPtr;
    if (StackOffset != 0)
      Addr = DAG.getNode(ISD::ADD, DL, PtrVT, StackPtr,
                         DAG.getIntPtrConstant(StackOffset, DL));
    MemOpChains.push_back(
        DAG.getStore(Chain, DL, ArgVal, Addr, MachinePointerInfo::getStack(MF, StackOffset)));
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  SDValue Glue;
  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, Glue);
    Glue = Chain.getValue(1);
  }

  if (IsIndirectCall) {
    Chain = DAG.getCopyToReg(Chain, DL, TC32::R12, Callee, Glue);
    Glue = Chain.getValue(1);
    Callee = DAG.getTargetExternalSymbol("__tc32_indirect_call_r12", PtrVT);
  }

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Callee);
  Ops.push_back(Chain);

  unsigned CallOpc = TC32::TJL;
  switch (RegsToPass.size()) {
  case 0: CallOpc = TC32::TJL; break;
  case 1: CallOpc = TC32::TJL_R0; break;
  case 2: CallOpc = TC32::TJL_R0_R1; break;
  case 3: CallOpc = TC32::TJL_R0_R1_R2; break;
  default: CallOpc = TC32::TJL_R0_R1_R2_R3; break;
  }

  MachineSDNode *Call =
      DAG.getMachineNode(CallOpc, DL, {MVT::Other, MVT::Glue}, Ops);
  Chain = SDValue(Call, 0);
  Glue = SDValue(Call, 1);
  Chain = DAG.getCALLSEQ_END(Chain, DAG.getConstant(StackArgBytes, DL, PtrVT),
                             DAG.getConstant(0, DL, PtrVT), Glue, DL);
  Glue = Chain.getValue(1);

  if (Ins.empty())
    return Chain;
  if (Ins.size() != 1 ||
      (Ins[0].VT != MVT::i32 && Ins[0].VT != MVT::i16 && Ins[0].VT != MVT::i8))
    report_fatal_error("TC32 only supports single i32/i16/i8 call return values right now");

  SDValue RetVal32 = DAG.getCopyFromReg(Chain, DL, TC32::R0, MVT::i32, Glue);
  if (Ins[0].VT == MVT::i8)
    InVals.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i8, RetVal32));
  else if (Ins[0].VT == MVT::i16)
    InVals.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i16, RetVal32));
  else
    InVals.push_back(RetVal32);
  return RetVal32.getValue(1);
}

bool TC32TargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  (void)CallConv;
  (void)MF;
  (void)Context;
  (void)RetTy;
  if (Outs.size() > 1)
    return false;
  return Outs.empty() || Outs[0].VT == MVT::i32 || Outs[0].VT == MVT::i16 ||
         Outs[0].VT == MVT::i8;
}

SDValue TC32TargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
    SelectionDAG &DAG) const {
  (void)CallConv;
  (void)IsVarArg;

  SDValue Glue;
  if (!Outs.empty()) {
    if (Outs.size() != 1 ||
        (Outs[0].VT != MVT::i32 && Outs[0].VT != MVT::i16 && Outs[0].VT != MVT::i8))
      report_fatal_error("TC32 only supports single i32/i16/i8 return values right now");
    SDValue RetVal = OutVals[0];
    if (Outs[0].VT == MVT::i8 || Outs[0].VT == MVT::i16)
      RetVal = DAG.getNode(ISD::ZERO_EXTEND, dl, MVT::i32, RetVal);
    Chain = DAG.getCopyToReg(Chain, dl, TC32::R0, RetVal, Glue);
    Glue = Chain.getValue(1);
    return DAG.getNode(TC32ISD::RET_FLAG, dl, MVT::Other, Chain, Glue);
  }
  return DAG.getNode(TC32ISD::RET_FLAG, dl, MVT::Other, Chain);
}
