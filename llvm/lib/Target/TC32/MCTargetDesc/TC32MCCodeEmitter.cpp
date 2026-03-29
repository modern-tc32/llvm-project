#include "TC32FixupKinds.h"
#include "TC32MCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class TC32MCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

  unsigned getRegEncoding(MCRegister Reg) const {
    return Ctx.getRegisterInfo()->getEncodingValue(Reg);
  }

  static void check(bool Cond, const char *Msg) {
    if (!Cond)
      report_fatal_error(Twine("invalid TC32 instruction encoding: ") + Msg);
  }

  static void addFixup(SmallVectorImpl<MCFixup> &Fixups, unsigned Offset,
                       const MCExpr *Expr, MCFixupKind Kind) {
    Fixups.push_back(MCFixup::create(Offset, Expr, Kind));
  }

  uint16_t encodeTCMPri0(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    check(Src < 8, "tcmp immediate form requires low register");
    return static_cast<uint16_t>(((0xA8u + Src) << 8));
  }

  uint16_t encodeTCMPrr(const MCInst &MI) const {
    unsigned LHS = getRegEncoding(MI.getOperand(0).getReg());
    unsigned RHS = getRegEncoding(MI.getOperand(1).getReg());
    check(LHS < 8 && RHS < 8, "tcmp register form requires low registers");
    return static_cast<uint16_t>(0x0280u | (RHS << 3) | LHS);
  }

  uint16_t encodeTMOVrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());

    if (Dst < 8 && Src < 8)
      return static_cast<uint16_t>(0xEC00u | ((Src << 3) | Dst));

    uint16_t Lo = 0x00;
    if (Dst & 0x8)
      Lo |= 0x80;
    if (Src & 0x8)
      Lo |= 0x40;
    Lo |= static_cast<uint16_t>((Src & 0x7) << 3);
    Lo |= static_cast<uint16_t>(Dst & 0x7);
    return static_cast<uint16_t>(0x0600u | Lo);
  }

  uint16_t encodeTADDSrru8(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();

    check(Dst < 8 && Src < 8, "tadds requires low registers");
    check(Imm >= 0, "negative immediate");

    if (Dst == Src) {
      check(Imm <= 255, "same-register tadds immediate out of range");
      return static_cast<uint16_t>(((0xB0u + Dst) << 8) |
                                   static_cast<uint16_t>(Imm));
    }

    check(Imm <= 7, "three-operand tadds immediate out of range");
    uint16_t Lo = static_cast<uint16_t>((Src << 3) | Dst | ((Imm & 0x3) << 6));
    uint16_t Hi = static_cast<uint16_t>(0xECu + (Imm >> 2));
    return static_cast<uint16_t>((Hi << 8) | Lo);
  }

  uint16_t encodeTMOVi8(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Dst < 8, "tmovs requires low destination register");
    check(Imm >= 0 && Imm <= 255, "tmovs immediate out of range");
    return static_cast<uint16_t>(((0xA0u + Dst) << 8) |
                                 static_cast<uint16_t>(Imm));
  }

  uint16_t encodeTADDSrrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    unsigned Rhs = getRegEncoding(MI.getOperand(2).getReg());

    check(Dst < 8 && Src < 8 && Rhs < 8, "tadds register form requires low registers");
    unsigned Enc = Dst | (Src << 3) | (Rhs << 6);
    return static_cast<uint16_t>(((0xE8u + (Enc >> 8)) << 8) |
                                 static_cast<uint16_t>(Enc & 0xFF));
  }

  uint16_t encodeTNEGrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Src < 8, "tneg requires low registers");
    return static_cast<uint16_t>(0x0240u | (Src << 3) | Dst);
  }

  uint16_t encodeTMOVNrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Src < 8, "tmovn requires low registers");
    return static_cast<uint16_t>(0x03C0u | (Src << 3) | Dst);
  }

  uint16_t encodeTADDCrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned RHS = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && RHS < 8, "taddc requires low registers");
    return static_cast<uint16_t>(0x0140u | (RHS << 3) | Dst);
  }

  uint16_t encodeTLogic2Addr(const MCInst &MI, uint16_t Base) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned RHS = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && RHS < 8, "logic op requires low registers");
    return static_cast<uint16_t>(Base | (RHS << 3) | Dst);
  }

  uint16_t encodeTSUBrrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    unsigned RHS = getRegEncoding(MI.getOperand(2).getReg());
    check(Dst < 8 && Src < 8 && RHS < 8, "tsub requires low registers");
    unsigned Enc = Dst | (Src << 3) | (RHS << 6);
    return static_cast<uint16_t>(((0xEAu + (Enc >> 8)) << 8) |
                                 static_cast<uint16_t>(Enc & 0xFF));
  }

  uint16_t encodeTSUBri1(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Src < 8, "tsub immediate form requires low registers");
    return static_cast<uint16_t>(0xEE40u | (Src << 3) | Dst);
  }

  uint16_t encodeTSUBCrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned RHS = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && RHS < 8, "tsubc requires low registers");
    return static_cast<uint16_t>(0x0180u | (RHS << 3) | Dst);
  }

  uint16_t encodeTSHFTRi31(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Src < 8, "tshftr requires low registers");
    return static_cast<uint16_t>(0xFFC0u | (Src << 3) | Dst);
  }

  uint16_t encodeTASRi31(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Src < 8, "tasr requires low registers");
    return static_cast<uint16_t>(0xE7C0u | (Src << 3) | Dst);
  }

  uint16_t encodeShiftImm5(const MCInst &MI, uint16_t Base) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Src = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();
    check(Dst < 8 && Src < 8, "shift requires low registers");
    check(Imm >= 0 && Imm <= 31, "shift immediate out of range");
    return static_cast<uint16_t>(Base | ((static_cast<uint16_t>(Imm) >> 2) << 8) |
                                 ((static_cast<uint16_t>(Imm) & 0x3) << 6) |
                                 (Src << 3) | Dst);
  }

  uint16_t encodeTLOADspu8(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Dst < 8, "tloadr sp form requires low destination register");
    check(Imm >= 0 && Imm <= 1020 && (Imm & 3) == 0,
          "tloadr sp immediate must be 0..1020 step 4");
    return static_cast<uint16_t>(((0x38u + Dst) << 8) |
                                 static_cast<uint16_t>(Imm >> 2));
  }

  uint16_t encodeTLOADfpu8(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Dst < 8, "tloadr fp form requires low destination register");
    check(Imm >= 0 && Imm <= 252 && (Imm & 3) == 0,
          "tloadr fp immediate must be 0..252 step 4");
    unsigned Scaled = static_cast<unsigned>(Imm >> 2);
    return static_cast<uint16_t>(((0x58u + (Scaled >> 2)) << 8) |
                                 (0x38u + Dst + ((Scaled & 0x3) << 6)));
  }

  uint16_t encodeTLOADpcu8(const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    check(Dst < 8, "tloadr pc form requires low destination register");

    const MCOperand &ImmOp = MI.getOperand(1);
    if (ImmOp.isExpr()) {
      addFixup(Fixups, 0, ImmOp.getExpr(),
               static_cast<MCFixupKind>(TC32::fixup_tc32_ldr_pcrel_u8));
      return static_cast<uint16_t>(((0x08u + Dst) << 8));
    }

    int64_t Imm = ImmOp.getImm();
    check(Imm >= 0 && Imm <= 1020 && (Imm & 3) == 0,
          "tloadr pc immediate must be 0..1020 step 4");
    return static_cast<uint16_t>(((0x08u + Dst) << 8) |
                                 static_cast<uint16_t>(Imm >> 2));
  }

  uint16_t encodeTLOADrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Base < 8, "tloadr [reg] form requires low registers");
    return static_cast<uint16_t>(0x5800u | (Base << 3) | Dst);
  }

  uint16_t encodeTLOADru3(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();
    check(Dst < 8 && Base < 8, "tloadr [reg,#imm] form requires low registers");
    check(Imm >= 0 && Imm <= 28 && (Imm & 3) == 0,
          "tloadr [reg,#imm] word offset must be 0..28 step 4");
    return static_cast<uint16_t>(0x5800u | (((unsigned)Imm >> 2) << 6) |
                                 (Base << 3) | Dst);
  }

  uint16_t encodeTLOADBrr(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    check(Dst < 8 && Base < 8, "tloadrb [reg] form requires low registers");
    return static_cast<uint16_t>(0x4800u | (Base << 3) | Dst);
  }

  uint16_t encodeTLOADBru3(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();
    check(Dst < 8 && Base < 8, "tloadrb [reg,#imm] form requires low registers");
    check(Imm >= 0 && Imm <= 7, "tloadrb [reg,#imm] byte offset must be 0..7");
    return static_cast<uint16_t>(0x4800u | ((unsigned)Imm << 6) |
                                 (Base << 3) | Dst);
  }

  uint16_t encodeTSTOREspu8(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Src < 8, "tstorer sp form requires low source register");
    check(Imm >= 0 && Imm <= 1020 && (Imm & 3) == 0,
          "tstorer sp immediate must be 0..1020 step 4");
    return static_cast<uint16_t>(((0x30u + Src) << 8) |
                                 static_cast<uint16_t>(Imm >> 2));
  }

  uint16_t encodeTSTOREfpu8(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Src < 8, "tstorer fp form requires low source register");
    check(Imm >= 0 && Imm <= 252 && (Imm & 3) == 0,
          "tstorer fp immediate must be 0..252 step 4");
    unsigned Scaled = static_cast<unsigned>(Imm >> 2);
    return static_cast<uint16_t>(((0x50u + (Scaled >> 2)) << 8) |
                                 (0x38u + Src + ((Scaled & 0x3) << 6)));
  }

  uint16_t encodeTSTORErr(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    check(Src < 8 && Base < 8, "tstorer [reg] form requires low registers");
    return static_cast<uint16_t>(0x5000u | (Base << 3) | Src);
  }

  uint16_t encodeTSTOREru3(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();
    check(Src < 8 && Base < 8, "tstorer [reg,#imm] form requires low registers");
    check(Imm >= 0 && Imm <= 28 && (Imm & 3) == 0,
          "tstorer [reg,#imm] word offset must be 0..28 step 4");
    return static_cast<uint16_t>(0x5000u | (((unsigned)Imm >> 2) << 6) |
                                 (Base << 3) | Src);
  }

  uint16_t encodeTSTOREBrr(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    check(Src < 8 && Base < 8, "tstorerb [reg] form requires low registers");
    return static_cast<uint16_t>(0x4000u | (Base << 3) | Src);
  }

  uint16_t encodeTSTOREBru3(const MCInst &MI) const {
    unsigned Src = getRegEncoding(MI.getOperand(0).getReg());
    unsigned Base = getRegEncoding(MI.getOperand(1).getReg());
    int64_t Imm = MI.getOperand(2).getImm();
    check(Src < 8 && Base < 8, "tstorerb [reg,#imm] form requires low registers");
    check(Imm >= 0 && Imm <= 7, "tstorerb [reg,#imm] byte offset must be 0..7");
    return static_cast<uint16_t>(0x4000u | ((unsigned)Imm << 6) |
                                 (Base << 3) | Src);
  }

  uint16_t encodeTADDspu8(const MCInst &MI) const {
    int64_t Imm = MI.getOperand(0).getImm();
    check(Imm >= 0 && Imm <= 508 && (Imm & 3) == 0,
          "tadd sp immediate must be 0..508 step 4");
    return static_cast<uint16_t>(0x6000u | static_cast<uint16_t>(Imm >> 2));
  }

  uint16_t encodeTSUBspu8(const MCInst &MI) const {
    int64_t Imm = MI.getOperand(0).getImm();
    check(Imm >= 0 && Imm <= 508 && (Imm & 3) == 0,
          "tsub sp immediate must be 0..508 step 4");
    return static_cast<uint16_t>(0x6000u | 0x0080u |
                                 static_cast<uint16_t>(Imm >> 2));
  }

  uint16_t encodeTADDdstspu8(const MCInst &MI) const {
    unsigned Dst = getRegEncoding(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();

    check(Dst < 8, "tadd dst, sp requires low destination register");
    check(Imm >= 0 && Imm <= 252 && (Imm & 3) == 0,
          "tadd dst, sp immediate must be 0..252 step 4");
    return static_cast<uint16_t>(((0x78u + Dst) << 8) |
                                 static_cast<uint16_t>(Imm >> 2));
  }

public:
  TC32MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override {
    (void)STI;

    uint16_t Bits = 0;
    switch (MI.getOpcode()) {
    case TC32::TPUSH_R0_R1_R2_R3:
      Bits = 0x640F;
      break;
    case TC32::TPUSH_LR:
      Bits = 0x6500;
      break;
    case TC32::TPUSH_R7_LR:
      Bits = 0x6580;
      break;
    case TC32::TPOP_R7:
      Bits = 0x6C80;
      break;
    case TC32::TPOP_R3:
      Bits = 0x6C08;
      break;
    case TC32::TPOP_PC:
      Bits = 0x6D00;
      break;
    case TC32::TPOP_R7_PC:
      Bits = 0x6D80;
      break;
    case TC32::TJL:
    case TC32::TJL_R0:
    case TC32::TJL_R0_R1:
    case TC32::TJL_R0_R1_R2:
    case TC32::TJL_R0_R1_R2_R3: {
      const MCOperand &Op = MI.getOperand(0);
      if (Op.isExpr()) {
        addFixup(Fixups, 0, Op.getExpr(), static_cast<MCFixupKind>(TC32::fixup_tc32_call));
        CB.push_back(static_cast<char>(0xFF));
        CB.push_back(static_cast<char>(0x97));
        CB.push_back(static_cast<char>(0xFE));
        CB.push_back(static_cast<char>(0x9F));
        return;
      }
      report_fatal_error("unhandled TC32 tjl operand kind");
    }
    case TC32::TCMPri0:
      Bits = encodeTCMPri0(MI);
      break;
    case TC32::TCMPrr:
      Bits = encodeTCMPrr(MI);
      break;
    case TC32::TJEQ:
    case TC32::TJNE:
    case TC32::TJCS:
    case TC32::TJCC:
    case TC32::TJGE:
    case TC32::TJLT:
    case TC32::TJHI:
    case TC32::TJLS:
    case TC32::TJGT:
    case TC32::TJLE:
    case TC32::TJ: {
      const MCOperand &Op = MI.getOperand(0);
      if (!Op.isExpr())
        report_fatal_error("unhandled TC32 branch operand kind");
      MCFixupKind Kind =
          MI.getOpcode() == TC32::TJ ? static_cast<MCFixupKind>(TC32::fixup_tc32_jump11)
                                     : static_cast<MCFixupKind>(TC32::fixup_tc32_branch8);
      addFixup(Fixups, 0, Op.getExpr(), Kind);
      switch (MI.getOpcode()) {
      case TC32::TJEQ: Bits = 0xC000; break;
      case TC32::TJNE: Bits = 0xC100; break;
      case TC32::TJCS: Bits = 0xC200; break;
      case TC32::TJCC: Bits = 0xC300; break;
      case TC32::TJGE: Bits = 0xCA00; break;
      case TC32::TJLT: Bits = 0xCB00; break;
      case TC32::TJHI: Bits = 0xC800; break;
      case TC32::TJLS: Bits = 0xC900; break;
      case TC32::TJGT: Bits = 0xCC00; break;
      case TC32::TJLE: Bits = 0xCD00; break;
      case TC32::TJ:   Bits = 0x8000; break;
      default: llvm_unreachable("unexpected tc32 branch opcode");
      }
      break;
    }
    case TC32::TRET:
    case TC32::TRET_R0:
      Bits = 0x06F7;
      break;
    case TC32::TJEXr:
      Bits = static_cast<uint16_t>(0x0700u |
                                   (getRegEncoding(MI.getOperand(0).getReg()) << 4));
      break;
    case TC32::TMOVrr:
      Bits = encodeTMOVrr(MI);
      break;
    case TC32::TMOVi8:
      Bits = encodeTMOVi8(MI);
      break;
    case TC32::TNEGrr:
      Bits = encodeTNEGrr(MI);
      break;
    case TC32::TMOVNrr:
      Bits = encodeTMOVNrr(MI);
      break;
    case TC32::TADDSrru8:
    case TC32::TADDSri8:
      Bits = encodeTADDSrru8(MI);
      break;
    case TC32::TADDSrrr:
      Bits = encodeTADDSrrr(MI);
      break;
    case TC32::TADDCrr:
      Bits = encodeTADDCrr(MI);
      break;
    case TC32::TANDrr:
      Bits = encodeTLogic2Addr(MI, 0x0000u);
      break;
    case TC32::TORrr:
      Bits = encodeTLogic2Addr(MI, 0x0300u);
      break;
    case TC32::TXORrr:
      Bits = encodeTLogic2Addr(MI, 0x0040u);
      break;
    case TC32::TMULrr:
      Bits = encodeTLogic2Addr(MI, 0x0340u);
      break;
    case TC32::TSUBrrr:
      Bits = encodeTSUBrrr(MI);
      break;
    case TC32::TSUBri1:
      Bits = encodeTSUBri1(MI);
      break;
    case TC32::TSUBCrr:
      Bits = encodeTSUBCrr(MI);
      break;
    case TC32::TSHFTRi31:
      Bits = encodeTSHFTRi31(MI);
      break;
    case TC32::TASRi31:
      Bits = encodeTASRi31(MI);
      break;
    case TC32::TSHFTLi5:
      Bits = encodeShiftImm5(MI, 0xF000u);
      break;
    case TC32::TSHFTRi5:
      Bits = encodeShiftImm5(MI, 0xF800u);
      break;
    case TC32::TASRi5:
      Bits = encodeShiftImm5(MI, 0xE000u);
      break;
    case TC32::TLOADspu8:
      Bits = encodeTLOADspu8(MI);
      break;
    case TC32::TLOADfpu8:
      Bits = encodeTLOADfpu8(MI);
      break;
    case TC32::TLOADpcu8:
      Bits = encodeTLOADpcu8(MI, Fixups);
      break;
    case TC32::TLOADrr:
      Bits = encodeTLOADrr(MI);
      break;
    case TC32::TLOADru3:
      Bits = encodeTLOADru3(MI);
      break;
    case TC32::TLOADBrr:
      Bits = encodeTLOADBrr(MI);
      break;
    case TC32::TLOADBru3:
      Bits = encodeTLOADBru3(MI);
      break;
    case TC32::TSTOREspu8:
      Bits = encodeTSTOREspu8(MI);
      break;
    case TC32::TSTOREfpu8:
      Bits = encodeTSTOREfpu8(MI);
      break;
    case TC32::TSTORErr:
      Bits = encodeTSTORErr(MI);
      break;
    case TC32::TSTOREru3:
      Bits = encodeTSTOREru3(MI);
      break;
    case TC32::TSTOREBrr:
      Bits = encodeTSTOREBrr(MI);
      break;
    case TC32::TSTOREBru3:
      Bits = encodeTSTOREBru3(MI);
      break;
    case TC32::TADDspu8:
      Bits = encodeTADDspu8(MI);
      break;
    case TC32::TSUBspu8:
      Bits = encodeTSUBspu8(MI);
      break;
    case TC32::TADDdstspu8:
      Bits = encodeTADDdstspu8(MI);
      break;
    default:
      report_fatal_error(Twine("unhandled TC32 opcode in code emitter: ") +
                         Twine(MCII.getName(MI.getOpcode())));
    }

    CB.push_back(static_cast<char>(Bits & 0xFF));
    CB.push_back(static_cast<char>((Bits >> 8) & 0xFF));
  }
};

} // namespace

MCCodeEmitter *llvm::createTC32MCCodeEmitter(const MCInstrInfo &MCII,
                                             MCContext &Ctx) {
  return new TC32MCCodeEmitter(MCII, Ctx);
}
