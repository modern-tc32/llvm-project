#include "TC32ISelLowering.h"
#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TC32Subtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

TC32TargetLowering::TC32TargetLowering(const TargetMachine &TM,
                                       const TC32Subtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &TC32::GR32RegClass);
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
  setOperationAction(ISD::SHL, MVT::i32, Legal);
  setOperationAction(ISD::SHL, MVT::i8, Legal);
  setOperationAction(ISD::SRL, MVT::i32, Legal);
  setOperationAction(ISD::SRL, MVT::i8, Legal);
  setOperationAction(ISD::SRA, MVT::i32, Legal);
  setOperationAction(ISD::SRA, MVT::i8, Legal);
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
  setOperationAction(ISD::AssertZext, MVT::i32, Legal);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i8, MVT::i1, Promote);
  setLoadExtAction(ISD::EXTLOAD, MVT::i8, MVT::i1, Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i8, Legal);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i16, Legal);
  setLoadExtAction(ISD::EXTLOAD, MVT::i32, MVT::i16, Legal);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MVT::i16, Expand);
  setTruncStoreAction(MVT::i32, MVT::i16, Legal);
  setTruncStoreAction(MVT::i32, MVT::i8, Legal);
}

const char *TC32TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case TC32ISD::CALL:
    return "TC32ISD::CALL";
  case TC32ISD::RET_FLAG:
    return "TC32ISD::RET_FLAG";
  default:
    return nullptr;
  }
}

SDValue TC32TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  (void)CallConv;
  static const MCPhysReg ArgRegs[] = {TC32::R0, TC32::R1, TC32::R2, TC32::R3};
  if (IsVarArg)
    report_fatal_error("TC32 varargs are not implemented yet");

  MachineFunction &MF = DAG.getMachineFunction();
  for (unsigned I = 0; I < Ins.size(); ++I) {
    if (Ins[I].VT != MVT::i32 && Ins[I].VT != MVT::i16 && Ins[I].VT != MVT::i8)
      report_fatal_error("TC32 only supports i32/i16/i8 arguments right now");
    if (I < std::size(ArgRegs)) {
      Register VReg = MF.addLiveIn(ArgRegs[I], &TC32::LoGR32RegClass);
      SDValue Arg = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
      if (Ins[I].VT == MVT::i8)
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, MVT::i8, Arg));
      else if (Ins[I].VT == MVT::i16)
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, MVT::i16, Arg));
      else
        InVals.push_back(Arg);
      Chain = Arg.getValue(1);
    } else {
      int FI = MF.getFrameInfo().CreateFixedObject(4, (I - std::size(ArgRegs)) * 4,
                                                   /*IsImmutable=*/true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      SDValue Load =
          DAG.getLoad(MVT::i32, dl, Chain, FIN, MachinePointerInfo::getFixedStack(MF, FI));
      if (Ins[I].VT == MVT::i8)
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, MVT::i8, Load));
      else if (Ins[I].VT == MVT::i16)
        InVals.push_back(DAG.getNode(ISD::TRUNCATE, dl, MVT::i16, Load));
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
    report_fatal_error("TC32 only supports direct calls right now");
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

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);
  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));
  if (Glue)
    Ops.push_back(Glue);

  MachineSDNode *Call =
      DAG.getMachineNode(TC32::TJL, DL, {MVT::Other, MVT::Glue}, Ops);
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
  (void)IsVarArg;
  (void)Context;
  (void)RetTy;
  if (IsVarArg)
    return false;
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
  if (IsVarArg)
    report_fatal_error("TC32 varargs are not implemented yet");

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
