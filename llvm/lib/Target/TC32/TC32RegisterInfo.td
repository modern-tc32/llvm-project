class TC32Reg<bits<4> Enc, string Name, list<string> AltNames = []>
    : Register<Name, AltNames> {
  let Namespace = "TC32";
  let HWEncoding{3-0} = Enc;
  let DwarfNumbers = [Enc];
}

class TC32SpecialReg<bits<4> Enc, string Name> : Register<Name> {
  let Namespace = "TC32";
  let HWEncoding{3-0} = Enc;
}

def R0  : TC32Reg<0,  "r0">;
def R1  : TC32Reg<1,  "r1">;
def R2  : TC32Reg<2,  "r2">;
def R3  : TC32Reg<3,  "r3">;
def R4  : TC32Reg<4,  "r4">;
def R5  : TC32Reg<5,  "r5">;
def R6  : TC32Reg<6,  "r6">;
def R7  : TC32Reg<7,  "r7">;
def R8  : TC32Reg<8,  "r8">;
def R9  : TC32Reg<9,  "r9">;
def R10 : TC32Reg<10, "r10">;
def R11 : TC32Reg<11, "r11">;
def R12 : TC32Reg<12, "r12">;
def R13 : TC32Reg<13, "r13", ["sp"]>;
def R14 : TC32Reg<14, "r14", ["lr"]>;
def R15 : TC32Reg<15, "r15", ["pc"]>;

def CPSR : TC32SpecialReg<0, "cpsr">;

let RegInfos = RegInfoByHwMode<[DefaultMode], [RegInfo<32, 32, 32>]> in
def GR32 : RegisterClass<"TC32", [i32, i8], 32,
                         (add R0, R1, R2, R3, R4, R5, R6, R7,
                              R8, R9, R10, R11, R12, R13, R14, R15)>;

let RegInfos = RegInfoByHwMode<[DefaultMode], [RegInfo<32, 32, 32>]> in
def LoGR32 : RegisterClass<"TC32", [i32, i8], 32,
                           (add R0, R1, R2, R3, R4, R5, R6, R7)>;
