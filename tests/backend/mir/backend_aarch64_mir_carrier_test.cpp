#include "src/backend/mir/aarch64/module/module.hpp"

#include <iostream>
#include <string_view>

namespace {

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

  const auto flat = mir::flatten_instructions(function);
  if (flat.size() != 1 || flat.front().opcode != 99) {
    return fail("expected common MIR flatten helper to preserve machine instruction identity");
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
  if (const int status = flatten_and_empty_helpers_use_hierarchical_carrier();
      status != 0) {
    return status;
  }
  return 0;
}
