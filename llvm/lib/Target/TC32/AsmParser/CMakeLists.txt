add_llvm_component_library(LLVMTC32AsmParser
  TC32AsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  TC32Desc
  TC32Info
  Support

  ADD_TO_COMPONENT
  TC32
  )
