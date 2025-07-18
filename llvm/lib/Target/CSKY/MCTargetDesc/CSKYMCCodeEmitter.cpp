//===-- CSKYMCCodeEmitter.cpp - CSKY Code Emitter interface ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the CSKYMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "CSKYMCAsmInfo.h"
#include "MCTargetDesc/CSKYFixupKinds.h"
#include "MCTargetDesc/CSKYMCAsmInfo.h"
#include "MCTargetDesc/CSKYMCTargetDesc.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

#define DEBUG_TYPE "csky-mccode-emitter"

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

static void addFixup(SmallVectorImpl<MCFixup> &Fixups, uint32_t Offset,
                     const MCExpr *Value, uint16_t Kind) {
  bool PCRel = false;
  switch (Kind) {
  case CSKY::Fixups::fixup_csky_pcrel_imm16_scale2:
  case CSKY::Fixups::fixup_csky_pcrel_uimm16_scale4:
  case CSKY::Fixups::fixup_csky_pcrel_imm26_scale2:
  case CSKY::Fixups::fixup_csky_pcrel_imm18_scale2:
  case CSKY::Fixups::fixup_csky_gotpc:
  case CSKY::Fixups::fixup_csky_pcrel_imm10_scale2:
  case CSKY::Fixups::fixup_csky_pcrel_uimm7_scale4:
  case CSKY::Fixups::fixup_csky_pcrel_uimm8_scale4:
    PCRel = true;
  }
  Fixups.push_back(MCFixup::create(Offset, Value, Kind, PCRel));
}

namespace {
class CSKYMCCodeEmitter : public MCCodeEmitter {
  MCContext &Ctx;
  const MCInstrInfo &MII;

public:
  CSKYMCCodeEmitter(MCContext &Ctx, const MCInstrInfo &MII)
      : Ctx(Ctx), MII(MII) {}

  ~CSKYMCCodeEmitter() {}

  void encodeInstruction(const MCInst &Inst, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  // Generated by tablegen.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // Default encoding method used by tablegen.
  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  template <int shift = 0>
  unsigned getImmOpValue(const MCInst &MI, unsigned Idx,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    if (MO.isImm())
      return (MO.getImm() >> shift);

    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = getTargetFixup(MO.getExpr());
    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  unsigned getRegSeqImmOpValue(const MCInst &MI, unsigned Idx,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;

  unsigned getRegisterSeqOpValue(const MCInst &MI, unsigned Op,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getOImmOpValue(const MCInst &MI, unsigned Idx,
                          SmallVectorImpl<MCFixup> &Fixups,
                          const MCSubtargetInfo &STI) const;

  unsigned getImmOpValueIDLY(const MCInst &MI, unsigned Idx,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getImmJMPIX(const MCInst &MI, unsigned Idx,
                       SmallVectorImpl<MCFixup> &Fixups,
                       const MCSubtargetInfo &STI) const;

  unsigned getImmOpValueMSBSize(const MCInst &MI, unsigned Idx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  unsigned getImmShiftOpValue(const MCInst &MI, unsigned Idx,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    assert(MO.isImm() && "Unexpected MO type.");
    return 1 << MO.getImm();
  }

  MCFixupKind getTargetFixup(const MCExpr *Expr) const;

  template <llvm::CSKY::Fixups FIXUP>
  unsigned getBranchSymbolOpValue(const MCInst &MI, unsigned Idx,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);

    if (MO.isImm())
      return MO.getImm() >> 1;

    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = MCFixupKind(FIXUP);
    if (MO.getExpr()->getKind() == MCExpr::Specifier)
      Kind = getTargetFixup(MO.getExpr());

    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  template <llvm::CSKY::Fixups FIXUP>
  unsigned getConstpoolSymbolOpValue(const MCInst &MI, unsigned Idx,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = MCFixupKind(FIXUP);
    if (MO.getExpr()->getKind() == MCExpr::Specifier)
      Kind = getTargetFixup(MO.getExpr());

    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  template <llvm::CSKY::Fixups FIXUP>
  unsigned getDataSymbolOpValue(const MCInst &MI, unsigned Idx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = MCFixupKind(FIXUP);
    if (MO.getExpr()->getKind() == MCExpr::Specifier)
      Kind = getTargetFixup(MO.getExpr());

    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  unsigned getCallSymbolOpValue(const MCInst &MI, unsigned Idx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = CSKY::fixup_csky_pcrel_imm26_scale2;
    if (MO.getExpr()->getKind() == MCExpr::Specifier)
      Kind = getTargetFixup(MO.getExpr());

    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  unsigned getBareSymbolOpValue(const MCInst &MI, unsigned Idx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(Idx);
    assert(MO.isExpr() && "Unexpected MO type.");

    MCFixupKind Kind = CSKY::fixup_csky_pcrel_imm18_scale2;
    if (MO.getExpr()->getKind() == MCExpr::Specifier)
      Kind = getTargetFixup(MO.getExpr());

    addFixup(Fixups, 0, MO.getExpr(), Kind);
    return 0;
  }

  void expandJBTF(const MCInst &MI, SmallVectorImpl<char> &CB,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const;
  void expandNEG(const MCInst &MI, SmallVectorImpl<char> &CB,
                 SmallVectorImpl<MCFixup> &Fixups,
                 const MCSubtargetInfo &STI) const;
  void expandRSUBI(const MCInst &MI, SmallVectorImpl<char> &CB,
                   SmallVectorImpl<MCFixup> &Fixups,
                   const MCSubtargetInfo &STI) const;
};
} // namespace

unsigned CSKYMCCodeEmitter::getOImmOpValue(const MCInst &MI, unsigned Idx,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(Idx);
  assert(MO.isImm() && "Unexpected MO type.");
  return MO.getImm() - 1;
}

unsigned
CSKYMCCodeEmitter::getImmOpValueIDLY(const MCInst &MI, unsigned Idx,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(Idx);
  assert(MO.isImm() && "Unexpected MO type.");

  auto V = (MO.getImm() <= 3) ? 4 : MO.getImm();
  return V - 1;
}

unsigned
CSKYMCCodeEmitter::getImmOpValueMSBSize(const MCInst &MI, unsigned Idx,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  const MCOperand &MSB = MI.getOperand(Idx);
  const MCOperand &LSB = MI.getOperand(Idx + 1);
  assert(MSB.isImm() && LSB.isImm() && "Unexpected MO type.");

  return MSB.getImm() - LSB.getImm();
}

static void writeData(uint32_t Bin, unsigned Size, SmallVectorImpl<char> &CB) {
  if (Size == 4)
    support::endian::write(CB, static_cast<uint16_t>(Bin >> 16),
                           llvm::endianness::little);
  support::endian::write(CB, static_cast<uint16_t>(Bin),
                         llvm::endianness::little);
}

void CSKYMCCodeEmitter::expandJBTF(const MCInst &MI, SmallVectorImpl<char> &CB,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const {

  MCInst TmpInst;

  uint32_t Binary;

  TmpInst =
      MCInstBuilder(MI.getOpcode() == CSKY::JBT_E ? CSKY::BF16 : CSKY::BT16)
          .addOperand(MI.getOperand(0))
          .addImm(6);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  writeData(Binary, 2, CB);

  if (!STI.hasFeature(CSKY::Has2E3))
    TmpInst = MCInstBuilder(CSKY::BR32)
                  .addOperand(MI.getOperand(1))
                  .addOperand(MI.getOperand(2));
  else
    TmpInst = MCInstBuilder(CSKY::JMPI32).addOperand(MI.getOperand(2));
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  Fixups[Fixups.size() - 1].setOffset(2);
  writeData(Binary, 4, CB);
}

void CSKYMCCodeEmitter::expandNEG(const MCInst &MI, SmallVectorImpl<char> &CB,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const {

  MCInst TmpInst;
  uint32_t Binary;
  unsigned Size = MI.getOpcode() == CSKY::NEG32 ? 4 : 2;

  TmpInst = MCInstBuilder(Size == 4 ? CSKY::NOT32 : CSKY::NOT16)
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(1));
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  writeData(Binary, Size, CB);

  TmpInst = MCInstBuilder(Size == 4 ? CSKY::ADDI32 : CSKY::ADDI16)
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(0))
                .addImm(1);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  writeData(Binary, Size, CB);
}

void CSKYMCCodeEmitter::expandRSUBI(const MCInst &MI, SmallVectorImpl<char> &CB,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const {

  MCInst TmpInst;
  uint32_t Binary;
  unsigned Size = MI.getOpcode() == CSKY::RSUBI32 ? 4 : 2;

  TmpInst = MCInstBuilder(Size == 4 ? CSKY::NOT32 : CSKY::NOT16)
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(1));
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  writeData(Binary, Size, CB);

  TmpInst = MCInstBuilder(Size == 4 ? CSKY::ADDI32 : CSKY::ADDI16)
                .addOperand(MI.getOperand(0))
                .addOperand(MI.getOperand(0))
                .addImm(MI.getOperand(2).getImm() + 1);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  writeData(Binary, Size, CB);
}

void CSKYMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                          SmallVectorImpl<char> &CB,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  const MCInstrDesc &Desc = MII.get(MI.getOpcode());
  unsigned Size = Desc.getSize();

  MCInst TmpInst;

  switch (MI.getOpcode()) {
  default:
    TmpInst = MI;
    break;
  case CSKY::JBT_E:
  case CSKY::JBF_E:
    expandJBTF(MI, CB, Fixups, STI);
    MCNumEmitted += 2;
    return;
  case CSKY::NEG32:
  case CSKY::NEG16:
    expandNEG(MI, CB, Fixups, STI);
    MCNumEmitted += 2;
    return;
  case CSKY::RSUBI32:
  case CSKY::RSUBI16:
    expandRSUBI(MI, CB, Fixups, STI);
    MCNumEmitted += 2;
    return;
  case CSKY::JBSR32:
    TmpInst = MCInstBuilder(CSKY::BSR32).addOperand(MI.getOperand(0));
    break;
  case CSKY::JBR16:
    TmpInst = MCInstBuilder(CSKY::BR16).addOperand(MI.getOperand(0));
    break;
  case CSKY::JBR32:
    TmpInst = MCInstBuilder(CSKY::BR32).addOperand(MI.getOperand(0));
    break;
  case CSKY::JBT16:
    TmpInst = MCInstBuilder(CSKY::BT16)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1));
    break;
  case CSKY::JBT32:
    TmpInst = MCInstBuilder(CSKY::BT32)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1));
    break;
  case CSKY::JBF16:
    TmpInst = MCInstBuilder(CSKY::BF16)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1));
    break;
  case CSKY::JBF32:
    TmpInst = MCInstBuilder(CSKY::BF32)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1));
    break;
  case CSKY::LRW32_Gen:
    TmpInst = MCInstBuilder(CSKY::LRW32)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(2));
    break;
  case CSKY::LRW16_Gen:
    TmpInst = MCInstBuilder(CSKY::LRW16)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(2));
    break;
  case CSKY::CMPLEI32:
    TmpInst = MCInstBuilder(CSKY::CMPLTI32)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1))
                  .addImm(MI.getOperand(2).getImm() + 1);
    break;
  case CSKY::CMPLEI16:
    TmpInst = MCInstBuilder(CSKY::CMPLTI16)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1))
                  .addImm(MI.getOperand(2).getImm() + 1);
    break;
  case CSKY::ROTRI32:
    TmpInst = MCInstBuilder(CSKY::ROTLI32)
                  .addOperand(MI.getOperand(0))
                  .addOperand(MI.getOperand(1))
                  .addImm(32 - MI.getOperand(2).getImm());
    break;
  case CSKY::BGENI:
    auto V = 1 << MI.getOperand(1).getImm();
    TmpInst =
        MCInstBuilder(CSKY::MOVI32).addOperand(MI.getOperand(0)).addImm(V);
    break;
  }

  ++MCNumEmitted;
  writeData(getBinaryCodeForInstr(TmpInst, Fixups, STI), Size, CB);
}

unsigned
CSKYMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());

  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  llvm_unreachable("Unhandled expression!");
  return 0;
}

unsigned
CSKYMCCodeEmitter::getRegSeqImmOpValue(const MCInst &MI, unsigned Idx,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  assert(MI.getOperand(Idx).isReg() && "Unexpected MO type.");
  assert(MI.getOperand(Idx + 1).isImm() && "Unexpected MO type.");

  unsigned Ry = MI.getOperand(Idx).getReg();
  unsigned Rz = MI.getOperand(Idx + 1).getImm();

  unsigned Imm = Ctx.getRegisterInfo()->getEncodingValue(Rz) -
                 Ctx.getRegisterInfo()->getEncodingValue(Ry);

  return ((Ctx.getRegisterInfo()->getEncodingValue(Ry) << 5) | Imm);
}

unsigned
CSKYMCCodeEmitter::getRegisterSeqOpValue(const MCInst &MI, unsigned Op,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  unsigned Reg1 =
      Ctx.getRegisterInfo()->getEncodingValue(MI.getOperand(Op).getReg());
  unsigned Reg2 =
      Ctx.getRegisterInfo()->getEncodingValue(MI.getOperand(Op + 1).getReg());

  unsigned Binary = ((Reg1 & 0x1f) << 5) | (Reg2 - Reg1);

  return Binary;
}

unsigned CSKYMCCodeEmitter::getImmJMPIX(const MCInst &MI, unsigned Idx,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  switch (MI.getOperand(Idx).getImm()) {
  default:
    llvm_unreachable("Unhandled jmpix imm!");
  case 16:
    return 0;
  case 24:
    return 1;
  case 32:
    return 2;
  case 40:
    return 3;
  }
}

MCFixupKind CSKYMCCodeEmitter::getTargetFixup(const MCExpr *Expr) const {
  const auto *CSKYExpr = cast<MCSpecifierExpr>(Expr);
  switch (CSKYExpr->getSpecifier()) {
  default:
    llvm_unreachable("Unhandled fixup kind!");
  case CSKY::S_ADDR:
    return CSKY::fixup_csky_addr32;
  case CSKY::S_ADDR_HI16:
    return CSKY::fixup_csky_addr_hi16;
  case CSKY::S_ADDR_LO16:
    return CSKY::fixup_csky_addr_lo16;
  case CSKY::S_GOT:
    return CSKY::fixup_csky_got32;
  case CSKY::S_GOTPC:
    return CSKY::fixup_csky_gotpc;
  case CSKY::S_GOTOFF:
    return CSKY::fixup_csky_gotoff;
  case CSKY::S_PLT:
    return CSKY::fixup_csky_plt32;
  case CSKY::S_PLT_IMM18_BY4:
    return CSKY::fixup_csky_plt_imm18_scale4;
  case CSKY::S_GOT_IMM18_BY4:
    return CSKY::fixup_csky_got_imm18_scale4;
  }
}

MCCodeEmitter *llvm::createCSKYMCCodeEmitter(const MCInstrInfo &MCII,
                                             MCContext &Ctx) {
  return new CSKYMCCodeEmitter(Ctx, MCII);
}

#include "CSKYGenMCCodeEmitter.inc"
