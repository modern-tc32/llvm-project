add_llvm_component_library(LLVMTC32Desc
  TC32AsmBackend.cpp
  TC32InstPrinter.cpp
  TC32ELFObjectWriter.cpp
  TC32MCCodeEmitter.cpp
  TC32MCAsmInfo.cpp
  TC32MCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser
  TC32Info

  ADD_TO_COMPONENT
  TC32
  )
