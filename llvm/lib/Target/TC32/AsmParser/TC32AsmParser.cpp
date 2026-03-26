#include "MCTargetDesc/TC32MCTargetDesc.h"
#include "TargetInfo/TC32TargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace {

class TC32Operand : public MCParsedAsmOperand {
public:
  enum KindTy { Token, Reg, Imm } Kind;

  SMLoc Start, End;
  StringRef Tok;
  MCRegister RegNo = TC32::NoRegister;
  const MCExpr *Expr = nullptr;

  TC32Operand(KindTy K, SMLoc S, SMLoc E) : Kind(K), Start(S), End(E) {}

  static std::unique_ptr<TC32Operand> createToken(StringRef Tok, SMLoc Loc) {
    auto Op = std::make_unique<TC32Operand>(Token, Loc, Loc);
    Op->Tok = Tok;
    return Op;
  }

  static std::unique_ptr<TC32Operand> createReg(MCRegister Reg, SMLoc S,
                                                SMLoc E) {
    auto Op = std::make_unique<TC32Operand>(TC32Operand::Reg, S, E);
    Op->RegNo = Reg;
    return Op;
  }

  static std::unique_ptr<TC32Operand> createImm(const MCExpr *Expr, SMLoc S,
                                                SMLoc E) {
    auto Op = std::make_unique<TC32Operand>(Imm, S, E);
    Op->Expr = Expr;
    return Op;
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Reg; }
  bool isImm() const override { return Kind == Imm; }
  bool isMem() const override { return false; }

  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }
  MCRegister getReg() const override { return RegNo; }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    switch (Kind) {
    case Token:
      OS << Tok;
      break;
    case Reg:
      OS << "<reg " << RegNo.id() << ">";
      break;
    case Imm:
      MAI.printExpr(OS, *Expr);
      break;
    }
  }
};

class TC32AsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

  MCAsmParser &getParser() const { return Parser; }
  AsmLexer &getLexer() const { return Parser.getLexer(); }

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override {
    return Kind;
  }
  void convertToMapAndConstraints(unsigned, const OperandVector &) override {}

  bool parseOperand(OperandVector &Operands);
  bool parseImmediateOperand(OperandVector &Operands);
  bool parseFixedPushPopOperands(StringRef Mnemonic, OperandVector &Operands);
  bool parseComma();
  bool parseEOL();

public:
  TC32AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser) {
    MCAsmParserExtension::Initialize(Parser);
  }
};

static bool isLoReg(MCRegister Reg) {
  switch (Reg.id()) {
  case TC32::R0:
  case TC32::R1:
  case TC32::R2:
  case TC32::R3:
  case TC32::R4:
  case TC32::R5:
  case TC32::R6:
  case TC32::R7:
    return true;
  default:
    return false;
  }
}

static bool extractAbsoluteImm(const MCExpr *Expr, int64_t &Value) {
  if (const auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    Value = CE->getValue();
    return true;
  }
  return false;
}

} // namespace

ParseStatus TC32AsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                            SMLoc &EndLoc) {
  if (getLexer().getKind() != AsmToken::Identifier)
    return ParseStatus::NoMatch;

  std::string Name = getLexer().getTok().getIdentifier().lower();
  Reg = StringSwitch<MCRegister>(Name)
            .Case("r0", TC32::R0)
            .Case("r1", TC32::R1)
            .Case("r2", TC32::R2)
            .Case("r3", TC32::R3)
            .Case("r4", TC32::R4)
            .Case("r5", TC32::R5)
            .Case("r6", TC32::R6)
            .Case("r7", TC32::R7)
            .Case("r8", TC32::R8)
            .Case("r9", TC32::R9)
            .Case("r10", TC32::R10)
            .Case("r11", TC32::R11)
            .Case("r12", TC32::R12)
            .Case("r13", TC32::R13)
            .Case("r14", TC32::R14)
            .Case("r15", TC32::R15)
            .Case("sp", TC32::R13)
            .Case("lr", TC32::R14)
            .Case("pc", TC32::R15)
            .Default(TC32::NoRegister);
  if (Reg == TC32::NoRegister)
    return ParseStatus::NoMatch;

  StartLoc = getLexer().getTok().getLoc();
  EndLoc = getLexer().getTok().getEndLoc();
  getLexer().Lex();
  return ParseStatus::Success;
}

bool TC32AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  ParseStatus Res = tryParseRegister(Reg, StartLoc, EndLoc);
  if (Res.isSuccess())
    return false;
  if (Res.isNoMatch())
    return Error(StartLoc, "invalid register");
  return true;
}

bool TC32AsmParser::parseComma() {
  if (getLexer().getKind() != AsmToken::Comma)
    return Error(getLexer().getTok().getLoc(), "expected ','");
  getLexer().Lex();
  return false;
}

bool TC32AsmParser::parseEOL() {
  return getLexer().getKind() != AsmToken::EndOfStatement &&
         getLexer().getKind() != AsmToken::Eof;
}

bool TC32AsmParser::parseImmediateOperand(OperandVector &Operands) {
  SMLoc S = getLexer().getTok().getLoc();
  if (getLexer().getKind() == AsmToken::Hash)
    getLexer().Lex();

  const MCExpr *Expr = nullptr;
  bool Negate = false;
  if (getLexer().getKind() == AsmToken::Minus) {
    Negate = true;
    getLexer().Lex();
  }

  if (getLexer().getKind() == AsmToken::Integer) {
    int64_t Value = static_cast<int64_t>(getLexer().getTok().getIntVal());
    if (Negate)
      Value = -Value;
    Expr = MCConstantExpr::create(Value, getContext());
    SMLoc E = getLexer().getTok().getEndLoc();
    getLexer().Lex();
    Operands.push_back(TC32Operand::createImm(Expr, S, E));
    return false;
  }

  if (Negate)
    return Error(S, "expected integer immediate");
  if (getParser().parseExpression(Expr))
    return true;
  Operands.push_back(
      TC32Operand::createImm(Expr, S, getLexer().getTok().getLoc()));
  return false;
}

bool TC32AsmParser::parseOperand(OperandVector &Operands) {
  MCRegister Reg;
  SMLoc S, E;
  if (tryParseRegister(Reg, S, E).isSuccess()) {
    Operands.push_back(TC32Operand::createReg(Reg, S, E));
    return false;
  }
  return parseImmediateOperand(Operands);
}

bool TC32AsmParser::parseFixedPushPopOperands(StringRef Mnemonic,
                                              OperandVector &Operands) {
  if (getLexer().getKind() != AsmToken::LCurly)
    return Error(getLexer().getTok().getLoc(), "expected '{'");
  getLexer().Lex();

  MCRegister First, Second;
  SMLoc S, E;
  if (parseRegister(First, S, E))
    return true;
  if (parseComma())
    return true;
  if (parseRegister(Second, S, E))
    return true;
  if (getLexer().getKind() != AsmToken::RCurly)
    return Error(getLexer().getTok().getLoc(), "expected '}'");
  getLexer().Lex();

  if (Mnemonic == "tpush" && First == TC32::R7 && Second == TC32::R14)
    return false;
  if (Mnemonic == "tpop" && First == TC32::R7 && Second == TC32::R15)
    return false;
  return Error(S, "unsupported register list for push/pop");
}

bool TC32AsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc, OperandVector &Operands) {
  Operands.push_back(TC32Operand::createToken(Name, NameLoc));

  std::string Mnemonic = StringRef(Name).lower();
  if (Mnemonic == "tpush" || Mnemonic == "tpop") {
    if (parseFixedPushPopOperands(Mnemonic, Operands))
      return true;
    if (getLexer().isNot(AsmToken::EndOfStatement)) {
      SMLoc Loc = getLexer().getLoc();
      getParser().eatToEndOfStatement();
      return Error(Loc, "unexpected token");
    }
    getParser().Lex();
    return false;
  }

  if (getLexer().getKind() == AsmToken::EndOfStatement)
    return false;

  while (true) {
    if (parseOperand(Operands))
      return true;
    if (getLexer().getKind() != AsmToken::Comma)
      break;
    getLexer().Lex();
  }
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }
  getParser().Lex();
  return false;
}

bool TC32AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  if (Operands.empty())
    return Error(IDLoc, "missing instruction");

  auto &Mnemonic = static_cast<TC32Operand &>(*Operands[0]);
  std::string Name = Mnemonic.Tok.lower();
  MCInst Inst;

  if (Name == "tpush") {
    Inst.setOpcode(TC32::TPUSH_R7_LR);
  } else if (Name == "tpop") {
    Inst.setOpcode(TC32::TPOP_R7_PC);
  } else if (Name == "tjl") {
    if (Operands.size() != 2)
      return Error(IDLoc, "expected one operand");
    auto &Target = static_cast<TC32Operand &>(*Operands[1]);
    if (!Target.isImm())
      return Error(IDLoc, "invalid operand for tjl");
    Inst.setOpcode(TC32::TJL);
    Inst.addOperand(MCOperand::createExpr(Target.Expr));
  } else if (Name == "tjeq" || Name == "tjne" || Name == "tjge" ||
             Name == "tjlt" || Name == "tjgt" || Name == "tjle" ||
             Name == "tj") {
    if (Operands.size() != 2)
      return Error(IDLoc, "expected one operand");
    auto &Target = static_cast<TC32Operand &>(*Operands[1]);
    if (!Target.isImm())
      return Error(IDLoc, "invalid branch target");
    Inst.setOpcode(Name == "tjeq" ? TC32::TJEQ
                  : Name == "tjne" ? TC32::TJNE
                  : Name == "tjge" ? TC32::TJGE
                  : Name == "tjlt" ? TC32::TJLT
                  : Name == "tjgt" ? TC32::TJGT
                  : Name == "tjle" ? TC32::TJLE
                                   : TC32::TJ);
    Inst.addOperand(MCOperand::createExpr(Target.Expr));
  } else if (Name == "tmov") {
    if (Operands.size() != 3)
      return Error(IDLoc, "expected two operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &Src = static_cast<TC32Operand &>(*Operands[2]);
    if (!Dst.isReg())
      return Error(IDLoc, "invalid operands for tmov");
    if (Src.isReg()) {
      Inst.setOpcode(TC32::TMOVrr);
      Inst.addOperand(MCOperand::createReg(Dst.getReg()));
      Inst.addOperand(MCOperand::createReg(Src.getReg()));
    } else if (Src.isImm()) {
      int64_t Imm;
      if (!isLoReg(Dst.getReg()))
        return Error(IDLoc, "lo register required");
      if (!extractAbsoluteImm(Src.Expr, Imm) || Imm < 0 || Imm > 255)
        return Error(IDLoc, "tmov immediate must be in range 0..255");
      Inst.setOpcode(TC32::TMOVi8);
      Inst.addOperand(MCOperand::createReg(Dst.getReg()));
      Inst.addOperand(MCOperand::createImm(Imm));
    } else {
      return Error(IDLoc, "invalid operands for tmov");
    }
  } else if (Name == "tneg" || Name == "tmovn") {
    if (Operands.size() != 3)
      return Error(IDLoc, "expected two operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &Src = static_cast<TC32Operand &>(*Operands[2]);
    if (!Dst.isReg() || !Src.isReg() || !isLoReg(Dst.getReg()) ||
        !isLoReg(Src.getReg()))
      return Error(IDLoc, "lo register required");
    Inst.setOpcode(Name == "tneg" ? TC32::TNEGrr : TC32::TMOVNrr);
    Inst.addOperand(MCOperand::createReg(Dst.getReg()));
    Inst.addOperand(MCOperand::createReg(Src.getReg()));
  } else if (Name == "tadds" || Name == "tadd") {
    if (Operands.size() == 3) {
      auto &Op1 = static_cast<TC32Operand &>(*Operands[1]);
      auto &Op2 = static_cast<TC32Operand &>(*Operands[2]);
      if (Op1.isReg() && Op1.getReg() == TC32::R13 && Op2.isImm()) {
        int64_t Imm;
        if (!extractAbsoluteImm(Op2.Expr, Imm) || Imm < 0 || Imm > 508 ||
            (Imm & 3) != 0)
          return Error(IDLoc,
                       "tadd sp immediate must be a multiple of 4 in range 0..508");
        Inst.setOpcode(TC32::TADDspu8);
        Inst.addOperand(MCOperand::createImm(Imm));
      } else {
        return Error(IDLoc, "unsupported tadd form");
      }
    } else if (Operands.size() == 4) {
      auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
      auto &Src = static_cast<TC32Operand &>(*Operands[2]);
      auto &Op3 = static_cast<TC32Operand &>(*Operands[3]);
      int64_t Imm;
      if (!Dst.isReg() || !Src.isReg())
        return Error(IDLoc, "invalid operands for tadd");
      if (Op3.isReg()) {
        if (!isLoReg(Dst.getReg()) || !isLoReg(Src.getReg()) ||
            !isLoReg(Op3.getReg()))
          return Error(IDLoc, "lo register required");
        Inst.setOpcode(TC32::TADDSrrr);
        Inst.addOperand(MCOperand::createReg(Dst.getReg()));
        Inst.addOperand(MCOperand::createReg(Src.getReg()));
        Inst.addOperand(MCOperand::createReg(Op3.getReg()));
        goto emit_inst;
      }
      if (!Op3.isImm() || !extractAbsoluteImm(Op3.Expr, Imm) || Imm < 0)
        return Error(IDLoc, "tadd immediate must be non-negative");
      if (Src.getReg() == TC32::R13) {
        if (!isLoReg(Dst.getReg()))
          return Error(IDLoc, "lo register required");
        if (Imm > 252 || (Imm & 3) != 0)
          return Error(IDLoc,
                       "tadd dst, sp immediate must be a multiple of 4 in range 0..252");
        Inst.setOpcode(TC32::TADDdstspu8);
        Inst.addOperand(MCOperand::createReg(Dst.getReg()));
        Inst.addOperand(MCOperand::createImm(Imm));
      } else {
        if (!isLoReg(Dst.getReg()) || !isLoReg(Src.getReg()))
          return Error(IDLoc, "lo register required");
        if (Dst.getReg() == Src.getReg()) {
          if (Imm > 255)
            return Error(IDLoc,
                         "tadd same-register immediate must be in range 0..255");
        } else if (Imm > 7) {
          return Error(IDLoc,
                       "tadd dst, src immediate must be in range 0..7 unless dst == src");
        }
        Inst.setOpcode(TC32::TADDSrru8);
        Inst.addOperand(MCOperand::createReg(Dst.getReg()));
        Inst.addOperand(MCOperand::createReg(Src.getReg()));
        Inst.addOperand(MCOperand::createImm(Imm));
      }
    } else {
      return Error(IDLoc, "unsupported tadd form");
    }
  } else if (Name == "taddc" || Name == "tsubc") {
    if (Operands.size() != 3 && Operands.size() != 4)
      return Error(IDLoc, "expected two or three operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &RHS = static_cast<TC32Operand &>(*Operands[Operands.size() - 1]);
    if (!Dst.isReg() || !RHS.isReg() || !isLoReg(Dst.getReg()) ||
        !isLoReg(RHS.getReg()))
      return Error(IDLoc, "lo register required");
    if (Operands.size() == 4) {
      auto &LHS = static_cast<TC32Operand &>(*Operands[2]);
      if (!LHS.isReg() || !isLoReg(LHS.getReg()))
        return Error(IDLoc, "lo register required");
      if (Dst.getReg() != LHS.getReg())
        return Error(IDLoc, "first source must match destination");
    }
    Inst.setOpcode(Name == "taddc" ? TC32::TADDCrr : TC32::TSUBCrr);
    Inst.addOperand(MCOperand::createReg(Dst.getReg()));
    Inst.addOperand(MCOperand::createReg(RHS.getReg()));
  } else if (Name == "tand" || Name == "tor" || Name == "txor") {
    if (Operands.size() != 3 && Operands.size() != 4)
      return Error(IDLoc, "expected two or three operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &RHS = static_cast<TC32Operand &>(*Operands[Operands.size() - 1]);
    if (!Dst.isReg() || !RHS.isReg() || !isLoReg(Dst.getReg()) ||
        !isLoReg(RHS.getReg()))
      return Error(IDLoc, "lo register required");
    if (Operands.size() == 4) {
      auto &LHS = static_cast<TC32Operand &>(*Operands[2]);
      if (!LHS.isReg() || !isLoReg(LHS.getReg()))
        return Error(IDLoc, "lo register required");
      if (Dst.getReg() != LHS.getReg())
        return Error(IDLoc, "first source must match destination");
    }
    Inst.setOpcode(Name == "tand" ? TC32::TANDrr
                  : Name == "tor" ? TC32::TORrr
                                   : TC32::TXORrr);
    Inst.addOperand(MCOperand::createReg(Dst.getReg()));
    Inst.addOperand(MCOperand::createReg(RHS.getReg()));
  } else if (Name == "tshftr" || Name == "tasr") {
    if (Operands.size() != 4)
      return Error(IDLoc, "expected three operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &Src = static_cast<TC32Operand &>(*Operands[2]);
    auto &ImmOp = static_cast<TC32Operand &>(*Operands[3]);
    int64_t Imm;
    if (!Dst.isReg() || !Src.isReg() || !ImmOp.isImm() || !isLoReg(Dst.getReg()) ||
        !isLoReg(Src.getReg()) || !extractAbsoluteImm(ImmOp.Expr, Imm) || Imm < 0 ||
        Imm > 31)
      return Error(IDLoc, "shift immediate must be in range 0..31");
    Inst.setOpcode(Name == "tshftr" ? (Imm == 31 ? TC32::TSHFTRi31 : TC32::TSHFTRi5)
                                    : (Imm == 31 ? TC32::TASRi31 : TC32::TASRi5));
    Inst.addOperand(MCOperand::createReg(Dst.getReg()));
    Inst.addOperand(MCOperand::createReg(Src.getReg()));
    if (Imm != 31)
      Inst.addOperand(MCOperand::createImm(Imm));
  } else if (Name == "tshftl") {
    if (Operands.size() != 4)
      return Error(IDLoc, "expected three operands");
    auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
    auto &Src = static_cast<TC32Operand &>(*Operands[2]);
    auto &ImmOp = static_cast<TC32Operand &>(*Operands[3]);
    int64_t Imm;
    if (!Dst.isReg() || !Src.isReg() || !ImmOp.isImm() || !isLoReg(Dst.getReg()) ||
        !isLoReg(Src.getReg()) || !extractAbsoluteImm(ImmOp.Expr, Imm) || Imm < 0 ||
        Imm > 31)
      return Error(IDLoc, "shift immediate must be in range 0..31");
    Inst.setOpcode(TC32::TSHFTLi5);
    Inst.addOperand(MCOperand::createReg(Dst.getReg()));
    Inst.addOperand(MCOperand::createReg(Src.getReg()));
    Inst.addOperand(MCOperand::createImm(Imm));
  } else if (Name == "tsub") {
    if (Operands.size() == 3) {
      auto &Op1 = static_cast<TC32Operand &>(*Operands[1]);
      auto &Op2 = static_cast<TC32Operand &>(*Operands[2]);
      int64_t Imm;
      if (!Op1.isReg() || Op1.getReg() != TC32::R13 || !Op2.isImm() ||
          !extractAbsoluteImm(Op2.Expr, Imm) || Imm < 0 || Imm > 508 ||
          (Imm & 3) != 0)
        return Error(IDLoc, "unsupported tsub form");
      Inst.setOpcode(TC32::TSUBspu8);
      Inst.addOperand(MCOperand::createImm(Imm));
    } else if (Operands.size() == 4) {
      auto &Dst = static_cast<TC32Operand &>(*Operands[1]);
      auto &Src = static_cast<TC32Operand &>(*Operands[2]);
      auto &Op3 = static_cast<TC32Operand &>(*Operands[3]);
      int64_t Imm;
      if (!Dst.isReg() || !Src.isReg() || !isLoReg(Dst.getReg()) ||
          !isLoReg(Src.getReg()))
        return Error(IDLoc, "lo register required");
      if (Op3.isReg()) {
        if (!isLoReg(Op3.getReg()))
          return Error(IDLoc, "lo register required");
        Inst.setOpcode(TC32::TSUBrrr);
        Inst.addOperand(MCOperand::createReg(Dst.getReg()));
        Inst.addOperand(MCOperand::createReg(Src.getReg()));
        Inst.addOperand(MCOperand::createReg(Op3.getReg()));
      } else if (Op3.isImm() && extractAbsoluteImm(Op3.Expr, Imm) && Imm == 1) {
        Inst.setOpcode(TC32::TSUBri1);
        Inst.addOperand(MCOperand::createReg(Dst.getReg()));
        Inst.addOperand(MCOperand::createReg(Src.getReg()));
      } else {
        return Error(IDLoc, "unsupported tsub form");
      }
    } else {
      return Error(IDLoc, "unsupported tsub form");
    }
  } else if (Name == "tcmp") {
    if (Operands.size() != 3)
      return Error(IDLoc, "expected two operands");
    auto &Op1 = static_cast<TC32Operand &>(*Operands[1]);
    auto &Op2 = static_cast<TC32Operand &>(*Operands[2]);
    int64_t Imm;
    if (!Op1.isReg())
      return Error(IDLoc, "invalid operands for tcmp");
    if (Op2.isReg()) {
      if (!isLoReg(Op1.getReg()) || !isLoReg(Op2.getReg()))
        return Error(IDLoc, "lo register required");
      Inst.setOpcode(TC32::TCMPrr);
      Inst.addOperand(MCOperand::createReg(Op1.getReg()));
      Inst.addOperand(MCOperand::createReg(Op2.getReg()));
    } else if (Op2.isImm()) {
      if (!isLoReg(Op1.getReg()))
        return Error(IDLoc, "lo register required");
      if (!extractAbsoluteImm(Op2.Expr, Imm) || Imm != 0)
        return Error(IDLoc, "only tcmp reg, #0 is supported");
      Inst.setOpcode(TC32::TCMPri0);
      Inst.addOperand(MCOperand::createReg(Op1.getReg()));
    } else {
      return Error(IDLoc, "invalid operands for tcmp");
    }
  } else {
    return Error(IDLoc, "invalid instruction mnemonic");
  }

emit_inst:
  Inst.setLoc(IDLoc);
  Out.emitInstruction(Inst, getSTI());
  return false;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTC32AsmParser() {
  RegisterMCAsmParser<TC32AsmParser> X(getTheTC32Target());
}
