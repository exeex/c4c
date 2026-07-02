#pragma once

#include <cstdint>
#include <optional>
#include <string>
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

struct Rv64BranchLine {
  std::uint32_t opcode = 0;
  std::uint32_t funct3 = 0;
  Rv64AsmRegister lhs;
  Rv64AsmRegister rhs;
  std::int64_t immediate = 0;
  std::string target_label;
};

struct Rv64JumpLine {
  Rv64AsmRegister destination;
  std::int64_t immediate = 0;
  std::string target_label;
};

enum class Rv64IFormat {
  RType,
  IType,
  SType,
  UType,
};

struct Rv64ILine {
  Rv64IFormat format = Rv64IFormat::IType;
  std::uint32_t opcode = 0;
  std::uint32_t funct3 = 0;
  std::uint32_t funct7 = 0;
  Rv64AsmRegister destination;
  Rv64AsmRegister lhs;
  Rv64AsmRegister rhs;
  std::int64_t immediate = 0;
};

using Rv64AsmLine =
    std::variant<Rv64InsnDLine,
                 Rv64LiLine,
                 Rv64RetLine,
                 Rv64ILine,
                 Rv64BranchLine,
                 Rv64JumpLine>;

[[nodiscard]] std::uint32_t rv64_encode_u_type(std::uint32_t opcode,
                                                std::uint32_t rd,
                                                std::uint32_t imm20);

[[nodiscard]] std::uint32_t rv64_encode_i_type(std::uint32_t opcode,
                                                std::uint32_t rd,
                                                std::uint32_t funct3,
                                                std::uint32_t rs1,
                                                std::int32_t imm12);

[[nodiscard]] std::uint32_t rv64_encode_s_type(std::uint32_t opcode,
                                                std::uint32_t funct3,
                                                std::uint32_t rs1,
                                                std::uint32_t rs2,
                                                std::int32_t imm12);

[[nodiscard]] std::uint32_t rv64_encode_r_type(std::uint32_t opcode,
                                                std::uint32_t rd,
                                                std::uint32_t funct3,
                                                std::uint32_t rs1,
                                                std::uint32_t rs2,
                                                std::uint32_t funct7);

[[nodiscard]] std::uint32_t rv64_encode_b_type(std::uint32_t opcode,
                                                std::uint32_t funct3,
                                                std::uint32_t rs1,
                                                std::uint32_t rs2,
                                                std::int32_t imm13);

[[nodiscard]] std::uint32_t rv64_encode_j_type(std::uint32_t opcode,
                                                std::uint32_t rd,
                                                std::int32_t imm21);

void rv64_append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word);

void rv64_append_le64(std::vector<std::uint8_t>& bytes, std::uint64_t word);

[[nodiscard]] std::optional<Rv64AsmLine> parse_rv64_asm_line(
    std::string_view line);

[[nodiscard]] std::optional<std::vector<std::uint8_t>> encode_rv64_asm_line(
    const Rv64AsmLine& line);

[[nodiscard]] std::optional<std::uint64_t> rv64_asm_line_size_bytes(
    const Rv64AsmLine& line);

}  // namespace c4c::backend::riscv::codegen
