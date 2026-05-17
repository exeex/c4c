#include "prologue.hpp"

#include "instruction.hpp"

#include <cstddef>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

[[nodiscard]] bool has_simple_fixed_frame_plan(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  if (frame == nullptr || context.control_flow == nullptr) {
    return false;
  }
  if (frame->function_name != context.control_flow->function_name ||
      frame->function_name == c4c::kInvalidFunctionName ||
      frame->frame_alignment_bytes == 0 || frame->frame_size_bytes == 0) {
    return false;
  }
  if (frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr ||
      !frame->saved_callee_registers.empty()) {
    return false;
  }
  return true;
}

[[nodiscard]] module::MachineInstruction make_frame_machine_instruction(
    const module::FunctionLoweringContext& context,
    FrameInstructionKind frame_kind,
    c4c::BlockLabelId block_label,
    std::size_t block_index) {
  const auto* frame = context.frame_plan;
  InstructionRecord target = make_frame_instruction(FrameInstructionRecord{
      .frame_kind = frame_kind,
      .function_name = frame != nullptr ? frame->function_name : c4c::kInvalidFunctionName,
      .frame_size_bytes = frame != nullptr ? frame->frame_size_bytes : 0,
      .frame_alignment_bytes = frame != nullptr ? frame->frame_alignment_bytes : 0,
      .frame_slot_order = frame != nullptr
                              ? frame->frame_slot_order
                              : std::vector<prepare::PreparedFrameSlotId>{},
      .saved_callee_registers = frame != nullptr
                                    ? frame->saved_callee_registers
                                    : std::vector<prepare::PreparedSavedRegister>{},
      .has_dynamic_stack = frame != nullptr ? frame->has_dynamic_stack : false,
      .uses_frame_pointer_for_fixed_slots =
          frame != nullptr ? frame->uses_frame_pointer_for_fixed_slots : false,
      .source_frame = frame,
  });
  target.block_label = block_label;
  target.block_index = block_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::Synthetic,
              .function_name = frame != nullptr ? frame->function_name : c4c::kInvalidFunctionName,
              .block_label = block_label,
          },
  };
}

}  // namespace

void insert_prepared_frame_boundary_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function) {
  if (!has_simple_fixed_frame_plan(context) || function.blocks.empty()) {
    return;
  }

  auto& entry = function.blocks.front();
  entry.instructions.insert(
      entry.instructions.begin(),
      make_frame_machine_instruction(context,
                                     FrameInstructionKind::PrologueSetup,
                                     entry.block_label,
                                     entry.index));

  for (std::size_t block_index = 0; block_index < prepared_function.blocks.size() &&
                                  block_index < function.blocks.size();
       ++block_index) {
    if (prepared_function.blocks[block_index].terminator_kind !=
        c4c::backend::bir::TerminatorKind::Return) {
      continue;
    }
    auto& block = function.blocks[block_index];
    auto insertion_point = block.instructions.end();
    for (auto it = block.instructions.begin(); it != block.instructions.end(); ++it) {
      if (std::holds_alternative<ReturnInstructionRecord>(it->target.payload)) {
        insertion_point = it;
        break;
      }
    }
    block.instructions.insert(
        insertion_point,
        make_frame_machine_instruction(context,
                                       FrameInstructionKind::EpilogueTeardown,
                                       block.block_label,
                                       block.index));
  }
}

}  // namespace c4c::backend::aarch64::codegen
