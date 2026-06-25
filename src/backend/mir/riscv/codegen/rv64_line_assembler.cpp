#include "rv64_line_assembler.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

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

std::optional<std::int64_t> parse_signed_field(std::string_view text) {
  text = trim_ascii(text);
  if (text.empty()) {
    return std::nullopt;
  }
  std::int64_t value = 0;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
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
  return parse_li(line);
}

}  // namespace c4c::backend::riscv::codegen
