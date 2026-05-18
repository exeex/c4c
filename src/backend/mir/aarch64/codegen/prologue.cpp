#include "prologue.hpp"

#include "instruction.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
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
  if (frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return false;
  }
  return true;
}

[[nodiscard]] bool has_non_leaf_call(const module::MachineFunction& function) {
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.instructions) {
      if (instruction.target.family == InstructionFamily::Call) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

struct FrameBoundaryFacts {
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  bool preserves_link_register = false;
  std::optional<std::size_t> link_register_save_offset_bytes;
};

[[nodiscard]] std::optional<FrameBoundaryFacts> frame_boundary_facts(
    const module::FunctionLoweringContext& context,
    const module::MachineFunction& function) {
  const bool simple_fixed_frame = has_simple_fixed_frame_plan(context);
  const bool non_leaf = has_non_leaf_call(function);
  if (!simple_fixed_frame && !non_leaf) {
    return std::nullopt;
  }
  if (context.frame_plan != nullptr &&
      (context.frame_plan->has_dynamic_stack || context.dynamic_stack_plan != nullptr)) {
    return std::nullopt;
  }

  const auto* frame = context.frame_plan;
  const std::size_t prepared_frame_alignment =
      frame != nullptr ? frame->frame_alignment_bytes : 0;
  std::size_t frame_alignment =
      non_leaf ? std::max<std::size_t>(prepared_frame_alignment, 16)
               : prepared_frame_alignment;
  std::size_t prepared_frame_size = frame != nullptr ? frame->frame_size_bytes : 0;
  if (frame != nullptr) {
    for (const auto& saved : frame->saved_callee_registers) {
      if (!saved.slot_placement.has_value() ||
          !prepare::has_complete_prepared_saved_register_slot_placement(
              *saved.slot_placement)) {
        return std::nullopt;
      }
      const auto& placement = *saved.slot_placement;
      prepared_frame_size = std::max(prepared_frame_size,
                                     *placement.stack_offset_bytes +
                                         *placement.size_bytes);
      frame_alignment = std::max(frame_alignment, *placement.align_bytes);
    }
  }
  if (non_leaf) {
    frame_alignment = std::max<std::size_t>(frame_alignment, 16);
  }

  FrameBoundaryFacts facts;
  facts.frame_alignment_bytes = frame_alignment;
  facts.frame_size_bytes = prepared_frame_size;
  if (non_leaf) {
    facts.preserves_link_register = true;
    facts.link_register_save_offset_bytes = prepared_frame_size;
    facts.frame_size_bytes = align_to(prepared_frame_size + 16, frame_alignment);
  }
  if (facts.frame_size_bytes == 0) {
    return std::nullopt;
  }
  return facts;
}

[[nodiscard]] module::MachineInstruction make_frame_machine_instruction(
    const module::FunctionLoweringContext& context,
    const FrameBoundaryFacts& facts,
    FrameInstructionKind frame_kind,
    c4c::BlockLabelId block_label,
    std::size_t block_index) {
  const auto* frame = context.frame_plan;
  InstructionRecord target = make_frame_instruction(FrameInstructionRecord{
      .frame_kind = frame_kind,
      .function_name = context.control_flow != nullptr
                           ? context.control_flow->function_name
                           : (frame != nullptr ? frame->function_name
                                               : c4c::kInvalidFunctionName),
      .frame_size_bytes = facts.frame_size_bytes,
      .frame_alignment_bytes = facts.frame_alignment_bytes,
      .preserves_link_register = facts.preserves_link_register,
      .link_register_save_offset_bytes = facts.link_register_save_offset_bytes,
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
              .function_name = context.control_flow != nullptr
                                   ? context.control_flow->function_name
                                   : (frame != nullptr ? frame->function_name
                                                       : c4c::kInvalidFunctionName),
              .block_label = block_label,
          },
  };
}

}  // namespace

void insert_prepared_frame_boundary_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function) {
  const auto facts = frame_boundary_facts(context, function);
  if (!facts.has_value() || function.blocks.empty()) {
    return;
  }

  auto& entry = function.blocks.front();
  entry.instructions.insert(
      entry.instructions.begin(),
      make_frame_machine_instruction(context,
                                     *facts,
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
                                       *facts,
                                       FrameInstructionKind::EpilogueTeardown,
                                       block.block_label,
                                       block.index));
  }
}

}  // namespace c4c::backend::aarch64::codegen
