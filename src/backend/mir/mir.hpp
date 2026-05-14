#pragma once

#include "../backend.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::mir {

using TargetOpcode = std::uint32_t;
using RegisterBankId = std::uint16_t;
using RegisterViewId = std::uint16_t;
using RegisterIndex = std::uint16_t;

struct PhysicalRegister {
  RegisterBankId bank = 0;
  RegisterViewId view = 0;
  RegisterIndex index = 0;
};

enum class ImmediateKind {
  Signed,
  Unsigned,
  Boolean,
  NullPointer,
};

struct Immediate {
  ImmediateKind kind = ImmediateKind::Signed;
  std::int64_t signed_value = 0;
  std::uint64_t unsigned_value = 0;
};

struct Memory {
  std::optional<PhysicalRegister> base;
  std::optional<PhysicalRegister> index;
  std::int64_t displacement = 0;
  std::uint8_t scale = 1;
};

struct Symbol {
  c4c::LinkNameId name = c4c::kInvalidLinkName;
  std::int64_t addend = 0;
};

struct Label {
  c4c::BlockLabelId label = c4c::kInvalidBlockLabel;
};

using OperandPayload = std::variant<PhysicalRegister, Immediate, Memory, Symbol, Label>;

struct Operand {
  OperandPayload payload = Immediate{};
};

template <typename TargetInstruction = std::monostate>
struct MachineNode {
  TargetOpcode opcode = 0;
  std::vector<Operand> operands;
  TargetInstruction target;
};

template <typename Inst>
struct Block {
  c4c::BlockLabelId label = c4c::kInvalidBlockLabel;
  std::size_t index = 0;
  std::vector<Inst> instructions;
};

template <typename Inst>
struct Function {
  c4c::FunctionNameId name = c4c::kInvalidFunctionName;
  std::vector<Block<Inst>> blocks;
};

template <typename Inst>
[[nodiscard]] bool empty(const Function<Inst>& function) {
  for (const auto& block : function.blocks) {
    if (!block.instructions.empty()) {
      return false;
    }
  }
  return true;
}

template <typename Inst>
[[nodiscard]] std::vector<Inst> flatten_instructions(const Function<Inst>& function) {
  std::vector<Inst> instructions;
  for (const auto& block : function.blocks) {
    instructions.insert(instructions.end(), block.instructions.begin(), block.instructions.end());
  }
  return instructions;
}

template <typename Inst>
void append_instruction(Function<Inst>& function,
                        c4c::BlockLabelId label,
                        std::size_t index,
                        Inst instruction) {
  for (auto& block : function.blocks) {
    if (block.label == label || block.index == index) {
      block.instructions.push_back(std::move(instruction));
      return;
    }
  }
  Block<Inst> block{
      .label = label,
      .index = index,
  };
  block.instructions.push_back(std::move(instruction));
  function.blocks.push_back(std::move(block));
}

}  // namespace c4c::backend::mir
