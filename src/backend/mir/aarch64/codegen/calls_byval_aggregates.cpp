#include "calls.hpp"
#include "memory.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move) {
  return move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
         (move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register ||
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) &&
         move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.reason == "call_arg_byval_aggregate_register_lanes";
}

[[nodiscard]] std::optional<MemoryOperand>
make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index) {
  if (size_bytes == 0) {
    return std::nullopt;
  }
  if (argument.source_selection.has_value() &&
      argument.source_selection->kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
      argument.source_selection->byval_lane_extent_bytes ==
          std::optional<std::size_t>{size_bytes} &&
      argument.source_selection->source_slot_id.has_value() &&
      argument.source_selection->source_stack_offset_bytes.has_value() &&
      argument.source_selection->source_size_bytes.has_value() &&
      argument.source_selection->source_align_bytes.has_value() &&
      *argument.source_selection->source_size_bytes >= size_bytes) {
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id.has_value()
                               ? argument.source_value_id
                               : std::optional<prepare::PreparedValueId>{
                                     source_home.value_id},
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = argument.source_selection->source_slot_id,
        .byte_offset = static_cast<std::int64_t>(
            *argument.source_selection->source_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = size_bytes,
        .align_bytes = *argument.source_selection->source_align_bytes,
        .can_use_base_plus_offset = true,
    };
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
