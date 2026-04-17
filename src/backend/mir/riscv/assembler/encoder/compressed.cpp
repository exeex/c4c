// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/compressed.rs

#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::assembler::encoder {

struct Operand {
  enum class Kind {
    Reg,
    Imm,
    Symbol,
    Label,
    SymbolOffset,
    Mem,
    MemSymbol,
  };

  Kind kind = Kind::Imm;
  std::string text;
  std::int64_t imm = 0;
  std::string base;
  std::string symbol;
  std::string modifier;

  static Operand reg(std::string value) {
    Operand op;
    op.kind = Kind::Reg;
    op.text = std::move(value);
    return op;
  }

  static Operand imm_value(std::int64_t value) {
    Operand op;
    op.kind = Kind::Imm;
    op.imm = value;
    return op;
  }

  static Operand symbol_value(std::string value) {
    Operand op;
    op.kind = Kind::Symbol;
    op.text = std::move(value);
    return op;
  }

  static Operand label(std::string value) {
    Operand op;
    op.kind = Kind::Label;
    op.text = std::move(value);
    return op;
  }

  static Operand symbol_offset(std::string value, std::int64_t addend) {
    Operand op;
    op.kind = Kind::SymbolOffset;
    op.symbol = std::move(value);
    op.imm = addend;
    return op;
  }

  static Operand mem(std::string base_reg, std::int64_t offset) {
    Operand op;
    op.kind = Kind::Mem;
    op.base = std::move(base_reg);
    op.imm = offset;
    return op;
  }

  static Operand mem_symbol(std::string base_reg, std::string sym, std::string mod) {
    Operand op;
    op.kind = Kind::MemSymbol;
    op.base = std::move(base_reg);
    op.symbol = std::move(sym);
    op.modifier = std::move(mod);
    return op;
  }
};

enum class EncodeResultKind {
  Word,
  Half,
  Words,
  WordWithReloc,
  WordsWithRelocs,
  Skip,
};

struct EncodeResult {
  EncodeResultKind kind = EncodeResultKind::Skip;
  std::uint32_t word = 0;
  std::uint16_t half = 0;
  std::vector<std::uint32_t> words;

  static EncodeResult Word(std::uint32_t value) {
    EncodeResult result;
    result.kind = EncodeResultKind::Word;
    result.word = value;
    return result;
  }

  static EncodeResult Half(std::uint16_t value) {
    EncodeResult result;
    result.kind = EncodeResultKind::Half;
    result.half = value;
    return result;
  }

  static EncodeResult Words(std::vector<std::uint32_t> values) {
    EncodeResult result;
    result.kind = EncodeResultKind::Words;
    result.words = std::move(values);
    return result;
  }
};

using EncodeOutcome = std::variant<EncodeResult, std::string>;

namespace {

constexpr std::uint32_t OP_LUI = 0b0110111;
constexpr std::uint32_t OP_OP_IMM = 0b0010011;
constexpr std::uint32_t OP_OP_IMM_32 = 0b0011011;
constexpr std::uint32_t OP_OP = 0b0110011;
constexpr std::uint32_t OP_AMO = 0b0101111;
constexpr std::uint32_t OP_SYSTEM = 0b1110011;

EncodeOutcome ok(EncodeResult result) { return result; }
EncodeOutcome err(std::string message) { return message; }

EncodeOutcome encode_insn_r(const std::vector<std::string>& fields);
EncodeOutcome encode_insn_i(const std::vector<std::string>& fields);
EncodeOutcome encode_insn_s(const std::vector<std::string>& fields);
EncodeOutcome encode_insn_b(const std::vector<std::string>& fields);
EncodeOutcome encode_insn_u(const std::vector<std::string>& fields);
EncodeOutcome encode_insn_j(const std::vector<std::string>& fields);

std::string trim_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::string lower_copy(std::string_view text) {
  std::string out(text);
  for (char& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

std::vector<std::string> split_csv(std::string_view text) {
  std::vector<std::string> out;
  std::string current;
  for (char ch : text) {
    if (ch == ',') {
      auto trimmed = trim_copy(current);
      if (!trimmed.empty()) out.push_back(std::move(trimmed));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  auto trimmed = trim_copy(current);
  if (!trimmed.empty()) out.push_back(std::move(trimmed));
  return out;
}

bool parse_decimal(std::string_view text, std::int64_t* out) {
  std::string value = trim_copy(text);
  if (value.empty()) return false;

  int sign = 1;
  std::size_t pos = 0;
  if (value[pos] == '+') {
    ++pos;
  } else if (value[pos] == '-') {
    sign = -1;
    ++pos;
  }

  if (pos >= value.size()) return false;

  std::int64_t result = 0;
  for (; pos < value.size(); ++pos) {
    const unsigned char ch = static_cast<unsigned char>(value[pos]);
    if (ch < '0' || ch > '9') return false;
    result = result * 10 + static_cast<std::int64_t>(ch - '0');
  }
  *out = result * sign;
  return true;
}

bool parse_hex(std::string_view text, std::int64_t* out) {
  std::string value = trim_copy(text);
  if (value.empty()) return false;

  int sign = 1;
  std::size_t pos = 0;
  if (value[pos] == '+') {
    ++pos;
  } else if (value[pos] == '-') {
    sign = -1;
    ++pos;
  }

  if (pos + 2 > value.size() || value[pos] != '0' ||
      (value[pos + 1] != 'x' && value[pos + 1] != 'X')) {
    return false;
  }
  pos += 2;
  if (pos >= value.size()) return false;

  std::uint64_t result = 0;
  for (; pos < value.size(); ++pos) {
    const unsigned char ch = static_cast<unsigned char>(value[pos]);
    unsigned digit;
    if (ch >= '0' && ch <= '9') {
      digit = ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
      digit = 10 + (ch - 'a');
    } else if (ch >= 'A' && ch <= 'F') {
      digit = 10 + (ch - 'A');
    } else {
      return false;
    }
    result = (result << 4) | digit;
  }

  *out = static_cast<std::int64_t>(result) * sign;
  return true;
}

bool parse_binary(std::string_view text, std::int64_t* out) {
  std::string value = trim_copy(text);
  if (value.empty()) return false;

  int sign = 1;
  std::size_t pos = 0;
  if (value[pos] == '+') {
    ++pos;
  } else if (value[pos] == '-') {
    sign = -1;
    ++pos;
  }

  if (pos + 2 > value.size() || value[pos] != '0' ||
      (value[pos + 1] != 'b' && value[pos + 1] != 'B')) {
    return false;
  }
  pos += 2;
  if (pos >= value.size()) return false;

  std::uint64_t result = 0;
  for (; pos < value.size(); ++pos) {
    const unsigned char ch = static_cast<unsigned char>(value[pos]);
    if (ch != '0' && ch != '1') return false;
    result = (result << 1) | static_cast<std::uint64_t>(ch - '0');
  }

  *out = static_cast<std::int64_t>(result) * sign;
  return true;
}

bool parse_insn_int_impl(std::string_view text, std::int64_t* out) {
  const std::string value = trim_copy(text);
  if (value.empty()) return false;

  std::size_t pos = 0;
  if (value[pos] == '+' || value[pos] == '-') {
    ++pos;
  }
  if (pos + 2 <= value.size() && value[pos] == '0' &&
      (value[pos + 1] == 'x' || value[pos + 1] == 'X')) {
    return parse_hex(value, out);
  }
  if (pos + 2 <= value.size() && value[pos] == '0' &&
      (value[pos + 1] == 'b' || value[pos + 1] == 'B')) {
    return parse_binary(value, out);
  }
  return parse_decimal(value, out);
}

std::optional<std::uint32_t> reg_num(std::string_view name) {
  const std::string lower = lower_copy(name);
  if (lower.empty()) return std::nullopt;

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

  if (lower.size() > 1 && lower[0] == 'x') {
    std::int64_t value = 0;
    if (!parse_decimal(lower.substr(1), &value)) return std::nullopt;
    if (value < 0 || value > 31) return std::nullopt;
    return static_cast<std::uint32_t>(value);
  }

  return std::nullopt;
}

std::uint32_t encode_r(
    std::uint32_t opcode,
    std::uint32_t rd,
    std::uint32_t funct3,
    std::uint32_t rs1,
    std::uint32_t rs2,
    std::uint32_t funct7) {
  return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
         (rd << 7) | opcode;
}

std::uint32_t encode_i(
    std::uint32_t opcode,
    std::uint32_t rd,
    std::uint32_t funct3,
    std::uint32_t rs1,
    std::int32_t imm) {
  const auto imm_u = static_cast<std::uint32_t>(imm) & 0xFFFu;
  return (imm_u << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

std::uint32_t encode_s(
    std::uint32_t opcode,
    std::uint32_t funct3,
    std::uint32_t rs1,
    std::uint32_t rs2,
    std::int32_t imm) {
  const auto imm_u = static_cast<std::uint32_t>(imm);
  const auto imm_11_5 = (imm_u >> 5) & 0x7Fu;
  const auto imm_4_0 = imm_u & 0x1Fu;
  return (imm_11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
         (imm_4_0 << 7) | opcode;
}

std::uint32_t encode_b(
    std::uint32_t opcode,
    std::uint32_t funct3,
    std::uint32_t rs1,
    std::uint32_t rs2,
    std::int32_t imm) {
  const auto imm_u = static_cast<std::uint32_t>(imm);
  const auto bit12 = (imm_u >> 12) & 1u;
  const auto bit11 = (imm_u >> 11) & 1u;
  const auto bits10_5 = (imm_u >> 5) & 0x3Fu;
  const auto bits4_1 = (imm_u >> 1) & 0xFu;
  return (bit12 << 31) | (bits10_5 << 25) | (rs2 << 20) | (rs1 << 15) |
         (funct3 << 12) | (bits4_1 << 8) | (bit11 << 7) | opcode;
}

std::uint32_t encode_u(std::uint32_t opcode, std::uint32_t rd, std::uint32_t imm) {
  return (imm & 0xFFFFF000u) | (rd << 7) | opcode;
}

std::uint32_t encode_j(std::uint32_t opcode, std::uint32_t rd, std::int32_t imm) {
  const auto imm_u = static_cast<std::uint32_t>(imm);
  const auto bit20 = (imm_u >> 20) & 1u;
  const auto bits10_1 = (imm_u >> 1) & 0x3FFu;
  const auto bit11 = (imm_u >> 11) & 1u;
  const auto bits19_12 = (imm_u >> 12) & 0xFFu;
  return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) |
         (rd << 7) | opcode;
}

std::uint32_t get_reg(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    throw std::runtime_error("expected register operand");
  }
  const Operand& op = operands[idx];
  if (op.kind == Operand::Kind::Reg) {
    const auto reg = reg_num(op.text);
    if (!reg.has_value()) {
      throw std::runtime_error("invalid register: " + op.text);
    }
    return *reg;
  }
  if (op.kind == Operand::Kind::Imm && op.imm >= 0 && op.imm <= 31) {
    return static_cast<std::uint32_t>(op.imm);
  }
  throw std::runtime_error("expected register at operand " + std::to_string(idx));
}

std::uint32_t get_imm(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size() || operands[idx].kind != Operand::Kind::Imm) {
    throw std::runtime_error("expected immediate at operand " + std::to_string(idx));
  }
  return static_cast<std::uint32_t>(operands[idx].imm);
}

std::pair<std::string, std::int64_t> get_symbol(
    const std::vector<Operand>& operands,
    std::size_t idx) {
  if (idx >= operands.size()) {
    throw std::runtime_error("expected symbol operand");
  }
  const Operand& op = operands[idx];
  switch (op.kind) {
    case Operand::Kind::Symbol:
    case Operand::Kind::Label:
      return {op.text, 0};
    case Operand::Kind::SymbolOffset:
      return {op.symbol, op.imm};
    case Operand::Kind::Reg:
      return {op.text, 0};
    default:
      break;
  }
  throw std::runtime_error("expected symbol at operand " + std::to_string(idx));
}

std::pair<std::uint32_t, std::int64_t> get_mem(
    const std::vector<Operand>& operands,
    std::size_t idx) {
  if (idx >= operands.size() || operands[idx].kind != Operand::Kind::Mem) {
    throw std::runtime_error("expected memory operand at operand " + std::to_string(idx));
  }
  const auto reg = reg_num(operands[idx].base);
  if (!reg.has_value()) {
    throw std::runtime_error("invalid base register: " + operands[idx].base);
  }
  return {*reg, operands[idx].imm};
}

std::optional<std::int64_t> parse_insn_int(std::string_view text) {
  std::int64_t value = 0;
  if (!parse_insn_int_impl(text, &value)) {
    return std::nullopt;
  }
  return value;
}

std::int64_t parse_int_or_throw(std::string_view text) {
  std::int64_t value = 0;
  if (!parse_insn_int_impl(text, &value)) {
    throw std::runtime_error("invalid int in .insn: " + trim_copy(text));
  }
  return value;
}

std::uint32_t parse_insn_reg(std::string_view text) {
  const auto reg = reg_num(text);
  if (!reg.has_value()) {
    throw std::runtime_error("invalid register in .insn: " + trim_copy(text));
  }
  return *reg;
}

}  // namespace

EncodeOutcome encode_c_lui(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  if (rd == 0 || rd == 2) {
    return err("c.lui: rd cannot be x0 or x2");
  }
  const auto imm = static_cast<std::int32_t>(get_imm(operands, 1));
  const auto nzimm = imm;
  if (nzimm == 0) {
    return err("c.lui: nzimm must not be zero");
  }
  const auto bit17 = static_cast<std::uint16_t>((nzimm >> 5) & 1);
  const auto bits16_12 = static_cast<std::uint16_t>(nzimm & 0x1F);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b01 | ((bits16_12 & 0x1F) << 2) | (rd << 7) | (bit17 << 12) | (0b011 << 13))));
}

EncodeOutcome encode_c_li(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  const auto imm = static_cast<std::int32_t>(get_imm(operands, 1));
  const auto bit5 = static_cast<std::uint16_t>((imm >> 5) & 1);
  const auto bits4_0 = static_cast<std::uint16_t>(imm & 0x1F);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b01 | (bits4_0 << 2) | (rd << 7) | (bit5 << 12) | (0b010 << 13))));
}

EncodeOutcome encode_c_addi(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  const auto imm = static_cast<std::int32_t>(get_imm(operands, 1));
  const auto bit5 = static_cast<std::uint16_t>((imm >> 5) & 1);
  const auto bits4_0 = static_cast<std::uint16_t>(imm & 0x1F);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b01 | (bits4_0 << 2) | (rd << 7) | (bit5 << 12))));
}

EncodeOutcome encode_c_mv(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  const auto rs2 = get_reg(operands, 1);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b10 | (rs2 << 2) | (rd << 7) | (0b100 << 13))));
}

EncodeOutcome encode_c_add(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  const auto rs2 = get_reg(operands, 1);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b10 | (rs2 << 2) | (rd << 7) | (1u << 12) | (0b100 << 13))));
}

EncodeOutcome encode_c_jr(const std::vector<Operand>& operands) {
  const auto rs1 = get_reg(operands, 0);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b10 | (rs1 << 7) | (0b100 << 13))));
}

EncodeOutcome encode_c_jalr(const std::vector<Operand>& operands) {
  const auto rs1 = get_reg(operands, 0);
  return ok(EncodeResult::Half(static_cast<std::uint16_t>(
      0b10 | (rs1 << 7) | (1u << 12) | (0b100 << 13))));
}

EncodeOutcome encode_insn_directive(std::string_view args) {
  const auto trimmed = trim_copy(args);
  if (trimmed.empty()) {
    return err("empty .insn directive");
  }

  std::size_t split = 0;
  while (split < trimmed.size() &&
         std::isspace(static_cast<unsigned char>(trimmed[split])) == 0 &&
         trimmed[split] != ',') {
    ++split;
  }

  const auto first = trim_copy(trimmed.substr(0, split));
  const auto format = lower_copy(first);
  const auto rest = split < trimmed.size() ? trim_copy(trimmed.substr(split + 1)) : std::string{};
  std::vector<std::string> fields = split_csv(rest);

  if (format == "r") return encode_insn_r(fields);
  if (format == "i") return encode_insn_i(fields);
  if (format == "s") return encode_insn_s(fields);
  if (format == "b" || format == "sb") return encode_insn_b(fields);
  if (format == "u") return encode_insn_u(fields);
  if (format == "j" || format == "uj") return encode_insn_j(fields);

  std::int64_t word = 0;
  if (parse_insn_int_impl(first, &word)) {
    return ok(EncodeResult::Word(static_cast<std::uint32_t>(word)));
  }
  return err("unsupported .insn format: " + format);
}

EncodeOutcome encode_insn_r(const std::vector<std::string>& fields) {
  if (fields.size() < 6) {
    return err(".insn r requires 6 fields (opcode, funct3, funct7, rd, rs1, rs2), got " +
               std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(parse_int_or_throw(fields[0]));
  const auto funct3 = static_cast<std::uint32_t>(parse_int_or_throw(fields[1]));
  const auto funct7 = static_cast<std::uint32_t>(parse_int_or_throw(fields[2]));
  const auto rd = parse_insn_reg(fields[3]);
  const auto rs1 = parse_insn_reg(fields[4]);
  const auto rs2 = parse_insn_reg(fields[5]);
  return ok(EncodeResult::Word(encode_r(opcode, rd, funct3, rs1, rs2, funct7)));
}

EncodeOutcome encode_insn_i(const std::vector<std::string>& fields) {
  if (fields.size() < 5) {
    return err(".insn i requires 5 fields (opcode, funct3, rd, rs1, imm), got " +
               std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(parse_int_or_throw(fields[0]));
  const auto funct3 = static_cast<std::uint32_t>(parse_int_or_throw(fields[1]));
  const auto rd = parse_insn_reg(fields[2]);
  const auto rs1 = parse_insn_reg(fields[3]);
  const auto imm = static_cast<std::int32_t>(parse_int_or_throw(fields[4]));
  return ok(EncodeResult::Word(encode_i(opcode, rd, funct3, rs1, imm)));
}

EncodeOutcome encode_insn_s(const std::vector<std::string>& fields) {
  if (fields.size() < 4) {
    return err(".insn s requires 4 fields, got " + std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(parse_int_or_throw(fields[0]));
  const auto funct3 = static_cast<std::uint32_t>(parse_int_or_throw(fields[1]));
  const auto rs2 = parse_insn_reg(fields[2]);
  const auto last = trim_copy(fields[3]);
  const auto paren_pos = last.find('(');
  if (paren_pos == std::string::npos) {
    return err(".insn s: expected imm(rs1) format for last field");
  }

  const auto imm_str = trim_copy(last.substr(0, paren_pos));
  const auto rs1_str = trim_copy(last.substr(paren_pos + 1, last.size() - paren_pos - 2));
  const auto imm = static_cast<std::int32_t>(std::stoll(imm_str));
  const auto rs1 = parse_insn_reg(rs1_str);
  return ok(EncodeResult::Word(encode_s(opcode, funct3, rs1, rs2, imm)));
}

EncodeOutcome encode_insn_b(const std::vector<std::string>& fields) {
  if (fields.size() < 5) {
    return err(".insn b requires 5 fields, got " + std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(std::stoll(fields[0]));
  const auto funct3 = static_cast<std::uint32_t>(std::stoll(fields[1]));
  const auto rs1 = parse_insn_reg(fields[2]);
  const auto rs2 = parse_insn_reg(fields[3]);
  const auto imm = static_cast<std::int32_t>(parse_int_or_throw(fields[4]));
  return ok(EncodeResult::Word(encode_b(opcode, funct3, rs1, rs2, imm)));
}

EncodeOutcome encode_insn_u(const std::vector<std::string>& fields) {
  if (fields.size() < 3) {
    return err(".insn u requires 3 fields, got " + std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(parse_int_or_throw(fields[0]));
  const auto rd = parse_insn_reg(fields[1]);
  const auto imm = static_cast<std::uint32_t>(parse_int_or_throw(fields[2]));
  return ok(EncodeResult::Word(encode_u(opcode, rd, imm)));
}

EncodeOutcome encode_insn_j(const std::vector<std::string>& fields) {
  if (fields.size() < 3) {
    return err(".insn j requires 3 fields, got " + std::to_string(fields.size()));
  }
  const auto opcode = static_cast<std::uint32_t>(parse_int_or_throw(fields[0]));
  const auto rd = parse_insn_reg(fields[1]);
  const auto imm = static_cast<std::int32_t>(parse_int_or_throw(fields[2]));
  return ok(EncodeResult::Word(encode_j(opcode, rd, imm)));
}

}  // namespace c4c::backend::riscv::assembler::encoder
