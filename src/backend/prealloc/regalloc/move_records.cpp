#include "move_records.hpp"

#include "classification.hpp"
#include "storage.hpp"

#include <algorithm>
#include <utility>

namespace c4c::backend::prepare::regalloc_detail {

std::string storage_transfer_reason(std::string_view prefix,
                                    PreparedMoveStorageKind from_kind,
                                    PreparedMoveStorageKind to_kind) {
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_stack_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_register_to_stack";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_register_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_stack_to_stack";
  }
  return std::string(prefix) + "_storage_resolution";
}

std::string storage_transfer_reason(std::string_view prefix,
                                    const PreparedRegallocValue& from,
                                    const PreparedRegallocValue& to) {
  return storage_transfer_reason(prefix, assigned_storage_kind(from), assigned_storage_kind(to));
}

void append_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& destination,
    std::size_t block_index,
    std::size_t instruction_index,
    bool uses_cycle_temp_source,
    bool coalesced_by_assigned_storage,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveResolutionOpKind op_kind,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label,
    std::optional<BlockLabelId> source_parallel_copy_successor_label) {
  if (assigned_storage_kind(source) == PreparedMoveStorageKind::None ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None ||
      (op_kind == PreparedMoveResolutionOpKind::Move && !coalesced_by_assigned_storage &&
       assigned_storage_matches(source, destination))) {
    return;
  }
  const auto destination_register_placement = assigned_register_placement(destination);

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == source.value_id && move.to_value_id == destination.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::Value &&
               move.destination_storage_kind == assigned_storage_kind(destination) &&
               !move.destination_abi_index.has_value() &&
               !move.destination_register_name.has_value() &&
               move.destination_register_placement == destination_register_placement &&
               !move.destination_stack_offset_bytes.has_value() &&
               move.uses_cycle_temp_source == uses_cycle_temp_source &&
               move.coalesced_by_assigned_storage == coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               !move.source_immediate_i32.has_value() &&
               move.op_kind == op_kind &&
               move.authority_kind == authority_kind &&
               move.source_parallel_copy_predecessor_label ==
                   source_parallel_copy_predecessor_label &&
               move.source_parallel_copy_successor_label ==
                   source_parallel_copy_successor_label &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = source.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = uses_cycle_temp_source,
      .coalesced_by_assigned_storage = coalesced_by_assigned_storage,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = std::nullopt,
      .op_kind = op_kind,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
      .destination_register_placement = destination_register_placement,
  });
}

void append_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    PreparedValueId from_value_id,
    PreparedValueId to_value_id,
    PreparedMoveDestinationKind destination_kind,
    PreparedMoveStorageKind from_kind,
    PreparedMoveStorageKind to_kind,
    std::optional<std::size_t> destination_abi_index,
    std::optional<std::string> destination_register_name,
    std::size_t destination_contiguous_width,
    std::vector<std::string> destination_occupied_register_names,
    std::optional<std::size_t> destination_stack_offset_bytes,
    std::size_t block_index,
    std::size_t instruction_index,
    bool uses_cycle_temp_source,
    bool coalesced_by_assigned_storage,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveResolutionOpKind op_kind,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label,
    std::optional<BlockLabelId> source_parallel_copy_successor_label,
    std::optional<PreparedRegisterPlacement> destination_register_placement) {
  if (from_kind == PreparedMoveStorageKind::None || to_kind == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == from_value_id && move.to_value_id == to_value_id &&
               move.destination_kind == destination_kind &&
               move.destination_storage_kind == to_kind &&
               move.destination_abi_index == destination_abi_index &&
               move.destination_register_name == destination_register_name &&
               move.destination_contiguous_width == destination_contiguous_width &&
               move.destination_occupied_register_names == destination_occupied_register_names &&
               move.destination_register_placement == destination_register_placement &&
               move.destination_stack_offset_bytes == destination_stack_offset_bytes &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index &&
               move.uses_cycle_temp_source == uses_cycle_temp_source &&
               move.coalesced_by_assigned_storage == coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               !move.source_immediate_i32.has_value() &&
               move.op_kind == op_kind &&
               move.authority_kind == authority_kind &&
               move.source_parallel_copy_predecessor_label ==
                   source_parallel_copy_predecessor_label &&
               move.source_parallel_copy_successor_label ==
                   source_parallel_copy_successor_label;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = from_value_id,
      .to_value_id = to_value_id,
      .destination_kind = destination_kind,
      .destination_storage_kind = to_kind,
      .destination_abi_index = destination_abi_index,
      .destination_register_name = std::move(destination_register_name),
      .destination_contiguous_width = destination_contiguous_width,
      .destination_occupied_register_names = std::move(destination_occupied_register_names),
      .destination_stack_offset_bytes = destination_stack_offset_bytes,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = uses_cycle_temp_source,
      .coalesced_by_assigned_storage = coalesced_by_assigned_storage,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = std::nullopt,
      .op_kind = op_kind,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
      .destination_register_placement = std::move(destination_register_placement),
  });
}

}  // namespace c4c::backend::prepare::regalloc_detail
