#include "consumer_moves.hpp"

#include "classification.hpp"
#include "move_records.hpp"
#include "storage.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <type_traits>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

[[nodiscard]] std::optional<BlockLabelId> resolve_existing_consumer_block_label_id(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return resolve_prepared_block_label_id(names, block.label);
}

[[nodiscard]] bool prepared_register_assignments_share_identity(
    const PreparedRegallocValue& lhs,
    const PreparedRegallocValue& rhs) {
  if (assigned_storage_kind(lhs) != PreparedMoveStorageKind::Register ||
      assigned_storage_kind(rhs) != PreparedMoveStorageKind::Register ||
      !lhs.assigned_register.has_value() ||
      !rhs.assigned_register.has_value()) {
    return false;
  }
  if (lhs.assigned_register->placement.has_value() &&
      rhs.assigned_register->placement.has_value()) {
    return lhs.assigned_register->placement == rhs.assigned_register->placement;
  }
  return lhs.assigned_register->register_name == rhs.assigned_register->register_name;
}

[[nodiscard]] bool edge_move_clobbers_consumer_source(
    const PreparedMoveResolution& edge_move,
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& edge_destination) {
  return edge_move.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
         edge_move.destination_kind == PreparedMoveDestinationKind::Value &&
         edge_move.destination_storage_kind == PreparedMoveStorageKind::Register &&
         edge_move.op_kind == PreparedMoveResolutionOpKind::Move &&
         edge_move.to_value_id == edge_destination.value_id &&
         edge_destination.value_id != source.value_id &&
         prepared_register_assignments_share_identity(source, edge_destination);
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_id(
    const PreparedRegallocFunction& function,
    PreparedValueId value_id) {
  const auto it = std::find_if(function.values.begin(),
                               function.values.end(),
                               [value_id](const PreparedRegallocValue& value) {
                                 return value.value_id == value_id;
                               });
  return it == function.values.end() ? nullptr : &*it;
}

[[nodiscard]] PreparedMoveResolution make_edge_consumer_preservation_move(
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& destination,
    const PreparedMoveResolution& clobbering_edge_move) {
  return PreparedMoveResolution{
      .from_value_id = source.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = clobbering_edge_move.block_index,
      .instruction_index = clobbering_edge_move.instruction_index,
      .uses_cycle_temp_source = false,
      .coalesced_by_assigned_storage = false,
      .source_parallel_copy_step_index = std::nullopt,
      .source_immediate_i32 = std::nullopt,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .authority_kind = PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .source_parallel_copy_predecessor_label =
          clobbering_edge_move.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          clobbering_edge_move.source_parallel_copy_successor_label,
      .reason = storage_transfer_reason(
          "edge_consumer_preservation", source, destination),
      .destination_register_placement = assigned_register_placement(destination),
  };
}

[[nodiscard]] bool prepared_move_resolution_matches_edge_consumer_preservation(
    const PreparedMoveResolution& move,
    const PreparedMoveResolution& preservation) {
  return move.from_value_id == preservation.from_value_id &&
         move.to_value_id == preservation.to_value_id &&
         move.destination_kind == preservation.destination_kind &&
         move.destination_storage_kind == preservation.destination_storage_kind &&
         move.destination_abi_index == preservation.destination_abi_index &&
         move.destination_register_name == preservation.destination_register_name &&
         move.destination_contiguous_width == preservation.destination_contiguous_width &&
         move.destination_occupied_register_names ==
             preservation.destination_occupied_register_names &&
         move.destination_register_placement ==
             preservation.destination_register_placement &&
         move.destination_stack_offset_bytes ==
             preservation.destination_stack_offset_bytes &&
         move.block_index == preservation.block_index &&
         move.instruction_index == preservation.instruction_index &&
         move.uses_cycle_temp_source == preservation.uses_cycle_temp_source &&
         move.coalesced_by_assigned_storage ==
             preservation.coalesced_by_assigned_storage &&
         move.source_parallel_copy_step_index ==
             preservation.source_parallel_copy_step_index &&
         move.source_immediate_i32 == preservation.source_immediate_i32 &&
         move.op_kind == preservation.op_kind &&
         move.authority_kind == preservation.authority_kind &&
         move.source_parallel_copy_predecessor_label ==
             preservation.source_parallel_copy_predecessor_label &&
         move.source_parallel_copy_successor_label ==
             preservation.source_parallel_copy_successor_label;
}

[[nodiscard]] bool insert_edge_consumer_preservation_before_clobber(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& destination,
    std::vector<PreparedMoveResolution>::iterator clobbering_edge_move) {
  if (assigned_storage_kind(source) == PreparedMoveStorageKind::None ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None ||
      (assigned_storage_matches(source, destination) &&
       !clobbering_edge_move->coalesced_by_assigned_storage)) {
    return false;
  }

  const auto preservation =
      make_edge_consumer_preservation_move(source, destination, *clobbering_edge_move);
  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return prepared_move_resolution_matches_edge_consumer_preservation(
            move, preservation);
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return false;
  }
  regalloc_function.move_resolution.insert(clobbering_edge_move, preservation);
  return true;
}

void append_edge_consumer_preservation(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& destination,
    BlockLabelId successor_label) {
  if (successor_label == kInvalidBlockLabel) {
    return;
  }
  const std::optional<BlockLabelId> expected_successor{successor_label};
  for (std::size_t edge_move_index = 0;
       edge_move_index < regalloc_function.move_resolution.size();
       ++edge_move_index) {
    auto edge_move = regalloc_function.move_resolution.begin() +
                     static_cast<std::ptrdiff_t>(edge_move_index);
    if (edge_move->source_parallel_copy_successor_label != expected_successor ||
        !edge_move->source_parallel_copy_predecessor_label.has_value()) {
      continue;
    }
    const auto* edge_destination =
        find_regalloc_value_by_id(regalloc_function, edge_move->to_value_id);
    if (edge_destination == nullptr ||
        !edge_move_clobbers_consumer_source(*edge_move, source, *edge_destination)) {
      continue;
    }
    if (insert_edge_consumer_preservation_before_clobber(
            regalloc_function, source, destination, edge_move)) {
      ++edge_move_index;
    }
  }
}

}  // namespace

void append_consumer_move_resolution(const PreparedNameTables& names,
                                     const bir::NameTables& bir_names,
                                     const bir::Function& function,
                                     const PreparedControlFlowFunction* function_cf,
                                     PreparedRegallocFunction& regalloc_function) {
  auto is_authoritative_select_materialized_join_result =
      [&](const bir::Block& block, const bir::SelectInst& select) {
        if (function_cf == nullptr || select.result.kind != bir::Value::Kind::Named) {
          return false;
        }

        const auto block_label_id = resolve_existing_consumer_block_label_id(names, bir_names, block);
        if (!block_label_id.has_value()) {
          return false;
        }

        const auto* join_transfer =
            find_prepared_join_transfer(names, *function_cf, *block_label_id, select.result.name);
        return join_transfer != nullptr &&
               effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
                   PreparedJoinTransferCarrierKind::SelectMaterialization;
      };

  auto append_for_instruction = [&](const bir::Value& result,
                                    std::initializer_list<const bir::Value*> operands,
                                    std::optional<BlockLabelId> block_label_id,
                                    std::size_t block_index,
                                    std::size_t instruction_index) {
    if (result.kind != bir::Value::Kind::Named) {
      return;
    }

    const auto* destination = find_regalloc_value(regalloc_function, names, result.name);
    if (destination == nullptr) {
      return;
    }

    for (const bir::Value* operand : operands) {
      if (operand == nullptr || operand->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source = find_regalloc_value(regalloc_function, names, operand->name);
      if (source == nullptr) {
        continue;
      }

      if (function_cf != nullptr && block_label_id.has_value()) {
        append_edge_consumer_preservation(
            regalloc_function, *source, *destination, *block_label_id);
      }

      append_move_resolution_record(regalloc_function,
                                    *source,
                                    *destination,
                                    block_index,
                                    instruction_index,
                                    false,
                                    false,
                                    std::nullopt,
                                    PreparedMoveResolutionOpKind::Move,
                                    PreparedMoveAuthorityKind::None,
                                    storage_transfer_reason("consumer", *source, *destination));
    }
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    const auto block_label_id =
        resolve_existing_consumer_block_label_id(names, bir_names, block);
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      std::visit(
          [&](const auto& inst) {
            using Inst = std::decay_t<decltype(inst)>;
            if constexpr (std::is_same_v<Inst, bir::BinaryInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs},
                                     block_label_id,
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::SelectInst>) {
              // Select-materialized joins are already owned by published out-of-SSA
              // join-transfer authority. Re-adding them here would regress into
              // consumer-shaped reconstruction.
              if (is_authoritative_select_materialized_join_result(block, inst)) {
                return;
              }
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs, &inst.true_value, &inst.false_value},
                                     block_label_id,
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::CastInst>) {
              append_for_instruction(
                  inst.result, {&inst.operand}, block_label_id, block_index, instruction_index);
            }
          },
          block.insts[instruction_index]);
    }
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
