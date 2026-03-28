include "llvm/Target/Target.td"

class Proc<string Name, list<SubtargetFeature> Features>
    : Processor<Name, NoItineraries, Features>;

def FeatureReserveR7 : SubtargetFeature<"reserve-r7", "ReserveR7", "true",
                                        "Treat r7 as an ABI-reserved register">;

def : Proc<"generic", [FeatureReserveR7]>;

include "TC32RegisterInfo.td"
include "TC32InstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<LoGR32>;

def TC32InstrInfo : InstrInfo;

def TC32AsmParser : AsmParser;

def TC32AsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
}

def TC32 : Target {
  let InstructionSet = TC32InstrInfo;
  let AssemblyParsers = [TC32AsmParser];
  let AssemblyWriters = [TC32AsmWriter];
}
