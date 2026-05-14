#include "src/backend/mir/aarch64/module/module.hpp"

#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace mir = c4c::backend::mir;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

int common_carrier_keeps_function_block_and_origin_identity() {
  aarch64_module::MachineFunction function;
  function.function_name = c4c::FunctionNameId{17};

  aarch64_module::MachineInstruction instruction;
  instruction.opcode = 42;
  instruction.origin = mir::MachineOrigin{
      .reason = mir::MachineOriginReason::BirInstruction,
      .function_name = c4c::FunctionNameId{17},
      .block_label = c4c::BlockLabelId{23},
      .instruction_index = std::size_t{5},
      .target_instruction_index = std::size_t{7},
  };

  mir::append_instruction(function, c4c::BlockLabelId{23}, 3, std::move(instruction));

  if (function.function_name != c4c::FunctionNameId{17}) {
    return fail("expected common MIR carrier to preserve function identity");
  }
  if (function.blocks.size() != 1) {
    return fail("expected common MIR carrier to create one block for appended instruction");
  }
  const auto& block = function.blocks.front();
  if (block.block_label != c4c::BlockLabelId{23} || block.index != 3) {
    return fail("expected common MIR carrier to preserve block label and index identity");
  }
  if (block.instructions.size() != 1 || block.instructions.front().opcode != 42) {
    return fail("expected common MIR carrier to retain appended machine instruction");
  }
  if (!block.successors.empty()) {
    return fail("expected common MIR carrier to leave successor metadata empty by default");
  }
  const auto& origin = block.instructions.front().origin;
  if (!origin.has_value() || origin->reason != mir::MachineOriginReason::BirInstruction ||
      origin->function_name != c4c::FunctionNameId{17} ||
      origin->block_label != c4c::BlockLabelId{23} ||
      origin->instruction_index != std::size_t{5} ||
      origin->target_instruction_index != std::size_t{7}) {
    return fail("expected common MIR origin to preserve BIR function/block provenance");
  }
  return 0;
}

int common_carrier_preserves_typed_successor_metadata() {
  aarch64_module::MachineBlock block;
  block.block_label = c4c::BlockLabelId{31};
  block.index = 2;
  block.successors.push_back(mir::MachineBlockSuccessor{
      .target_label = c4c::BlockLabelId{37},
      .kind = mir::MachineBlockSuccessorKind::Unconditional,
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirTerminator,
              .function_name = c4c::FunctionNameId{41},
              .block_label = c4c::BlockLabelId{31},
          },
  });

  if (block.successors.size() != 1) {
    return fail("expected common MIR carrier to retain one block successor");
  }
  const auto& successor = block.successors.front();
  if (successor.target_label != c4c::BlockLabelId{37} ||
      successor.kind != mir::MachineBlockSuccessorKind::Unconditional ||
      !successor.origin.has_value() ||
      successor.origin->reason != mir::MachineOriginReason::BirTerminator ||
      successor.origin->function_name != c4c::FunctionNameId{41} ||
      successor.origin->block_label != c4c::BlockLabelId{31}) {
    return fail("expected common MIR successor to preserve typed target and provenance");
  }
  return 0;
}

int common_hierarchy_walks_without_owning_target_payload() {
  static_assert(
      std::is_same_v<decltype(aarch64_module::MachineInstruction{}.target),
                     aarch64_codegen::InstructionRecord>,
      "common MIR instruction must carry the target-owned instruction payload");

  aarch64_module::MachineModule module;
  module.functions.push_back(aarch64_module::MachineFunction{
      .function_name = c4c::FunctionNameId{53},
  });

  aarch64_module::MachineInstruction instruction;
  instruction.opcode = 71;
  instruction.target.family = aarch64_codegen::InstructionFamily::Return;
  instruction.target.surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode;
  instruction.target.opcode = aarch64_codegen::MachineOpcode::Unspecified;

  mir::append_instruction(module.functions.front(), c4c::BlockLabelId{59}, 4,
                          std::move(instruction));

  std::size_t visited = 0;
  bool saw_function = false;
  bool saw_block = false;
  bool saw_target_payload = false;
  mir::walk_instructions(module,
                         [&](const aarch64_module::MachineFunction& function,
                             const aarch64_module::MachineBlock& block,
                             const aarch64_module::MachineInstruction& visited_instruction) {
                           ++visited;
                           saw_function = function.function_name == c4c::FunctionNameId{53};
                           saw_block = block.block_label == c4c::BlockLabelId{59} &&
                                       block.index == 4;
                           saw_target_payload =
                               visited_instruction.target.family ==
                                   aarch64_codegen::InstructionFamily::Return &&
                               visited_instruction.target.surface ==
                                   aarch64_codegen::RecordSurfaceKind::MachineInstructionNode;
                         });

  if (visited != 1 || !saw_function || !saw_block || !saw_target_payload) {
    return fail("expected common MIR walker to preserve hierarchy and target-owned payload");
  }
  return 0;
}

int flatten_and_empty_helpers_use_hierarchical_carrier() {
  aarch64_module::MachineFunction function;
  function.function_name = c4c::FunctionNameId{19};
  function.blocks.push_back(aarch64_module::MachineBlock{
      .block_label = c4c::BlockLabelId{29},
      .index = 0,
  });

  if (!mir::empty(function)) {
    return fail("expected common MIR empty helper to treat blocks without instructions as empty");
  }

  aarch64_module::MachineInstruction instruction;
  instruction.opcode = 99;
  mir::append_instruction(function, c4c::BlockLabelId{29}, 0, std::move(instruction));

  const auto flat = mir::flatten_compatibility_instructions(function);
  if (flat.size() != 1 || flat.front().opcode != 99) {
    return fail(
        "expected common MIR compatibility flatten helper to preserve instruction identity");
  }
  if (mir::empty(function)) {
    return fail("expected common MIR empty helper to observe appended instructions");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = common_carrier_keeps_function_block_and_origin_identity();
      status != 0) {
    return status;
  }
  if (const int status = common_carrier_preserves_typed_successor_metadata();
      status != 0) {
    return status;
  }
  if (const int status = common_hierarchy_walks_without_owning_target_payload();
      status != 0) {
    return status;
  }
  if (const int status = flatten_and_empty_helpers_use_hierarchical_carrier();
      status != 0) {
    return status;
  }
  return 0;
}
