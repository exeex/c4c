// Translated from /workspaces/c4c/src/backend/riscv/assembler/encoder/base.rs
// This translation is kept self-contained so it can be validated in isolation.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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
  struct Label {
    std::string name;
  };
  struct Mem {
    std::string base;
    std::int64_t offset;
  };
  struct MemSymbol {
    std::string base;
    std::string symbol;
    std::string modifier;
  };

  using Value = std::variant<Reg, Imm, Symbol, Label, Mem, MemSymbol>;
  Value value;

  Operand(Reg v) : value(std::move(v)) {}
  Operand(Imm v) : value(std::move(v)) {}
  Operand(Symbol v) : value(std::move(v)) {}
  Operand(Label v) : value(std::move(v)) {}
  Operand(Mem v) : value(std::move(v)) {}
  Operand(MemSymbol v) : value(std::move(v)) {}
};

enum class RelocType {
  CallPlt,
  PcrelHi20,
  PcrelLo12I,
  PcrelLo12S,
  Hi20,
  Lo12I,
  Lo12S,
  Branch,
  Jal,
  Abs64,
  Abs32,
  GotHi20,
  TlsGdHi20,
  TlsGotHi20,
  TprelHi20,
  TprelLo12I,
  TprelLo12S,
  TprelAdd,
  Add16,
  Sub16,
  Add32,
  Sub32,
  Add64,
  Sub64,
};

struct Relocation {
  RelocType reloc_type;
  std::string symbol;
  std::int64_t addend;
};

struct EncodeWord {
  std::uint32_t value;
};
struct EncodeHalf {
  std::uint16_t value;
};
struct EncodeWords {
  std::vector<std::uint32_t> values;
};
struct EncodeWordWithReloc {
  std::uint32_t word;
  Relocation reloc;
};
struct EncodeWordsWithRelocs {
  std::vector<std::pair<std::uint32_t, std::optional<Relocation>>> values;
};
struct EncodeSkip {};

using EncodeResult = std::variant<EncodeWord, EncodeHalf, EncodeWords, EncodeWordWithReloc, EncodeWordsWithRelocs, EncodeSkip>;

static constexpr std::uint32_t OP_LUI = 0b0110111;
static constexpr std::uint32_t OP_AUIPC = 0b0010111;
static constexpr std::uint32_t OP_JAL = 0b1101111;
static constexpr std::uint32_t OP_JALR = 0b1100111;
static constexpr std::uint32_t OP_BRANCH = 0b1100011;
static constexpr std::uint32_t OP_LOAD = 0b0000011;
static constexpr std::uint32_t OP_STORE = 0b0100011;
static constexpr std::uint32_t OP_OP_IMM = 0b0010011;
static constexpr std::uint32_t OP_OP = 0b0110011;
static constexpr std::uint32_t OP_OP_IMM_32 = 0b0011011;
static constexpr std::uint32_t OP_OP_32 = 0b0111011;
static constexpr std::uint32_t OP_AMO = 0b0101111;

static std::string operand_kind(const Operand& operand) {
  return std::visit(
      [](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Operand::Reg>) return "register";
        if constexpr (std::is_same_v<T, Operand::Imm>) return "immediate";
        if constexpr (std::is_same_v<T, Operand::Symbol>) return "symbol";
        if constexpr (std::is_same_v<T, Operand::Label>) return "label";
        if constexpr (std::is_same_v<T, Operand::Mem>) return "memory";
        if constexpr (std::is_same_v<T, Operand::MemSymbol>) return "symbolic memory";
        return "operand";
      },
      operand.value);
}

static std::string operand_text(const Operand& operand) {
  return std::visit(
      [](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Operand::Reg>) return value.name;
        if constexpr (std::is_same_v<T, Operand::Imm>) return std::to_string(value.value);
        if constexpr (std::is_same_v<T, Operand::Symbol>) return value.name;
        if constexpr (std::is_same_v<T, Operand::Label>) return value.name;
        if constexpr (std::is_same_v<T, Operand::Mem>) return value.base;
        if constexpr (std::is_same_v<T, Operand::MemSymbol>) return value.symbol;
        return std::string();
      },
      operand.value);
}

static std::string operand_debug(const Operand* operand) {
  if (operand == nullptr) return "none";
  return operand_kind(*operand) + "(" + operand_text(*operand) + ")";
}

static std::uint32_t reg_num(const std::string& name) {
  const std::string lower = [&] {
    std::string out = name;
    for (char& ch : out) {
      if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return out;
  }();

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

static std::optional<std::uint32_t> try_reg_num(const std::string& name) {
  const std::uint32_t value = reg_num(name);
  if (value <= 31) {
    if (name == "zero" || name == "ra" || name == "sp" || name == "gp" || name == "tp" || !name.empty() ||
        value != 0) {
      if (name == "zero" || name == "ra" || name == "sp" || name == "gp" || name == "tp" || name[0] == 'x' ||
          name[0] == 'X' || value != 0 || name == "x0") {
        return value;
      }
    }
  }
  return std::nullopt;
}

static std::uint32_t encode_r(std::uint32_t opcode, std::uint32_t rd, std::uint32_t funct3, std::uint32_t rs1,
                              std::uint32_t rs2, std::uint32_t funct7) {
  return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static std::uint32_t encode_i(std::uint32_t opcode, std::uint32_t rd, std::uint32_t funct3, std::uint32_t rs1,
                              std::int32_t imm) {
  const std::uint32_t encoded = static_cast<std::uint32_t>(imm) & 0xFFFu;
  return (encoded << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

static std::uint32_t encode_s(std::uint32_t opcode, std::uint32_t funct3, std::uint32_t rs1, std::uint32_t rs2,
                              std::int32_t imm) {
  const std::uint32_t encoded = static_cast<std::uint32_t>(imm);
  const std::uint32_t imm_11_5 = (encoded >> 5) & 0x7Fu;
  const std::uint32_t imm_4_0 = encoded & 0x1Fu;
  return (imm_11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_4_0 << 7) | opcode;
}

static std::uint32_t encode_b(std::uint32_t opcode, std::uint32_t funct3, std::uint32_t rs1, std::uint32_t rs2,
                              std::int32_t imm) {
  const std::uint32_t encoded = static_cast<std::uint32_t>(imm);
  const std::uint32_t bit12 = (encoded >> 12) & 1u;
  const std::uint32_t bit11 = (encoded >> 11) & 1u;
  const std::uint32_t bits10_5 = (encoded >> 5) & 0x3Fu;
  const std::uint32_t bits4_1 = (encoded >> 1) & 0xFu;
  return (bit12 << 31) | (bits10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (bits4_1 << 8) |
         (bit11 << 7) | opcode;
}

static std::uint32_t encode_u(std::uint32_t opcode, std::uint32_t rd, std::uint32_t imm) {
  return (imm & 0xFFFFF000u) | (rd << 7) | opcode;
}

static std::uint32_t encode_j(std::uint32_t opcode, std::uint32_t rd, std::int32_t imm) {
  const std::uint32_t encoded = static_cast<std::uint32_t>(imm);
  const std::uint32_t bit20 = (encoded >> 20) & 1u;
  const std::uint32_t bits10_1 = (encoded >> 1) & 0x3FFu;
  const std::uint32_t bit11 = (encoded >> 11) & 1u;
  const std::uint32_t bits19_12 = (encoded >> 12) & 0xFFu;
  return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) | (rd << 7) | opcode;
}

static const Operand* get_operand(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) return nullptr;
  return &operands[idx];
}

static std::optional<std::uint32_t> operand_reg_num(const Operand& operand) {
  if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
    return try_reg_num(reg->name);
  }
  if (const auto* imm = std::get_if<Operand::Imm>(&operand.value)) {
    if (imm->value >= 0 && imm->value <= 31) {
      return static_cast<std::uint32_t>(imm->value);
    }
  }
  return std::nullopt;
}

static std::uint32_t get_reg(const std::vector<Operand>& operands, std::size_t idx) {
  const Operand* operand = get_operand(operands, idx);
  if (operand == nullptr) {
    throw std::string("expected register at operand ") + std::to_string(idx) + ", got none";
  }
  if (const auto* reg = std::get_if<Operand::Reg>(&operand->value)) {
    const auto value = try_reg_num(reg->name);
    if (value.has_value()) return *value;
    throw std::string("invalid integer register: ") + reg->name;
  }
  if (const auto* imm = std::get_if<Operand::Imm>(&operand->value)) {
    if (imm->value >= 0 && imm->value <= 31) return static_cast<std::uint32_t>(imm->value);
  }
  throw std::string("expected register at operand ") + std::to_string(idx) + ", got " + operand_kind(*operand);
}

static std::int64_t get_imm(const std::vector<Operand>& operands, std::size_t idx) {
  const Operand* operand = get_operand(operands, idx);
  if (operand == nullptr) {
    throw std::string("expected immediate at operand ") + std::to_string(idx) + ", got none";
  }
  if (const auto* imm = std::get_if<Operand::Imm>(&operand->value)) {
    return imm->value;
  }
  throw std::string("expected immediate at operand ") + std::to_string(idx) + ", got " + operand_kind(*operand);
}

static std::pair<std::uint32_t, std::int64_t> get_mem(const std::vector<Operand>& operands, std::size_t idx) {
  const Operand* operand = get_operand(operands, idx);
  if (operand == nullptr) {
    throw std::string("expected memory operand at operand ") + std::to_string(idx) + ", got none";
  }
  if (const auto* mem = std::get_if<Operand::Mem>(&operand->value)) {
    const std::uint32_t base_reg = reg_num(mem->base);
    return {base_reg, mem->offset};
  }
  throw std::string("expected memory operand at operand ") + std::to_string(idx) + ", got " + operand_kind(*operand);
}

static std::string extract_modifier_symbol(const std::string& s) {
  const auto start = s.find('(');
  const auto end = s.rfind(')');
  if (start != std::string::npos && end != std::string::npos && end > start + 1) {
    return s.substr(start + 1, end - start - 1);
  }
  return s;
}

static std::pair<RelocType, std::string> parse_reloc_modifier(const std::string& s) {
  if (s.rfind("%pcrel_hi(", 0) == 0) return {RelocType::PcrelHi20, extract_modifier_symbol(s)};
  if (s.rfind("%pcrel_lo(", 0) == 0) return {RelocType::PcrelLo12I, extract_modifier_symbol(s)};
  if (s.rfind("%hi(", 0) == 0) return {RelocType::Hi20, extract_modifier_symbol(s)};
  if (s.rfind("%lo(", 0) == 0) return {RelocType::Lo12I, extract_modifier_symbol(s)};
  if (s.rfind("%tprel_hi(", 0) == 0) return {RelocType::TprelHi20, extract_modifier_symbol(s)};
  if (s.rfind("%tprel_lo(", 0) == 0) return {RelocType::TprelLo12I, extract_modifier_symbol(s)};
  if (s.rfind("%tprel_add(", 0) == 0) return {RelocType::TprelAdd, extract_modifier_symbol(s)};
  if (s.rfind("%got_pcrel_hi(", 0) == 0) return {RelocType::GotHi20, extract_modifier_symbol(s)};
  if (s.rfind("%tls_ie_pcrel_hi(", 0) == 0) return {RelocType::TlsGotHi20, extract_modifier_symbol(s)};
  if (s.rfind("%tls_gd_pcrel_hi(", 0) == 0) return {RelocType::TlsGdHi20, extract_modifier_symbol(s)};
  return {RelocType::PcrelHi20, s};
}

static EncodeResult make_word(std::uint32_t word) {
  return EncodeResult{EncodeWord{word}};
}

static EncodeResult make_word_with_reloc(std::uint32_t word, RelocType reloc_type, std::string symbol,
                                         std::int64_t addend = 0) {
  return EncodeResult{EncodeWordWithReloc{word, Relocation{reloc_type, std::move(symbol), addend}}};
}

static bool is_in_range(std::int64_t value, std::int64_t min, std::int64_t max) {
  return value >= min && value <= max;
}

EncodeResult encode_lui(const std::vector<Operand>& operands) {
  const std::uint32_t rd = get_reg(operands, 0);
  const Operand* op = get_operand(operands, 1);
  if (const auto* imm = op != nullptr ? std::get_if<Operand::Imm>(&op->value) : nullptr) {
    return make_word(encode_u(OP_LUI, rd, static_cast<std::uint32_t>(imm->value) << 12));
  }
  if (const auto* sym = op != nullptr ? std::get_if<Operand::Symbol>(&op->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_u(OP_LUI, rd, 0),
        Relocation{sym->name.rfind("%tprel_hi(", 0) == 0 ? RelocType::TprelHi20 : RelocType::Hi20,
                   extract_modifier_symbol(sym->name), 0},
    }};
  }
  throw std::string("lui: invalid operands");
}

EncodeResult encode_auipc(const std::vector<Operand>& operands) {
  const std::uint32_t rd = get_reg(operands, 0);
  const Operand* op = get_operand(operands, 1);
  if (const auto* imm = op != nullptr ? std::get_if<Operand::Imm>(&op->value) : nullptr) {
    return make_word(encode_u(OP_AUIPC, rd, static_cast<std::uint32_t>(imm->value) << 12));
  }
  if (const auto* sym = op != nullptr ? std::get_if<Operand::Symbol>(&op->value) : nullptr) {
    const auto [reloc_type, symbol] = parse_reloc_modifier(sym->name);
    return make_word_with_reloc(encode_u(OP_AUIPC, rd, 0), reloc_type, symbol, 0);
  }
  throw std::string("auipc: invalid operands");
}

EncodeResult encode_jal(const std::vector<Operand>& operands) {
  if (operands.size() == 1) {
    const Operand& operand = operands[0];
    if (const auto* imm = std::get_if<Operand::Imm>(&operand.value)) {
      return make_word(encode_j(OP_JAL, 1, static_cast<std::int32_t>(imm->value)));
    }
    if (const auto* sym = std::get_if<Operand::Symbol>(&operand.value)) {
      return EncodeResult{EncodeWordWithReloc{
          encode_j(OP_JAL, 1, 0),
          Relocation{RelocType::Jal, sym->name, 0},
      }};
    }
    if (const auto* lab = std::get_if<Operand::Label>(&operand.value)) {
      return EncodeResult{EncodeWordWithReloc{
          encode_j(OP_JAL, 1, 0),
          Relocation{RelocType::Jal, lab->name, 0},
      }};
    }
    if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
      return EncodeResult{EncodeWordWithReloc{
          encode_j(OP_JAL, 1, 0),
          Relocation{RelocType::Jal, reg->name, 0},
      }};
    }
    throw std::string("jal: invalid operand");
  }

  const std::uint32_t rd = get_reg(operands, 0);
  const Operand* operand = get_operand(operands, 1);
  if (const auto* imm = operand != nullptr ? std::get_if<Operand::Imm>(&operand->value) : nullptr) {
    return make_word(encode_j(OP_JAL, rd, static_cast<std::int32_t>(imm->value)));
  }
  if (const auto* sym = operand != nullptr ? std::get_if<Operand::Symbol>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_j(OP_JAL, rd, 0),
        Relocation{RelocType::Jal, sym->name, 0},
    }};
  }
  if (const auto* lab = operand != nullptr ? std::get_if<Operand::Label>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_j(OP_JAL, rd, 0),
        Relocation{RelocType::Jal, lab->name, 0},
    }};
  }
  if (const auto* reg = operand != nullptr ? std::get_if<Operand::Reg>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_j(OP_JAL, rd, 0),
        Relocation{RelocType::Jal, reg->name, 0},
    }};
  }
  throw std::string("jal: invalid operand");
}

EncodeResult encode_jalr(const std::vector<Operand>& operands) {
  switch (operands.size()) {
    case 1: {
      const std::uint32_t rs1 = get_reg(operands, 0);
      return make_word(encode_i(OP_JALR, 1, 0, rs1, 0));
    }
    case 2: {
      const std::uint32_t rd = get_reg(operands, 0);
      const Operand& operand = operands[1];
      if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
        const std::uint32_t rs1 = reg_num(reg->name);
        return make_word(encode_i(OP_JALR, rd, 0, rs1, 0));
      }
      if (const auto* mem = std::get_if<Operand::Mem>(&operand.value)) {
        const std::uint32_t rs1 = reg_num(mem->base);
        return make_word(encode_i(OP_JALR, rd, 0, rs1, static_cast<std::int32_t>(mem->offset)));
      }
      throw std::string("jalr: invalid operands");
    }
    case 3: {
      const std::uint32_t rd = get_reg(operands, 0);
      const std::uint32_t rs1 = get_reg(operands, 1);
      const std::int64_t imm = get_imm(operands, 2);
      return make_word(encode_i(OP_JALR, rd, 0, rs1, static_cast<std::int32_t>(imm)));
    }
    default:
      throw std::string("jalr: wrong number of operands");
  }
}

EncodeResult encode_branch_instr(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rs1 = get_reg(operands, 0);
  const std::uint32_t rs2 = get_reg(operands, 1);
  const Operand* operand = get_operand(operands, 2);
  if (const auto* imm = operand != nullptr ? std::get_if<Operand::Imm>(&operand->value) : nullptr) {
    return make_word(encode_b(OP_BRANCH, funct3, rs1, rs2, static_cast<std::int32_t>(imm->value)));
  }
  if (const auto* sym = operand != nullptr ? std::get_if<Operand::Symbol>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_b(OP_BRANCH, funct3, rs1, rs2, 0),
        Relocation{RelocType::Branch, sym->name, 0},
    }};
  }
  if (const auto* lab = operand != nullptr ? std::get_if<Operand::Label>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_b(OP_BRANCH, funct3, rs1, rs2, 0),
        Relocation{RelocType::Branch, lab->name, 0},
    }};
  }
  if (const auto* reg = operand != nullptr ? std::get_if<Operand::Reg>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordWithReloc{
        encode_b(OP_BRANCH, funct3, rs1, rs2, 0),
        Relocation{RelocType::Branch, reg->name, 0},
    }};
  }
  throw std::string("branch: expected offset or label as 3rd operand");
}

EncodeResult encode_load(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rd = get_reg(operands, 0);
  const Operand* operand = get_operand(operands, 1);
  if (const auto* mem = operand != nullptr ? std::get_if<Operand::Mem>(&operand->value) : nullptr) {
    const std::uint32_t rs1 = reg_num(mem->base);
    return make_word(encode_i(OP_LOAD, rd, funct3, rs1, static_cast<std::int32_t>(mem->offset)));
  }
  if (const auto* memsym = operand != nullptr ? std::get_if<Operand::MemSymbol>(&operand->value) : nullptr) {
    const std::uint32_t rs1 = reg_num(memsym->base);
    const auto [reloc_type0, sym] = parse_reloc_modifier(memsym->symbol);
    const RelocType reloc_type = reloc_type0 == RelocType::PcrelHi20 ? RelocType::PcrelLo12I
                                  : reloc_type0 == RelocType::Hi20   ? RelocType::Lo12I
                                  : reloc_type0 == RelocType::TprelHi20 ? RelocType::TprelLo12I
                                                                         : reloc_type0;
    return EncodeResult{EncodeWordWithReloc{
        encode_i(OP_LOAD, rd, funct3, rs1, 0),
        Relocation{reloc_type, sym, 0},
    }};
  }
  if (const auto* sym = operand != nullptr ? std::get_if<Operand::Symbol>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordsWithRelocs{{
        {encode_u(OP_AUIPC, rd, 0), Relocation{RelocType::PcrelHi20, sym->name, 0}},
        {encode_i(OP_LOAD, rd, funct3, rd, 0), Relocation{RelocType::PcrelLo12I, sym->name, 0}},
    }}};
  }
  if (const auto* lab = operand != nullptr ? std::get_if<Operand::Label>(&operand->value) : nullptr) {
    return EncodeResult{EncodeWordsWithRelocs{{
        {encode_u(OP_AUIPC, rd, 0), Relocation{RelocType::PcrelHi20, lab->name, 0}},
        {encode_i(OP_LOAD, rd, funct3, rd, 0), Relocation{RelocType::PcrelLo12I, lab->name, 0}},
    }}};
  }
  throw std::string("load: expected memory operand");
}

EncodeResult encode_store(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rs2 = get_reg(operands, 0);
  const Operand* operand = get_operand(operands, 1);
  if (const auto* mem = operand != nullptr ? std::get_if<Operand::Mem>(&operand->value) : nullptr) {
    const std::uint32_t rs1 = reg_num(mem->base);
    return make_word(encode_s(OP_STORE, funct3, rs1, rs2, static_cast<std::int32_t>(mem->offset)));
  }
  if (const auto* memsym = operand != nullptr ? std::get_if<Operand::MemSymbol>(&operand->value) : nullptr) {
    const std::uint32_t rs1 = reg_num(memsym->base);
    const auto [reloc_type0, sym] = parse_reloc_modifier(memsym->symbol);
    const RelocType reloc_type = reloc_type0 == RelocType::PcrelHi20 ? RelocType::PcrelLo12S
                                  : reloc_type0 == RelocType::Hi20   ? RelocType::Lo12S
                                  : reloc_type0 == RelocType::TprelHi20 ? RelocType::TprelLo12S
                                                                         : reloc_type0;
    return EncodeResult{EncodeWordWithReloc{
        encode_s(OP_STORE, funct3, rs1, rs2, 0),
        Relocation{reloc_type, sym, 0},
    }};
  }
  throw std::string("store: expected memory operand");
}

EncodeResult encode_alu_imm(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const Operand* operand = get_operand(operands, 2);
  if (const auto* imm = operand != nullptr ? std::get_if<Operand::Imm>(&operand->value) : nullptr) {
    return make_word(encode_i(OP_OP_IMM, rd, funct3, rs1, static_cast<std::int32_t>(imm->value)));
  }
  if (const auto* sym = operand != nullptr ? std::get_if<Operand::Symbol>(&operand->value) : nullptr) {
    const auto [reloc_type0, sym_name] = parse_reloc_modifier(sym->name);
    const RelocType reloc_type = reloc_type0 == RelocType::PcrelHi20 ? RelocType::PcrelLo12I
                                  : reloc_type0 == RelocType::Hi20   ? RelocType::Lo12I
                                  : reloc_type0 == RelocType::TprelHi20 ? RelocType::TprelLo12I
                                                                         : reloc_type0;
    return EncodeResult{EncodeWordWithReloc{
        encode_i(OP_OP_IMM, rd, funct3, rs1, 0),
        Relocation{reloc_type, sym_name, 0},
    }};
  }
  throw std::string("alu_imm: expected immediate");
}

EncodeResult encode_shift_imm(const std::vector<Operand>& operands, std::uint32_t funct3, std::uint32_t funct6) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const std::uint32_t shamt = static_cast<std::uint32_t>(get_imm(operands, 2));
  const std::uint32_t imm = (funct6 << 6) | (shamt & 0x3Fu);
  return make_word(encode_i(OP_OP_IMM, rd, funct3, rs1, static_cast<std::int32_t>(imm)));
}

EncodeResult encode_alu_reg(const std::vector<Operand>& operands, std::uint32_t funct3, std::uint32_t funct7) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const std::uint32_t rs2 = get_reg(operands, 2);
  return make_word(encode_r(OP_OP, rd, funct3, rs1, rs2, funct7));
}

EncodeResult encode_alu_imm_w(const std::vector<Operand>& operands, std::uint32_t funct3) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const std::int32_t imm = static_cast<std::int32_t>(get_imm(operands, 2));
  return make_word(encode_i(OP_OP_IMM_32, rd, funct3, rs1, imm));
}

EncodeResult encode_shift_imm_w(const std::vector<Operand>& operands, std::uint32_t funct3, std::uint32_t funct7) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const std::uint32_t shamt = static_cast<std::uint32_t>(get_imm(operands, 2));
  const std::uint32_t imm = (funct7 << 5) | (shamt & 0x1Fu);
  return make_word(encode_i(OP_OP_IMM_32, rd, funct3, rs1, static_cast<std::int32_t>(imm)));
}

EncodeResult encode_alu_reg_w(const std::vector<Operand>& operands, std::uint32_t funct3, std::uint32_t funct7) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  const std::uint32_t rs2 = get_reg(operands, 2);
  return make_word(encode_r(OP_OP_32, rd, funct3, rs1, rs2, funct7));
}

EncodeResult encode_zbb_unary(const std::vector<Operand>& operands, std::uint32_t imm12) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  return make_word(encode_i(OP_OP_IMM, rd, 0b001, rs1, static_cast<std::int32_t>(imm12)));
}

EncodeResult encode_zbb_unary_f5(const std::vector<Operand>& operands, std::uint32_t imm12) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  return make_word(encode_i(OP_OP_IMM, rd, 0b101, rs1, static_cast<std::int32_t>(imm12)));
}

EncodeResult encode_zbb_unary_w(const std::vector<Operand>& operands, std::uint32_t imm12) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  return make_word(encode_i(OP_OP_IMM_32, rd, 0b001, rs1, static_cast<std::int32_t>(imm12)));
}

EncodeResult encode_zbb_zexth(const std::vector<Operand>& operands) {
  const std::uint32_t rd = get_reg(operands, 0);
  const std::uint32_t rs1 = get_reg(operands, 1);
  return make_word(encode_r(OP_OP_32, rd, 0b100, rs1, 0, 0b0000100));
}

}  // namespace c4c::backend::riscv::assembler::encoder
