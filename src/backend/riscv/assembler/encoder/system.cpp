// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/system.rs
// Self-contained translation of the system/CSR encoder helpers.

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::assembler::encoder {

struct Operand {
  struct Reg {
    std::string name;
  };
  struct Imm {
    std::int64_t value;
  };
  struct Symbol {
    std::string name;
  };
  struct Csr {
    std::string name;
  };
  struct FenceArg {
    std::string value;
  };

  using Value = std::variant<Reg, Imm, Symbol, Csr, FenceArg>;
  Value value;

  Operand(Reg v) : value(std::move(v)) {}
  Operand(Imm v) : value(std::move(v)) {}
  Operand(Symbol v) : value(std::move(v)) {}
  Operand(Csr v) : value(std::move(v)) {}
  Operand(FenceArg v) : value(std::move(v)) {}
};

struct EncodeResult {
  std::uint32_t word = 0;
};

using EncodeOutcome = std::variant<EncodeResult, std::string>;

static constexpr std::uint32_t OP_MISC_MEM = 0b0001111;
static constexpr std::uint32_t OP_SYSTEM = 0b1110011;

namespace {

std::string lower_copy(std::string_view text) {
  std::string out(text);
  for (char& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

std::uint32_t encode_i(std::uint32_t opcode,
                       std::uint32_t rd,
                       std::uint32_t funct3,
                       std::uint32_t rs1,
                       std::int32_t imm) {
  return (static_cast<std::uint32_t>(imm) & 0xfffU) << 20 | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

std::uint32_t encode_r(std::uint32_t opcode,
                       std::uint32_t rd,
                       std::uint32_t funct3,
                       std::uint32_t rs1,
                       std::uint32_t rs2,
                       std::uint32_t funct7) {
  return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

std::uint32_t reg_num(std::string_view name) {
  const std::string lower = lower_copy(name);

  if (lower == "zero") return 0;
  if (lower == "ra") return 1;
  if (lower == "sp") return 2;
  if (lower == "gp") return 3;
  if (lower == "tp") return 4;
  if (lower == "t0") return 5;
  if (lower == "t1") return 6;
  if (lower == "t2") return 7;
  if (lower == "s0" || lower == "fp") return 8;
  if (lower == "s1") return 9;
  if (lower == "a0") return 10;
  if (lower == "a1") return 11;
  if (lower == "a2") return 12;
  if (lower == "a3") return 13;
  if (lower == "a4") return 14;
  if (lower == "a5") return 15;
  if (lower == "a6") return 16;
  if (lower == "a7") return 17;
  if (lower == "s2") return 18;
  if (lower == "s3") return 19;
  if (lower == "s4") return 20;
  if (lower == "s5") return 21;
  if (lower == "s6") return 22;
  if (lower == "s7") return 23;
  if (lower == "s8") return 24;
  if (lower == "s9") return 25;
  if (lower == "s10") return 26;
  if (lower == "s11") return 27;
  if (lower == "t3") return 28;
  if (lower == "t4") return 29;
  if (lower == "t5") return 30;
  if (lower == "t6") return 31;

  if (!lower.empty() && lower[0] == 'x') {
    std::uint32_t value = 0;
    for (std::size_t i = 1; i < lower.size(); ++i) {
      const char ch = lower[i];
      if (ch < '0' || ch > '9') return 0;
      value = value * 10 + static_cast<std::uint32_t>(ch - '0');
    }
    if (value <= 31) return value;
  }

  return 0;
}

std::uint32_t get_reg(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return 0;
  }
  const auto* reg = std::get_if<Operand::Reg>(&operands[idx].value);
  if (reg == nullptr) {
    return 0;
  }
  return reg_num(reg->name);
}

std::int64_t get_imm(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return 0;
  }
  const auto* imm = std::get_if<Operand::Imm>(&operands[idx].value);
  if (imm == nullptr) {
    return 0;
  }
  return imm->value;
}

std::uint32_t parse_fence_bits(std::string_view text) {
  const std::string lower = lower_copy(text);
  std::uint32_t bits = 0;
  if (lower.find('i') != std::string::npos) bits |= 0b1000;
  if (lower.find('o') != std::string::npos) bits |= 0b0100;
  if (lower.find('r') != std::string::npos) bits |= 0b0010;
  if (lower.find('w') != std::string::npos) bits |= 0b0001;
  return bits;
}

bool parse_u32(std::string_view text, std::uint32_t* out) {
  const std::string lower = lower_copy(text);
  if (lower.empty()) {
    return false;
  }

  std::uint64_t value = 0;
  std::size_t pos = 0;
  if (lower.size() > 2 && lower[0] == '0' && lower[1] == 'x') {
    pos = 2;
    if (pos >= lower.size()) {
      return false;
    }
    for (; pos < lower.size(); ++pos) {
      const char ch = lower[pos];
      value <<= 4;
      if (ch >= '0' && ch <= '9') value |= static_cast<std::uint32_t>(ch - '0');
      else if (ch >= 'a' && ch <= 'f') value |= static_cast<std::uint32_t>(10 + ch - 'a');
      else return false;
    }
    *out = static_cast<std::uint32_t>(value);
    return true;
  }

  for (char ch : lower) {
    if (ch < '0' || ch > '9') {
      return false;
    }
    value = value * 10 + static_cast<std::uint64_t>(ch - '0');
  }
  *out = static_cast<std::uint32_t>(value);
  return true;
}

EncodeOutcome make_error(std::string message) {
  return message;
}

}  // namespace

EncodeOutcome encode_fence(const std::vector<Operand>& operands) {
  std::uint32_t pred = 0xF;
  std::uint32_t succ = 0xF;
  if (operands.size() >= 2) {
    if (const auto* fence = std::get_if<Operand::FenceArg>(&operands[0].value)) {
      pred = parse_fence_bits(fence->value);
    }
    if (const auto* fence = std::get_if<Operand::FenceArg>(&operands[1].value)) {
      succ = parse_fence_bits(fence->value);
    }
  }
  const std::int32_t imm = static_cast<std::int32_t>((pred << 4) | succ);
  return EncodeResult{encode_i(OP_MISC_MEM, 0, 0, 0, imm)};
}

EncodeOutcome encode_sfence_vma(const std::vector<Operand>& operands) {
  const std::uint32_t rs1 = operands.empty() ? 0 : get_reg(operands, 0);
  const std::uint32_t rs2 = operands.size() < 2 ? 0 : get_reg(operands, 1);
  return EncodeResult{encode_r(OP_SYSTEM, 0, 0b000, rs1, rs2, 0b0001001)};
}

std::uint32_t csr_name_to_num(std::string_view name) {
  const std::string lower = lower_copy(name);
  if (lower == "fflags") return 0x001;
  if (lower == "frm") return 0x002;
  if (lower == "fcsr") return 0x003;
  if (lower == "cycle") return 0xC00;
  if (lower == "time") return 0xC01;
  if (lower == "instret") return 0xC02;
  if (lower == "cycleh") return 0xC80;
  if (lower == "timeh") return 0xC81;
  if (lower == "instreth") return 0xC82;
  if (lower == "mstatus") return 0x300;
  if (lower == "misa") return 0x301;
  if (lower == "mie") return 0x304;
  if (lower == "mtvec") return 0x305;
  if (lower == "mscratch") return 0x340;
  if (lower == "mepc") return 0x341;
  if (lower == "mcause") return 0x342;
  if (lower == "mtval") return 0x343;
  if (lower == "mip") return 0x344;
  if (lower == "sstatus") return 0x100;
  if (lower == "sip") return 0x144;
  if (lower == "sie") return 0x104;
  if (lower == "stvec") return 0x105;
  if (lower == "sscratch") return 0x140;
  if (lower == "sepc") return 0x141;
  if (lower == "scause") return 0x142;
  if (lower == "stval") return 0x143;
  if (lower == "satp") return 0x180;

  std::uint32_t parsed = 0;
  if (parse_u32(lower, &parsed)) {
    return parsed;
  }
  return 0;
}

std::uint32_t get_csr_num(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return 0;
  }
  const auto& operand = operands[idx].value;
  if (const auto* imm = std::get_if<Operand::Imm>(&operand)) {
    return static_cast<std::uint32_t>(imm->value);
  }
  if (const auto* csr = std::get_if<Operand::Csr>(&operand)) {
    return csr_name_to_num(csr->name);
  }
  if (const auto* symbol = std::get_if<Operand::Symbol>(&operand)) {
    return csr_name_to_num(symbol->name);
  }
  if (const auto* reg = std::get_if<Operand::Reg>(&operand)) {
    return csr_name_to_num(reg->name);
  }
  return 0;
}

EncodeOutcome encode_csr(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t csr = get_csr_num(operands, 1);
  if (operands.size() > 2 && std::holds_alternative<Operand::Imm>(operands[2].value)) {
    const std::uint32_t zimm = static_cast<std::uint32_t>(get_imm(operands, 2));
    const std::uint32_t rs1 = zimm & 0x1F;
    const std::uint32_t imm_funct3 = funct3 | 0b100;
    return EncodeResult{encode_i(OP_SYSTEM, rd, imm_funct3, rs1, static_cast<std::int32_t>(csr))};
  }
  const std::uint32_t rs1 = get_reg(operands, 2);
  return EncodeResult{encode_i(OP_SYSTEM, rd, funct3, rs1, static_cast<std::int32_t>(csr))};
}

EncodeOutcome encode_csri(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t csr = get_csr_num(operands, 1);
  const std::uint32_t zimm = static_cast<std::uint32_t>(get_imm(operands, 2));
  const std::uint32_t rs1 = zimm & 0x1F;
  return EncodeResult{encode_i(OP_SYSTEM, rd, funct3, rs1, static_cast<std::int32_t>(csr))};
}

}  // namespace c4c::backend::riscv::assembler::encoder
