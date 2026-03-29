#include "mod.hpp"

#include <optional>

namespace c4c::backend::aarch64::assembler::encoder {

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

std::optional<std::uint32_t> parse_unsigned(const std::string& text) {
  if (text.empty()) return std::nullopt;
  std::uint32_t value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return std::nullopt;
    value = value * 10u + static_cast<std::uint32_t>(c - '0');
  }
  return value;
}

std::optional<std::uint32_t> parse_w_register(const std::string& text) {
  if (text.size() < 2 || text.front() != 'w') return std::nullopt;
  const auto reg = parse_unsigned(text.substr(1));
  if (!reg.has_value() || *reg > 31) return std::nullopt;
  return reg;
}

std::optional<std::uint32_t> parse_mov_immediate(const std::string& text) {
  if (text.size() < 2 || text.front() != '#') return std::nullopt;
  const auto imm = parse_unsigned(text.substr(1));
  if (!imm.has_value() || *imm > 65535u) return std::nullopt;
  return imm;
}

EncodeResult encode_instruction(const std::string& mnemonic,
                                const std::vector<Operand>& operands,
                                const std::string& raw_operands) {
  (void)raw_operands;
  if (mnemonic == "ret" && operands.empty()) {
    return EncodeResult{
        .encoded = true,
        .kind = EncodeResultKind::Word,
        .word = 0xd65f03c0u,
    };
  }

  if (mnemonic == "mov" && operands.size() == 2) {
    const auto rd = parse_w_register(operands[0].text);
    const auto imm = parse_mov_immediate(operands[1].text);
    if (rd.has_value() && imm.has_value()) {
      return EncodeResult{
          .encoded = true,
          .kind = EncodeResultKind::Word,
          .word = 0x52800000u | (*imm << 5) | *rd,
      };
    }
  }

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
