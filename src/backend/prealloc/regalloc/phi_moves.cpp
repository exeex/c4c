#include "phi_moves.hpp"

#include "classification.hpp"
#include "move_records.hpp"
#include "storage.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

void append_immediate_i32_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const bir::Value& source,
    const PreparedRegallocValue& destination,
    std::size_t block_index,
    std::size_t instruction_index,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> source_parallel_copy_successor_label = std::nullopt) {
  if (source.kind != bir::Value::Kind::Immediate || source.type != bir::TypeKind::I32 ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None) {
    return;
  }
  const auto destination_register_placement = assigned_register_placement(destination);

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == destination.value_id &&
               move.to_value_id == destination.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::Value &&
               move.destination_storage_kind == assigned_storage_kind(destination) &&
               !move.destination_abi_index.has_value() &&
               !move.destination_register_name.has_value() &&
               move.destination_register_placement == destination_register_placement &&
               !move.destination_stack_offset_bytes.has_value() &&
               !move.uses_cycle_temp_source && !move.coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               move.source_immediate_i32 == source.immediate &&
               move.op_kind == PreparedMoveResolutionOpKind::Move &&
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
      .from_value_id = destination.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = false,
      .coalesced_by_assigned_storage = false,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = source.immediate,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
      .destination_register_placement = destination_register_placement,
  });
}

[[nodiscard]] std::string phi_transfer_reason_prefix(const PreparedJoinTransfer& join_transfer,
                                                     bool uses_cycle_temp_source) {
  std::string prefix = join_transfer.kind == PreparedJoinTransferKind::LoopCarry
                           ? "phi_loop_carry"
                           : "phi_join";
  if (uses_cycle_temp_source) {
    prefix += "_cycle_temp";
  }
  return prefix;
}

[[nodiscard]] std::string phi_temp_save_reason(const PreparedJoinTransfer& join_transfer) {
  std::string prefix = phi_transfer_reason_prefix(join_transfer, false);
  prefix += "_save_destination_to_temp";
  return prefix;
}

}  // namespace

void append_phi_move_resolution(const PreparedNameTables& names,
                                const bir::Function& function,
                                const PreparedControlFlowFunction& function_cf,
                                PreparedRegallocFunction& regalloc_function) {
  for (const auto& bundle : function_cf.parallel_copy_bundles) {
    const auto block_index = published_prepared_parallel_copy_execution_block_index(
        names, function, bundle);
    if (!block_index.has_value()) {
      continue;
    }

    for (std::size_t step_index = 0; step_index < bundle.steps.size(); ++step_index) {
      const auto& step = bundle.steps[step_index];
      if (step.move_index >= bundle.moves.size()) {
        continue;
      }

      const auto& move = bundle.moves[step.move_index];
      if (move.join_transfer_index >= function_cf.join_transfers.size() ||
          move.destination_value.kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto& join_transfer = function_cf.join_transfers[move.join_transfer_index];

      const auto* destination =
          find_regalloc_value(regalloc_function, names, move.destination_value.name);
      if (destination == nullptr ||
          assigned_storage_kind(*destination) == PreparedMoveStorageKind::None) {
        continue;
      }

      if (step.kind == PreparedParallelCopyStepKind::SaveDestinationToTemp) {
        append_move_resolution_record(regalloc_function,
                                      *destination,
                                      *destination,
                                      *block_index,
                                      0,
                                      false,
                                      false,
                                      step_index,
                                      PreparedMoveResolutionOpKind::SaveDestinationToTemp,
                                      PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                                      phi_temp_save_reason(join_transfer),
                                      bundle.predecessor_label,
                                      bundle.successor_label);
        continue;
      }

      if (step.kind != PreparedParallelCopyStepKind::Move) {
        continue;
      }

      if (move.source_value.kind == bir::Value::Kind::Immediate &&
          move.source_value.type == bir::TypeKind::I32) {
        append_immediate_i32_move_resolution_record(
            regalloc_function,
            move.source_value,
            *destination,
            *block_index,
            0,
            step_index,
            PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            phi_transfer_reason_prefix(join_transfer, false) + "_immediate_materialization",
            bundle.predecessor_label,
            bundle.successor_label);
        continue;
      }

      if (move.source_value.kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto* source = find_regalloc_value(regalloc_function, names, move.source_value.name);
      if (source == nullptr || assigned_storage_kind(*source) == PreparedMoveStorageKind::None) {
        continue;
      }
      const bool coalesced_by_assigned_storage = assigned_storage_matches(*source, *destination);

      append_move_resolution_record(
          regalloc_function,
          *source,
          *destination,
          *block_index,
          0,
          step.uses_cycle_temp_source,
          coalesced_by_assigned_storage,
          step_index,
          PreparedMoveResolutionOpKind::Move,
          PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          storage_transfer_reason(
              phi_transfer_reason_prefix(join_transfer, step.uses_cycle_temp_source),
              *source,
              *destination),
          bundle.predecessor_label,
          bundle.successor_label);
    }
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
