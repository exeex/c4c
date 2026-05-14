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

enum class MachineOriginReason {
  Unknown,
  BirInstruction,
  BirTerminator,
  Synthetic,
  TargetExpansion,
};

struct MachineOrigin {
  MachineOriginReason reason = MachineOriginReason::Unknown;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::optional<std::size_t> instruction_index;
  std::optional<std::size_t> target_instruction_index;
};

template <typename TargetInstruction = std::monostate>
struct MachineInstruction {
  TargetOpcode opcode = 0;
  std::vector<Operand> operands;
  TargetInstruction target;
  std::optional<MachineOrigin> origin;
};

template <typename TargetInstruction = std::monostate>
struct MachineBlock {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t index = 0;
  std::vector<MachineInstruction<TargetInstruction>> instructions;
};

template <typename TargetInstruction = std::monostate>
struct MachineFunction {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::vector<MachineBlock<TargetInstruction>> blocks;
};

template <typename TargetInstruction = std::monostate>
struct MachineModule {
  std::vector<MachineFunction<TargetInstruction>> functions;
};

template <typename TargetInstruction = std::monostate>
using MachineNode = MachineInstruction<TargetInstruction>;

template <typename Inst>
struct Block {
  c4c::BlockLabelId label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t index = 0;
  std::vector<Inst> instructions;
};

template <typename Inst>
struct Function {
  c4c::FunctionNameId name = c4c::kInvalidFunctionName;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::vector<Block<Inst>> blocks;
};

template <typename TargetInstruction>
[[nodiscard]] bool empty(const MachineFunction<TargetInstruction>& function) {
  for (const auto& block : function.blocks) {
    if (!block.instructions.empty()) {
      return false;
    }
  }
  return true;
}

template <typename Inst>
[[nodiscard]] bool empty(const Function<Inst>& function) {
  for (const auto& block : function.blocks) {
    if (!block.instructions.empty()) {
      return false;
    }
  }
  return true;
}

template <typename TargetInstruction>
[[nodiscard]] std::vector<MachineInstruction<TargetInstruction>> flatten_instructions(
    const MachineFunction<TargetInstruction>& function) {
  std::vector<MachineInstruction<TargetInstruction>> instructions;
  for (const auto& block : function.blocks) {
    instructions.insert(instructions.end(), block.instructions.begin(), block.instructions.end());
  }
  return instructions;
}

template <typename Inst>
[[nodiscard]] std::vector<Inst> flatten_instructions(const Function<Inst>& function) {
  std::vector<Inst> instructions;
  for (const auto& block : function.blocks) {
    instructions.insert(instructions.end(), block.instructions.begin(), block.instructions.end());
  }
  return instructions;
}

template <typename TargetInstruction>
void append_instruction(MachineFunction<TargetInstruction>& function,
                        c4c::BlockLabelId block_label,
                        std::size_t index,
                        MachineInstruction<TargetInstruction> instruction) {
  for (auto& block : function.blocks) {
    if (block.block_label == block_label || block.index == index) {
      block.block_label = block_label;
      block.instructions.push_back(std::move(instruction));
      return;
    }
  }
  MachineBlock<TargetInstruction> block{
      .block_label = block_label,
      .index = index,
  };
  block.instructions.push_back(std::move(instruction));
  function.blocks.push_back(std::move(block));
}

template <typename Inst>
void append_instruction(Function<Inst>& function,
                        c4c::BlockLabelId label,
                        std::size_t index,
                        Inst instruction) {
  for (auto& block : function.blocks) {
    if (block.label == label || block.block_label == label || block.index == index) {
      block.label = label;
      block.block_label = label;
      block.instructions.push_back(std::move(instruction));
      return;
    }
  }
  Block<Inst> block{
      .label = label,
      .block_label = label,
      .index = index,
  };
  block.instructions.push_back(std::move(instruction));
  function.blocks.push_back(std::move(block));
}

}  // namespace c4c::backend::mir
