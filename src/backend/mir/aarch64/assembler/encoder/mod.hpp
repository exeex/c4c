#pragma once

#include "../types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler::encoder {

// Staged encoder helpers for parsed external assembler operands and relocations.
// They are not the internal object-writer contract for terminal --codegen asm
// text. Future backend-owned compile-through work should consume structured
// asm/encoding records derived from machine instruction nodes.
enum class RelocType {
  Call26,
  Jump26,
  AdrpPage21,
  AddAbsLo12,
  Ldst8AbsLo12,
  Ldst16AbsLo12,
  Ldst32AbsLo12,
  Ldst64AbsLo12,
  Ldst128AbsLo12,
  AdrGotPage21,
  Ld64GotLo12,
  TlsLeAddTprelHi12,
  TlsLeAddTprelLo12,
  CondBr19,
  TstBr14,
  AdrPrelLo21,
  Abs64,
  Abs32,
  Prel32,
  Prel64,
  Ldr19,
};

struct Relocation {
  RelocType reloc_type = RelocType::Abs32;
  std::string symbol;
  std::int64_t addend = 0;
};

enum class EncodeResultKind {
  Word,
  WordWithReloc,
  Words,
  Skip,
};

struct EncodeResult {
  bool encoded = false;
  EncodeResultKind kind = EncodeResultKind::Skip;
  std::uint32_t word = 0;
  std::vector<std::uint32_t> words;
  Relocation reloc;
};

std::uint32_t parse_reg_num(const std::string& name);
bool is_64bit_reg(const std::string& name);
bool is_32bit_reg(const std::string& name);
bool is_fp_reg(const std::string& name);
std::uint32_t encode_cond(const std::string& cond);

// Encodes one parsed external-assembler instruction. Do not use this as a
// bridge from machine_printer.cpp output back into backend semantics.
EncodeResult encode_instruction(const std::string& mnemonic,
                                const std::vector<Operand>& operands,
                                const std::string& raw_operands);
std::pair<std::uint32_t, bool> get_reg(const std::vector<Operand>& operands, std::size_t idx);
std::int64_t get_imm(const std::vector<Operand>& operands, std::size_t idx);
std::pair<std::string, std::int64_t> get_symbol(const std::vector<Operand>& operands,
                                                std::size_t idx);
std::uint32_t sf_bit(bool is_64);

}  // namespace c4c::backend::aarch64::assembler::encoder
