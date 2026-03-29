#ifndef LLVM_LIB_TARGET_TC32_TC32ISELLOWERING_H
#define LLVM_LIB_TARGET_TC32_TC32ISELLOWERING_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Support/Alignment.h"

namespace llvm {

class TC32Subtarget;

namespace TC32ISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  FRAMEADDR,
  BRCOND,
  ULOAD,
  USTORE,
  CALL,
  RET_FLAG
};
}

class TC32TargetLowering : public TargetLowering {
public:
  explicit TC32TargetLowering(const TargetMachine &TM,
                              const TC32Subtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr &MI,
                                                 MachineBasicBlock *MBB) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  bool allowsMisalignedMemoryAccesses(EVT VT, unsigned AddrSpace,
                                      Align Alignment,
                                      MachineMemOperand::Flags Flags,
                                      unsigned *Fast) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context,
                      const Type *RetTy) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                      SelectionDAG &DAG) const override;
};

} // namespace llvm

#endif
