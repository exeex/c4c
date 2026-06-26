#include "rv64_line_assembler.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

constexpr std::uint32_t encode_i_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::int32_t imm12) {
  return ((static_cast<std::uint32_t>(imm12) & 0xfffu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_r_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::uint32_t rs2, std::uint32_t funct7) {
  return ((funct7 & 0x7fu) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_s_type(std::uint32_t opcode, std::uint32_t funct3,
                                      std::uint32_t rs1, std::uint32_t rs2,
                                      std::int32_t imm12) {
  const auto imm = static_cast<std::uint32_t>(imm12) & 0xfffu;
  return ((imm >> 5) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((imm & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_b_type(std::uint32_t opcode, std::uint32_t funct3,
                                      std::uint32_t rs1, std::uint32_t rs2,
                                      std::int32_t imm13) {
  const auto imm = static_cast<std::uint32_t>(imm13);
  return (((imm >> 12) & 0x1u) << 31) | (((imm >> 5) & 0x3fu) << 25) |
         ((rs2 & 0x1fu) << 20) | ((rs1 & 0x1fu) << 15) |
         ((funct3 & 0x7u) << 12) | (((imm >> 1) & 0xfu) << 8) |
         (((imm >> 11) & 0x1u) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_j_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::int32_t imm21) {
  const auto imm = static_cast<std::uint32_t>(imm21);
  return (((imm >> 20) & 0x1u) << 31) | (((imm >> 1) & 0x3ffu) << 21) |
         (((imm >> 11) & 0x1u) << 20) | (((imm >> 12) & 0xffu) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_u_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::int32_t imm20) {
  return ((static_cast<std::uint32_t>(imm20) & 0xfffffu) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

void append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  bytes.push_back(static_cast<std::uint8_t>(word & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 8) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 16) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 24) & 0xffu));
}

void append_le64(std::vector<std::uint8_t>& bytes, std::uint64_t word) {
  for (int shift = 0; shift < 64; shift += 8) {
    bytes.push_back(static_cast<std::uint8_t>((word >> shift) & 0xffu));
  }
}

std::string_view trim_ascii(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t' ||
                          text.front() == '\n' || text.front() == '\r')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t' ||
                          text.back() == '\n' || text.back() == '\r')) {
    text.remove_suffix(1);
  }
  return text;
}

bool starts_with_token(std::string_view text, std::string_view token) {
  return text.size() >= token.size() && text.substr(0, token.size()) == token &&
         (text.size() == token.size() || text[token.size()] == ' ' ||
          text[token.size()] == '\t');
}

std::optional<std::vector<std::string_view>> split_fields(
    std::string_view text,
    std::size_t expected_count) {
  std::vector<std::string_view> fields;
  while (true) {
    const std::size_t comma = text.find(',');
    const bool last = comma == std::string_view::npos;
    fields.push_back(trim_ascii(last ? text : text.substr(0, comma)));
    if (last) {
      break;
    }
    text.remove_prefix(comma + 1);
    if (fields.size() == expected_count) {
      return std::nullopt;
    }
  }
  if (fields.size() != expected_count ||
      std::any_of(fields.begin(), fields.end(), [](std::string_view field) {
        return field.empty();
      })) {
    return std::nullopt;
  }
  return fields;
}

std::optional<std::uint32_t> parse_u32_field(std::string_view text,
                                             std::uint32_t max_value) {
  text = trim_ascii(text);
  if (text.empty()) {
    return std::nullopt;
  }
  int base = 10;
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text.remove_prefix(2);
    base = 16;
  }
  std::uint32_t value = 0;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, base);
  if (ec != std::errc{} || ptr != end || value > max_value) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint64_t> parse_unsigned_digits(std::string_view text,
                                                   int base) {
  std::uint64_t value = 0;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, base);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::int64_t> parse_signed_field(std::string_view text) {
  text = trim_ascii(text);
  if (text.empty()) {
    return std::nullopt;
  }
  bool negative = false;
  if (text.front() == '-' || text.front() == '+') {
    negative = text.front() == '-';
    text.remove_prefix(1);
    if (text.empty()) {
      return std::nullopt;
    }
  }
  int base = 10;
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text.remove_prefix(2);
    base = 16;
    if (text.empty()) {
      return std::nullopt;
    }
  }
  const auto magnitude = parse_unsigned_digits(text, base);
  if (!magnitude.has_value()) {
    return std::nullopt;
  }
  constexpr auto int64_max =
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
  if ((!negative && *magnitude > int64_max) ||
      (negative && *magnitude > int64_max + 1ull)) {
    return std::nullopt;
  }
  if (negative && *magnitude == int64_max + 1ull) {
    return INT64_MIN;
  }
  const auto signed_magnitude = static_cast<std::int64_t>(*magnitude);
  return negative ? -signed_magnitude : signed_magnitude;
}

bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

bool fits_signed_20_bit_immediate(std::int64_t value) {
  return value >= -(1 << 19) && value <= ((1 << 19) - 1);
}

bool fits_branch_immediate(std::int64_t value) {
  return value >= -4096 && value <= 4094 && value % 2 == 0;
}

bool fits_jump_immediate(std::int64_t value) {
  return value >= -(1 << 20) && value <= ((1 << 20) - 2) && value % 2 == 0;
}

bool fits_unsigned_shift_immediate(std::int64_t value, std::uint32_t max_value) {
  return value >= 0 && value <= max_value;
}

std::optional<std::uint32_t> gpr_number(std::string_view name) {
  if (name.size() >= 2 && name.front() == 'x') {
    std::uint32_t value = 0;
    const char* const begin = name.data() + 1;
    const char* const end = name.data() + name.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc{} && ptr == end && value <= 31) {
      return value;
    }
    return std::nullopt;
  }
  if (name == "zero") return 0;
  if (name == "ra") return 1;
  if (name == "sp") return 2;
  if (name == "gp") return 3;
  if (name == "tp") return 4;
  if (name == "t0") return 5;
  if (name == "t1") return 6;
  if (name == "t2") return 7;
  if (name == "s0" || name == "fp") return 8;
  if (name == "s1") return 9;
  if (name == "a0") return 10;
  if (name == "a1") return 11;
  if (name == "a2") return 12;
  if (name == "a3") return 13;
  if (name == "a4") return 14;
  if (name == "a5") return 15;
  if (name == "a6") return 16;
  if (name == "a7") return 17;
  if (name == "s2") return 18;
  if (name == "s3") return 19;
  if (name == "s4") return 20;
  if (name == "s5") return 21;
  if (name == "s6") return 22;
  if (name == "s7") return 23;
  if (name == "s8") return 24;
  if (name == "s9") return 25;
  if (name == "s10") return 26;
  if (name == "s11") return 27;
  if (name == "t3") return 28;
  if (name == "t4") return 29;
  if (name == "t5") return 30;
  if (name == "t6") return 31;
  return std::nullopt;
}

std::optional<Rv64AsmRegister> parse_gpr(std::string_view token) {
  const auto number = gpr_number(trim_ascii(token));
  if (!number.has_value()) {
    return std::nullopt;
  }
  return Rv64AsmRegister{
      .bank = Rv64AsmRegisterBank::Gpr,
      .physical_index = *number,
  };
}

bool is_symbol_start(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isalpha(uc) != 0 || c == '_' || c == '.' || c == '$';
}

bool is_symbol_continue(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) != 0 || c == '_' || c == '.' || c == '$';
}

bool is_valid_symbol(std::string_view symbol) {
  if (symbol.empty() || !is_symbol_start(symbol.front())) {
    return false;
  }
  for (const char c : symbol.substr(1)) {
    if (!is_symbol_continue(c)) {
      return false;
    }
  }
  return true;
}

std::optional<Rv64AsmRegister> parse_vector_register(std::string_view token) {
  token = trim_ascii(token);
  if (token.size() < 2 || token.front() != 'v') {
    return std::nullopt;
  }
  std::uint32_t number = 0;
  const char* const begin = token.data() + 1;
  const char* const end = token.data() + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, number);
  if (ec != std::errc{} || ptr != end || number > 31) {
    return std::nullopt;
  }
  return Rv64AsmRegister{
      .bank = Rv64AsmRegisterBank::Vector,
      .physical_index = number,
  };
}

std::optional<Rv64AsmLine> parse_insn_d(std::string_view line) {
  constexpr std::string_view prefix = ".insn.d";
  if (!starts_with_token(line, prefix)) {
    return std::nullopt;
  }
  line.remove_prefix(prefix.size());
  const auto fields = split_fields(trim_ascii(line), 7);
  if (!fields.has_value()) {
    return std::nullopt;
  }

  const auto major = parse_u32_field((*fields)[0], 0x7f);
  const auto operation = parse_u32_field((*fields)[1], 0xff);
  const auto destination = parse_vector_register((*fields)[2]);
  const auto lhs = parse_vector_register((*fields)[3]);
  const auto rhs = parse_vector_register((*fields)[4]);
  const auto accumulator = parse_vector_register((*fields)[5]);
  const auto dtype = parse_u32_field((*fields)[6], 0xffff);
  if (!major.has_value() || !operation.has_value() || !destination.has_value() ||
      !lhs.has_value() || !rhs.has_value() || !accumulator.has_value() ||
      !dtype.has_value()) {
    return std::nullopt;
  }
  return Rv64AsmLine{Rv64InsnDLine{
      .major = *major,
      .operation = *operation,
      .destination = *destination,
      .lhs = *lhs,
      .rhs = *rhs,
      .accumulator = *accumulator,
      .dtype = *dtype,
  }};
}

std::optional<Rv64AsmLine> parse_li(std::string_view line) {
  constexpr std::string_view prefix = "li";
  if (!starts_with_token(line, prefix)) {
    return std::nullopt;
  }
  line.remove_prefix(prefix.size());
  const auto fields = split_fields(trim_ascii(line), 2);
  if (!fields.has_value()) {
    return std::nullopt;
  }
  const auto destination = parse_gpr((*fields)[0]);
  const auto immediate = parse_signed_field((*fields)[1]);
  if (!destination.has_value() || !immediate.has_value() ||
      !fits_signed_12_bit_immediate(*immediate)) {
    return std::nullopt;
  }
  return Rv64AsmLine{Rv64LiLine{
      .destination = *destination,
      .immediate = *immediate,
  }};
}

struct Rv64RSpec {
  std::string_view mnemonic;
  std::uint32_t opcode;
  std::uint32_t funct3;
  std::uint32_t funct7;
};

struct Rv64ISpec {
  std::string_view mnemonic;
  std::uint32_t opcode;
  std::uint32_t funct3;
};

struct Rv64ShiftSpec {
  std::string_view mnemonic;
  std::uint32_t opcode;
  std::uint32_t funct3;
  std::uint32_t funct7;
  std::uint32_t max_shamt;
};

struct Rv64LoadStoreSpec {
  std::string_view mnemonic;
  std::uint32_t opcode;
  std::uint32_t funct3;
};

struct Rv64USpec {
  std::string_view mnemonic;
  std::uint32_t opcode;
};

struct Rv64BSpec {
  std::string_view mnemonic;
  std::uint32_t funct3;
};

template <typename Spec, std::size_t Count>
const Spec* find_spec(std::string_view mnemonic, const Spec (&specs)[Count]) {
  for (const auto& spec : specs) {
    if (spec.mnemonic == mnemonic) {
      return &spec;
    }
  }
  return nullptr;
}

std::optional<std::pair<std::string_view, std::string_view>> split_mnemonic(
    std::string_view line) {
  line = trim_ascii(line);
  const auto space = line.find_first_of(" \t");
  if (space == std::string_view::npos) {
    return std::nullopt;
  }
  return std::pair{line.substr(0, space), trim_ascii(line.substr(space + 1))};
}

std::optional<std::pair<std::string_view, Rv64AsmRegister>> parse_mem_operand(
    std::string_view field) {
  field = trim_ascii(field);
  const auto open = field.find('(');
  const auto close = field.find(')');
  if (open == std::string_view::npos || close == std::string_view::npos ||
      close + 1 != field.size() || open == 0 || close <= open + 1) {
    return std::nullopt;
  }
  const auto base = parse_gpr(field.substr(open + 1, close - open - 1));
  if (!base.has_value()) {
    return std::nullopt;
  }
  return std::pair{trim_ascii(field.substr(0, open)), *base};
}

std::optional<Rv64AsmLine> parse_rv64i_line(std::string_view line) {
  constexpr Rv64RSpec r_specs[] = {
      {"add", 0x33, 0, 0x00},  {"sub", 0x33, 0, 0x20},
      {"sll", 0x33, 1, 0x00},  {"slt", 0x33, 2, 0x00},
      {"sltu", 0x33, 3, 0x00}, {"xor", 0x33, 4, 0x00},
      {"srl", 0x33, 5, 0x00},  {"sra", 0x33, 5, 0x20},
      {"or", 0x33, 6, 0x00},   {"and", 0x33, 7, 0x00},
      {"addw", 0x3b, 0, 0x00}, {"subw", 0x3b, 0, 0x20},
      {"sllw", 0x3b, 1, 0x00}, {"srlw", 0x3b, 5, 0x00},
      {"sraw", 0x3b, 5, 0x20},
  };
  constexpr Rv64ISpec i_specs[] = {
      {"addi", 0x13, 0}, {"slti", 0x13, 2},  {"sltiu", 0x13, 3},
      {"xori", 0x13, 4}, {"ori", 0x13, 6},   {"andi", 0x13, 7},
      {"addiw", 0x1b, 0},
  };
  constexpr Rv64ShiftSpec shift_specs[] = {
      {"slli", 0x13, 1, 0x00, 63}, {"srli", 0x13, 5, 0x00, 63},
      {"srai", 0x13, 5, 0x20, 63}, {"slliw", 0x1b, 1, 0x00, 31},
      {"srliw", 0x1b, 5, 0x00, 31}, {"sraiw", 0x1b, 5, 0x20, 31},
  };
  constexpr Rv64LoadStoreSpec load_specs[] = {
      {"lb", 0x03, 0},  {"lh", 0x03, 1},  {"lw", 0x03, 2},
      {"ld", 0x03, 3},  {"lbu", 0x03, 4}, {"lhu", 0x03, 5},
      {"lwu", 0x03, 6},
  };
  constexpr Rv64LoadStoreSpec store_specs[] = {
      {"sb", 0x23, 0},
      {"sh", 0x23, 1},
      {"sw", 0x23, 2},
      {"sd", 0x23, 3},
  };
  constexpr Rv64USpec u_specs[] = {
      {"lui", 0x37},
      {"auipc", 0x17},
  };
  constexpr Rv64BSpec b_specs[] = {
      {"beq", 0},  {"bne", 1},  {"blt", 4},
      {"bge", 5},  {"bltu", 6}, {"bgeu", 7},
  };

  const auto parts = split_mnemonic(line);
  if (!parts.has_value()) {
    return std::nullopt;
  }
  const auto [mnemonic, operands] = *parts;

  if (const auto* spec = find_spec(mnemonic, r_specs)) {
    const auto fields = split_fields(operands, 3);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto rs1 = parse_gpr((*fields)[1]);
    const auto rs2 = parse_gpr((*fields)[2]);
    if (!rd.has_value() || !rs1.has_value() || !rs2.has_value()) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::RType,
                                 .opcode = spec->opcode,
                                 .funct3 = spec->funct3,
                                 .funct7 = spec->funct7,
                                 .destination = *rd,
                                 .lhs = *rs1,
                                 .rhs = *rs2}};
  }

  if (const auto* spec = find_spec(mnemonic, i_specs)) {
    const auto fields = split_fields(operands, 3);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto rs1 = parse_gpr((*fields)[1]);
    const auto immediate = parse_signed_field((*fields)[2]);
    if (!rd.has_value() || !rs1.has_value() || !immediate.has_value() ||
        !fits_signed_12_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::IType,
                                 .opcode = spec->opcode,
                                 .funct3 = spec->funct3,
                                 .destination = *rd,
                                 .lhs = *rs1,
                                 .immediate = *immediate}};
  }

  if (const auto* spec = find_spec(mnemonic, shift_specs)) {
    const auto fields = split_fields(operands, 3);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto rs1 = parse_gpr((*fields)[1]);
    const auto shamt = parse_signed_field((*fields)[2]);
    if (!rd.has_value() || !rs1.has_value() || !shamt.has_value() ||
        !fits_unsigned_shift_immediate(*shamt, spec->max_shamt)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::IType,
                                 .opcode = spec->opcode,
                                 .funct3 = spec->funct3,
                                 .funct7 = spec->funct7,
                                 .destination = *rd,
                                 .lhs = *rs1,
                                 .immediate = (spec->funct7 << 5) |
                                              static_cast<std::uint32_t>(*shamt)}};
  }

  if (const auto* spec = find_spec(mnemonic, load_specs)) {
    const auto fields = split_fields(operands, 2);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto mem = parse_mem_operand((*fields)[1]);
    if (!rd.has_value() || !mem.has_value()) {
      return std::nullopt;
    }
    const auto immediate = parse_signed_field(mem->first);
    if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::IType,
                                 .opcode = spec->opcode,
                                 .funct3 = spec->funct3,
                                 .destination = *rd,
                                 .lhs = mem->second,
                                 .immediate = *immediate}};
  }

  if (const auto* spec = find_spec(mnemonic, store_specs)) {
    const auto fields = split_fields(operands, 2);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rs2 = parse_gpr((*fields)[0]);
    const auto mem = parse_mem_operand((*fields)[1]);
    if (!rs2.has_value() || !mem.has_value()) {
      return std::nullopt;
    }
    const auto immediate = parse_signed_field(mem->first);
    if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::SType,
                                 .opcode = spec->opcode,
                                 .funct3 = spec->funct3,
                                 .lhs = mem->second,
                                 .rhs = *rs2,
                                 .immediate = *immediate}};
  }

  if (const auto* spec = find_spec(mnemonic, u_specs)) {
    const auto fields = split_fields(operands, 2);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto immediate = parse_signed_field((*fields)[1]);
    if (!rd.has_value() || !immediate.has_value() ||
        !fits_signed_20_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::UType,
                                 .opcode = spec->opcode,
                                 .destination = *rd,
                                 .immediate = *immediate}};
  }

  if (const auto* spec = find_spec(mnemonic, b_specs)) {
    const auto fields = split_fields(operands, 3);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rs1 = parse_gpr((*fields)[0]);
    const auto rs2 = parse_gpr((*fields)[1]);
    const auto target = trim_ascii((*fields)[2]);
    if (!rs1.has_value() || !rs2.has_value() || !is_valid_symbol(target)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64BranchLine{.opcode = 0x63,
                                      .funct3 = spec->funct3,
                                      .lhs = *rs1,
                                      .rhs = *rs2,
                                      .target_label = std::string(target)}};
  }

  if (mnemonic == "jal") {
    const auto fields = split_fields(operands, 2);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto target = trim_ascii((*fields)[1]);
    if (!rd.has_value() || !is_valid_symbol(target)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64JumpLine{
        .destination = *rd,
        .target_label = std::string(target),
    }};
  }

  if (mnemonic == "jalr") {
    const auto fields = split_fields(operands, 2);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    const auto rd = parse_gpr((*fields)[0]);
    const auto mem = parse_mem_operand((*fields)[1]);
    if (!rd.has_value() || !mem.has_value()) {
      return std::nullopt;
    }
    const auto immediate = parse_signed_field(mem->first);
    if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    return Rv64AsmLine{Rv64ILine{.format = Rv64IFormat::IType,
                                 .opcode = 0x67,
                                 .funct3 = 0,
                                 .destination = *rd,
                                 .lhs = mem->second,
                                 .immediate = *immediate}};
  }

  return std::nullopt;
}

std::optional<std::uint32_t> register_field(const Rv64AsmRegister& reg,
                                            Rv64AsmRegisterBank expected_bank) {
  if (reg.bank != expected_bank || reg.physical_index > 31) {
    return std::nullopt;
  }
  return reg.physical_index;
}

std::optional<std::uint64_t> encode_insn_d_word(const Rv64InsnDLine& insn) {
  const auto rd = register_field(insn.destination, Rv64AsmRegisterBank::Vector);
  const auto rs1 = register_field(insn.lhs, Rv64AsmRegisterBank::Vector);
  const auto rs2 = register_field(insn.rhs, Rv64AsmRegisterBank::Vector);
  const auto rs4 = register_field(insn.accumulator, Rv64AsmRegisterBank::Vector);
  if (insn.major > 0x7f || insn.operation > 0xff || insn.dtype > 0xffff ||
      !rd.has_value() || !rs1.has_value() || !rs2.has_value() ||
      !rs4.has_value()) {
    return std::nullopt;
  }

  // First supported EV64 shape, matching object_emission.cpp.
  return std::uint64_t{0x3f} | (static_cast<std::uint64_t>(*rd) << 7) |
         (static_cast<std::uint64_t>(*rs1) << 15) |
         (static_cast<std::uint64_t>(*rs2) << 20) |
         (static_cast<std::uint64_t>(insn.major) << 25) |
         (static_cast<std::uint64_t>(insn.operation) << 32) |
         (static_cast<std::uint64_t>(*rs4) << 40) |
         (static_cast<std::uint64_t>(insn.dtype) << 48);
}

}  // namespace

std::optional<Rv64AsmLine> parse_rv64_asm_line(std::string_view line) {
  line = trim_ascii(line);
  if (line.empty()) {
    return std::nullopt;
  }
  if (line == "ret") {
    return Rv64AsmLine{Rv64RetLine{}};
  }
  if (auto parsed = parse_insn_d(line); parsed.has_value()) {
    return parsed;
  }
  if (auto parsed = parse_li(line); parsed.has_value()) {
    return parsed;
  }
  return parse_rv64i_line(line);
}

std::optional<std::vector<std::uint8_t>> encode_rv64_asm_line(
    const Rv64AsmLine& line) {
  std::vector<std::uint8_t> bytes;
  if (const auto* insn_d = std::get_if<Rv64InsnDLine>(&line)) {
    const auto word = encode_insn_d_word(*insn_d);
    if (!word.has_value()) {
      return std::nullopt;
    }
    append_le64(bytes, *word);
    return bytes;
  }
  if (const auto* li = std::get_if<Rv64LiLine>(&line)) {
    const auto destination = register_field(li->destination, Rv64AsmRegisterBank::Gpr);
    if (!destination.has_value() || !fits_signed_12_bit_immediate(li->immediate)) {
      return std::nullopt;
    }
    append_le32(bytes,
                encode_i_type(0x13,
                              *destination,
                              0,
                              0,
                              static_cast<std::int32_t>(li->immediate)));
    return bytes;
  }
  if (std::holds_alternative<Rv64RetLine>(line)) {
    append_le32(bytes, encode_i_type(0x67, 0, 0, 1, 0));
    return bytes;
  }
  if (const auto* rv64i = std::get_if<Rv64ILine>(&line)) {
    const auto rd = register_field(rv64i->destination, Rv64AsmRegisterBank::Gpr);
    const auto rs1 = register_field(rv64i->lhs, Rv64AsmRegisterBank::Gpr);
    const auto rs2 = register_field(rv64i->rhs, Rv64AsmRegisterBank::Gpr);
    switch (rv64i->format) {
      case Rv64IFormat::RType:
        if (!rd.has_value() || !rs1.has_value() || !rs2.has_value()) {
          return std::nullopt;
        }
        append_le32(bytes, encode_r_type(rv64i->opcode, *rd, rv64i->funct3,
                                         *rs1, *rs2, rv64i->funct7));
        return bytes;
      case Rv64IFormat::IType:
        if (!rd.has_value() || !rs1.has_value() ||
            !fits_signed_12_bit_immediate(rv64i->immediate)) {
          return std::nullopt;
        }
        append_le32(bytes, encode_i_type(rv64i->opcode, *rd, rv64i->funct3,
                                         *rs1,
                                         static_cast<std::int32_t>(rv64i->immediate)));
        return bytes;
      case Rv64IFormat::SType:
        if (!rs1.has_value() || !rs2.has_value() ||
            !fits_signed_12_bit_immediate(rv64i->immediate)) {
          return std::nullopt;
        }
        append_le32(bytes, encode_s_type(rv64i->opcode, rv64i->funct3, *rs1,
                                         *rs2,
                                         static_cast<std::int32_t>(rv64i->immediate)));
        return bytes;
      case Rv64IFormat::UType:
        if (!rd.has_value() || !fits_signed_20_bit_immediate(rv64i->immediate)) {
          return std::nullopt;
        }
        append_le32(bytes, encode_u_type(rv64i->opcode, *rd,
                                         static_cast<std::int32_t>(rv64i->immediate)));
        return bytes;
    }
  }
  if (const auto* branch = std::get_if<Rv64BranchLine>(&line)) {
    const auto rs1 = register_field(branch->lhs, Rv64AsmRegisterBank::Gpr);
    const auto rs2 = register_field(branch->rhs, Rv64AsmRegisterBank::Gpr);
    if (!branch->target_label.empty() || !rs1.has_value() || !rs2.has_value() ||
        !fits_branch_immediate(branch->immediate)) {
      return std::nullopt;
    }
    append_le32(bytes,
                encode_b_type(branch->opcode,
                              branch->funct3,
                              *rs1,
                              *rs2,
                              static_cast<std::int32_t>(branch->immediate)));
    return bytes;
  }
  if (const auto* jump = std::get_if<Rv64JumpLine>(&line)) {
    const auto rd = register_field(jump->destination, Rv64AsmRegisterBank::Gpr);
    if (!jump->target_label.empty() || !rd.has_value() ||
        !fits_jump_immediate(jump->immediate)) {
      return std::nullopt;
    }
    append_le32(bytes,
                encode_j_type(0x6f,
                              *rd,
                              static_cast<std::int32_t>(jump->immediate)));
    return bytes;
  }
  return std::nullopt;
}

std::optional<std::uint64_t> rv64_asm_line_size_bytes(const Rv64AsmLine& line) {
  if (std::holds_alternative<Rv64InsnDLine>(line)) {
    return 8;
  }
  if (std::holds_alternative<Rv64LiLine>(line) ||
      std::holds_alternative<Rv64RetLine>(line) ||
      std::holds_alternative<Rv64ILine>(line) ||
      std::holds_alternative<Rv64BranchLine>(line) ||
      std::holds_alternative<Rv64JumpLine>(line)) {
    return 4;
  }
  return std::nullopt;
}

}  // namespace c4c::backend::riscv::codegen
