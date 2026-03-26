class TC32Inst<dag Outs, dag Ins, string Asm, list<dag> Pats = []>
    : Instruction {
  let Namespace = "TC32";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Pattern = Pats;
  let Size = 2;
}

class Pseudo<dag Outs, dag Ins, string Asm, list<dag> Pats = []> : Instruction {
  let Namespace = "TC32";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Pattern = Pats;
  let Size = 0;
  let isCodeGenOnly = 1;
  let isPseudo = 1;
}

def uimm3 : Operand<i32>;
def uimm8 : Operand<i32>;
def calltarget : Operand<i32>;
def brtarget : Operand<OtherVT>;
def addrtarget : Operand<i32>;

let Defs = [R13], Uses = [R13] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              "#ADJCALLSTACKDOWN $amt1, $amt2">;
def ADJCALLSTACKUP : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                            "#ADJCALLSTACKUP $amt1, $amt2">;
}

def TPUSH_R7_LR : TC32Inst<(outs), (ins), "tpush\t{r7, lr}"> {
  let Defs = [R13];
  let Uses = [R7, R13, R14];
}

def TPOP_R7_PC : TC32Inst<(outs), (ins), "tpop\t{r7, pc}"> {
  let Defs = [R7, R13, R15];
  let Uses = [R13];
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def TJL : TC32Inst<(outs), (ins calltarget:$dst, variable_ops), "tjl\t$dst"> {
  let Size = 4;
  let isCall = 1;
  let Defs = [R0, R1, R2, R3, R14];
}

def TCMPri0 : TC32Inst<(outs), (ins LoGR32:$src), "tcmp\t$src, #0">;

def TCMPrr : TC32Inst<(outs), (ins LoGR32:$lhs, LoGR32:$rhs),
                      "tcmp\t$lhs, $rhs">;

def TJEQ : TC32Inst<(outs), (ins brtarget:$dst), "tjeq\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJNE : TC32Inst<(outs), (ins brtarget:$dst), "tjne\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJGE : TC32Inst<(outs), (ins brtarget:$dst), "tjge\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJLT : TC32Inst<(outs), (ins brtarget:$dst), "tjlt\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJGT : TC32Inst<(outs), (ins brtarget:$dst), "tjgt\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJLE : TC32Inst<(outs), (ins brtarget:$dst), "tjle\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
}

def TJ : TC32Inst<(outs), (ins brtarget:$dst), "tj\t$dst"> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def TRET : TC32Inst<(outs), (ins), "tmov\tpc, lr"> {
  let Defs = [R15];
  let Uses = [R14];
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

def TRET_R0 : TC32Inst<(outs), (ins), "tmov\tpc, lr"> {
  let Defs = [R15];
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let Uses = [R0, R14];
}

def TMOVrr : TC32Inst<(outs GR32:$dst), (ins GR32:$src), "tmov\t$dst, $src">;

def TMOVi8 : TC32Inst<(outs LoGR32:$dst), (ins uimm8:$imm),
                      "tmovs\t$dst, #$imm">;

def TNEGrr : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src),
                      "tneg\t$dst, $src">;

def TMOVNrr : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src),
                       "tmovn\t$dst, $src">;

def TADDSrru8 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src, uimm3:$imm),
                         "tadds\t$dst, $src, #$imm">;

let Constraints = "$dst = $src" in
def TADDSri8 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src, uimm8:$imm),
                        "tadds\t$dst, $src, #$imm">;

def TADDSrrr : TC32Inst<(outs LoGR32:$dst),
                        (ins LoGR32:$src, LoGR32:$rhs),
                        "tadds\t$dst, $src, $rhs">;

let Constraints = "$dst = $lhs" in
def TADDCrr : TC32Inst<(outs LoGR32:$dst),
                       (ins LoGR32:$lhs, LoGR32:$rhs),
                       "taddc\t$dst, $rhs">;

let Constraints = "$dst = $lhs" in
def TANDrr : TC32Inst<(outs LoGR32:$dst),
                      (ins LoGR32:$lhs, LoGR32:$rhs),
                      "tand\t$dst, $rhs">;

let Constraints = "$dst = $lhs" in
def TORrr : TC32Inst<(outs LoGR32:$dst),
                     (ins LoGR32:$lhs, LoGR32:$rhs),
                     "tor\t$dst, $rhs">;

let Constraints = "$dst = $lhs" in
def TXORrr : TC32Inst<(outs LoGR32:$dst),
                      (ins LoGR32:$lhs, LoGR32:$rhs),
                      "txor\t$dst, $rhs">;

let Constraints = "$dst = $lhs" in
def TMULrr : TC32Inst<(outs LoGR32:$dst),
                      (ins LoGR32:$lhs, LoGR32:$rhs),
                      "tmul\t$dst, $rhs">;

def TSUBrrr : TC32Inst<(outs LoGR32:$dst),
                       (ins LoGR32:$src, LoGR32:$rhs),
                       "tsub\t$dst, $src, $rhs">;

def TSUBri1 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src),
                       "tsub\t$dst, $src, #1">;

let Constraints = "$dst = $lhs" in
def TSUBCrr : TC32Inst<(outs LoGR32:$dst),
                       (ins LoGR32:$lhs, LoGR32:$rhs),
                       "tsubc\t$dst, $rhs">;

def TSHFTRi31 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src),
                         "tshftr\t$dst, $src, #31">;

def TASRi31 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src),
                       "tasr\t$dst, $src, #31">;

def TSHFTLi5 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src, uimm8:$imm),
                        "tshftl\t$dst, $src, #$imm">;

def TSHFTRi5 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src, uimm8:$imm),
                        "tshftr\t$dst, $src, #$imm">;

def TASRi5 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$src, uimm8:$imm),
                      "tasr\t$dst, $src, #$imm">;

def TLOADspu8 : TC32Inst<(outs LoGR32:$dst), (ins uimm8:$imm),
                         "tloadr\t$dst, [sp, #$imm]"> {
  let Uses = [R13];
}

def TLOADfpu8 : TC32Inst<(outs LoGR32:$dst), (ins uimm8:$imm),
                         "tloadr\t$dst, [r7, #$imm]"> {
  let Uses = [R7];
}

def TLOADpcu8 : TC32Inst<(outs LoGR32:$dst), (ins uimm8:$imm),
                         "tloadr\t$dst, [pc, #$imm]"> {
  let Uses = [R15];
}

def TLOADrr : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$base),
                       "tloadr\t$dst, [$base]">;

def TLOADru3 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$base, uimm8:$imm),
                        "tloadr\t$dst, [$base, #$imm]">;

def TLOADBrr : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$base),
                        "tloadrb\t$dst, [$base]">;

def TLOADBru3 : TC32Inst<(outs LoGR32:$dst), (ins LoGR32:$base, uimm8:$imm),
                         "tloadrb\t$dst, [$base, #$imm]">;

def TSTOREspu8 : TC32Inst<(outs), (ins LoGR32:$src, uimm8:$imm),
                          "tstorer\t$src, [sp, #$imm]"> {
  let Uses = [R13];
}

def TSTOREfpu8 : TC32Inst<(outs), (ins LoGR32:$src, uimm8:$imm),
                          "tstorer\t$src, [r7, #$imm]"> {
  let Uses = [R7];
}

def TSTORErr : TC32Inst<(outs), (ins LoGR32:$src, LoGR32:$base),
                        "tstorer\t$src, [$base]">;

def TSTOREru3 : TC32Inst<(outs), (ins LoGR32:$src, LoGR32:$base, uimm8:$imm),
                         "tstorer\t$src, [$base, #$imm]">;

def TSTOREBrr : TC32Inst<(outs), (ins LoGR32:$src, LoGR32:$base),
                         "tstorerb\t$src, [$base]">;

def TSTOREBru3 : TC32Inst<(outs), (ins LoGR32:$src, LoGR32:$base, uimm8:$imm),
                          "tstorerb\t$src, [$base, #$imm]">;

def TADDspu8 : TC32Inst<(outs), (ins uimm8:$imm), "tadd\tsp, #$imm"> {
  let Defs = [R13];
  let Uses = [R13];
}

def TSUBspu8 : TC32Inst<(outs), (ins uimm8:$imm), "tsub\tsp, #$imm"> {
  let Defs = [R13];
  let Uses = [R13];
}

def TADDdstspu8 : TC32Inst<(outs LoGR32:$dst), (ins uimm8:$imm),
                           "tadd\t$dst, sp, #$imm"> {
  let Uses = [R13];
}

def TLOADaddr : Pseudo<(outs LoGR32:$dst), (ins addrtarget:$addr),
                       "#TLOADaddr $dst, $addr">;
