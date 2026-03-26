add_llvm_component_library(LLVMTC32Info
  TC32TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  TC32
  )
