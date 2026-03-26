add_llvm_component_group(TC32)

set(LLVM_TARGET_DEFINITIONS TC32.td)

tablegen(LLVM TC32GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM TC32GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM TC32GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM TC32GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(TC32CommonTableGen)

add_llvm_target(TC32CodeGen
  TC32AsmPrinter.cpp
  TC32FrameLowering.cpp
  TC32ISelDAGToDAG.cpp
  TC32ISelLowering.cpp
  TC32InstrInfo.cpp
  TC32MCInstLower.cpp
  TC32RegisterInfo.cpp
  TC32Subtarget.cpp
  TC32TargetMachine.cpp

  LINK_COMPONENTS
  AsmPrinter
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TC32Desc
  TC32Info

  ADD_TO_COMPONENT
  TC32
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(AsmParser)
add_subdirectory(TargetInfo)
