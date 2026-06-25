#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::codegen {

enum class Rv64AsmRegisterBank {
  Gpr,
  Vector,
};

struct Rv64AsmRegister {
  Rv64AsmRegisterBank bank = Rv64AsmRegisterBank::Gpr;
  std::uint32_t physical_index = 0;
};

struct Rv64InsnDLine {
  std::uint32_t major = 0;
  std::uint32_t operation = 0;
  Rv64AsmRegister destination;
  Rv64AsmRegister lhs;
  Rv64AsmRegister rhs;
  Rv64AsmRegister accumulator;
  std::uint32_t dtype = 0;
};

struct Rv64LiLine {
  Rv64AsmRegister destination;
  std::int64_t immediate = 0;
};

struct Rv64RetLine {};

using Rv64AsmLine = std::variant<Rv64InsnDLine, Rv64LiLine, Rv64RetLine>;

[[nodiscard]] std::optional<Rv64AsmLine> parse_rv64_asm_line(
    std::string_view line);

[[nodiscard]] std::optional<std::vector<std::uint8_t>> encode_rv64_asm_line(
    const Rv64AsmLine& line);

}  // namespace c4c::backend::riscv::codegen
