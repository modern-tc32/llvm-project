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
  setOperationAction(ISD::SHL, MVT::i32, Legal);
  setOperationAction(ISD::SHL, MVT::i8, Legal);
  setOperationAction(ISD::SRL, MVT::i32, Legal);
  setOperationAction(ISD::SRL, MVT::i8, Legal);
  setOperationAction(ISD::SRA, MVT::i32, Legal);
  setOperationAction(ISD::SRA, MVT::i8, Legal);
  setOperationAction(ISD::AssertZext, MVT::i32, Legal);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MVT::i8, Legal);
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
    if (Ins[I].VT != MVT::i32 && Ins[I].VT != MVT::i8)
      report_fatal_error("TC32 only supports i32/i8 arguments right now");
    if (I < std::size(ArgRegs)) {
      Register VReg = MF.addLiveIn(ArgRegs[I], &TC32::LoGR32RegClass);
      EVT ArgVT = Ins[I].VT == MVT::i8 ? MVT::i8 : MVT::i32;
      SDValue Arg = DAG.getCopyFromReg(Chain, dl, VReg, ArgVT);
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
  if (Outs.size() > std::size(ArgRegs))
    report_fatal_error("TC32 stack-passed call arguments are not implemented yet");
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
  default:
    report_fatal_error("TC32 only supports direct calls right now");
  }

  MachineFunction &MF = DAG.getMachineFunction();
  MF.getFrameInfo().setHasCalls(true);

  SDValue Glue;
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Callee);

  for (unsigned I = 0; I < Outs.size(); ++I) {
    if (Outs[I].VT != MVT::i32 && Outs[I].VT != MVT::i8)
      report_fatal_error("TC32 only supports i32/i8 call arguments right now");
    SDValue ArgVal = OutVals[I];
    if (Outs[I].VT == MVT::i8)
      ArgVal = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, ArgVal);
    Chain = DAG.getCopyToReg(Chain, DL, ArgRegs[I], ArgVal, Glue);
    Glue = Chain.getValue(1);
    Ops.push_back(DAG.getRegister(ArgRegs[I], MVT::i32));
  }

  Ops.push_back(Chain);
  if (Glue)
    Ops.push_back(Glue);

  MachineSDNode *Call =
      DAG.getMachineNode(TC32::TJL, DL, {MVT::Other, MVT::Glue}, Ops);
  Chain = SDValue(Call, 0);
  Glue = SDValue(Call, 1);

  if (Ins.empty())
    return Chain;
  if (Ins.size() != 1 || (Ins[0].VT != MVT::i32 && Ins[0].VT != MVT::i8))
    report_fatal_error("TC32 only supports single i32/i8 call return values right now");

  SDValue RetVal32 = DAG.getCopyFromReg(Chain, DL, TC32::R0, MVT::i32, Glue);
  if (Ins[0].VT == MVT::i8)
    InVals.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i8, RetVal32));
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
  return Outs.empty() || Outs[0].VT == MVT::i32 || Outs[0].VT == MVT::i8;
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
    if (Outs.size() != 1 || (Outs[0].VT != MVT::i32 && Outs[0].VT != MVT::i8))
      report_fatal_error("TC32 only supports single i32/i8 return values right now");
    SDValue RetVal = OutVals[0];
    if (Outs[0].VT == MVT::i8)
      RetVal = DAG.getNode(ISD::ZERO_EXTEND, dl, MVT::i32, RetVal);
    Chain = DAG.getCopyToReg(Chain, dl, TC32::R0, RetVal, Glue);
    Glue = Chain.getValue(1);
    return DAG.getNode(TC32ISD::RET_FLAG, dl, MVT::Other, Chain, Glue);
  }
  return DAG.getNode(TC32ISD::RET_FLAG, dl, MVT::Other, Chain);
}
