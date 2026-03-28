#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler::encoder {

struct Operand {
  std::string text;
};

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
  EncodeResultKind kind = EncodeResultKind::Skip;
  std::uint32_t word = 0;
  std::vector<std::uint32_t> words;
  Relocation reloc;
};

std::uint32_t parse_reg_num(const std::string& name) {
  (void)name;
  return 0;
}

bool is_64bit_reg(const std::string& name) {
  return !name.empty() && name.front() == 'x';
}

bool is_32bit_reg(const std::string& name) {
  return !name.empty() && name.front() == 'w';
}

bool is_fp_reg(const std::string& name) {
  return !name.empty() && (name.front() == 'd' || name.front() == 's' || name.front() == 'q' || name.front() == 'v');
}

std::uint32_t encode_cond(const std::string& cond) {
  (void)cond;
  return 0;
}

EncodeResult encode_instruction(const std::string& mnemonic,
                                const std::vector<Operand>& operands,
                                const std::string& raw_operands) {
  (void)mnemonic;
  (void)operands;
  (void)raw_operands;
  return {};
}

std::pair<std::uint32_t, bool> get_reg(const std::vector<Operand>& operands, std::size_t idx) {
  (void)operands;
  (void)idx;
  return {0, false};
}

std::int64_t get_imm(const std::vector<Operand>& operands, std::size_t idx) {
  (void)operands;
  (void)idx;
  return 0;
}

std::pair<std::string, std::int64_t> get_symbol(const std::vector<Operand>& operands, std::size_t idx) {
  (void)operands;
  (void)idx;
  return {"", 0};
}

std::uint32_t sf_bit(bool is_64) {
  return is_64 ? 1u : 0u;
}

}  // namespace c4c::backend::aarch64::assembler::encoder
