#include "prepared_object_traversal.hpp"

#include <algorithm>
#include <utility>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] std::optional<std::size_t> prepared_block_index_by_label(
    const PreparedControlFlowFunction& control_flow,
    BlockLabelId label) {
  if (label == kInvalidBlockLabel) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < control_flow.blocks.size(); ++index) {
    if (control_flow.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool prepared_join_transfer_matches_select(
    const PreparedJoinTransfer& join_transfer,
    BlockLabelId block_label,
    const bir::SelectInst& select) {
  return block_label != kInvalidBlockLabel &&
         join_transfer.join_block_label == block_label &&
         join_transfer.result.kind == bir::Value::Kind::Named &&
         select.result.kind == bir::Value::Kind::Named &&
         join_transfer.result.name == select.result.name;
}

[[nodiscard]] bool prepared_select_materialization_carrier_is_complete(
    const PreparedJoinTransfer& join_transfer) {
  if (!join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return false;
  }

  const auto true_index = *join_transfer.source_true_transfer_index;
  const auto false_index = *join_transfer.source_false_transfer_index;
  if (true_index >= join_transfer.edge_transfers.size() ||
      false_index >= join_transfer.edge_transfers.size() ||
      true_index == false_index) {
    return false;
  }

  const auto& true_transfer = join_transfer.edge_transfers[true_index];
  const auto& false_transfer = join_transfer.edge_transfers[false_index];
  return true_transfer.successor_label == join_transfer.join_block_label &&
         false_transfer.successor_label == join_transfer.join_block_label;
}

[[nodiscard]] bool prepared_object_value_home_is_complete(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return home.register_name.has_value();
    case PreparedValueHomeKind::StackSlot:
      return home.offset_bytes.has_value();
    case PreparedValueHomeKind::RematerializableImmediate:
      return home.immediate_i32.has_value() || home.immediate_f128.has_value();
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return (home.pointer_base_value_name.has_value() ||
              home.pointer_base_symbol_name.has_value()) &&
             home.pointer_byte_delta.has_value();
    case PreparedValueHomeKind::None:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_object_value_home_kind_is_supported(
    PreparedValueHomeKind kind) {
  switch (kind) {
    case PreparedValueHomeKind::Register:
    case PreparedValueHomeKind::StackSlot:
    case PreparedValueHomeKind::RematerializableImmediate:
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return true;
    case PreparedValueHomeKind::None:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_object_stack_slot_home_is_complete(
    const PreparedValueHome& home) {
  return home.slot_id.has_value() && home.offset_bytes.has_value() &&
         home.size_bytes.has_value() && *home.size_bytes > 0 &&
         home.align_bytes.has_value() && *home.align_bytes > 0;
}

[[nodiscard]] const PreparedFrameSlot* find_unique_frame_slot_owner(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id,
    bool& ambiguous) {
  const PreparedFrameSlot* match = nullptr;
  ambiguous = false;
  for (const auto& frame_slot : stack_layout.frame_slots) {
    if (frame_slot.slot_id != slot_id) {
      continue;
    }
    if (match != nullptr) {
      ambiguous = true;
      return nullptr;
    }
    match = &frame_slot;
  }
  return match;
}

[[nodiscard]] bool prepared_object_regalloc_value_id_agrees(
    const PreparedRegallocFunction* regalloc,
    ValueNameId value_name,
    PreparedValueId value_id) {
  if (regalloc == nullptr) {
    return true;
  }

  const PreparedRegallocValue* match = nullptr;
  for (const auto& value : regalloc->values) {
    if (value.value_name != value_name) {
      continue;
    }
    if (match != nullptr) {
      return false;
    }
    match = &value;
  }
  return match == nullptr || match->value_id == value_id;
}

[[nodiscard]] bool prepared_object_value_home_lookup_agrees(
    const PreparedValueHomeLookups* value_home_lookups,
    const PreparedValueHome& home) {
  if (value_home_lookups == nullptr) {
    return true;
  }

  const auto value_id_it = value_home_lookups->value_ids.find(home.value_name);
  if (value_id_it == value_home_lookups->value_ids.end() ||
      value_id_it->second != home.value_id) {
    return false;
  }

  const auto home_it = value_home_lookups->homes_by_id.find(home.value_id);
  return home_it != value_home_lookups->homes_by_id.end() &&
         home_it->second == &home;
}

[[nodiscard]] bool prepared_object_event_kind_can_consume_move_bundle(
    PreparedObjectTraversalEventKind kind) {
  switch (kind) {
    case PreparedObjectTraversalEventKind::BlockEntryCopies:
    case PreparedObjectTraversalEventKind::BeforeInstructionCopies:
    case PreparedObjectTraversalEventKind::PreTerminatorCopies:
      return true;
    case PreparedObjectTraversalEventKind::Label:
    case PreparedObjectTraversalEventKind::Instruction:
    case PreparedObjectTraversalEventKind::Terminator:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_object_non_parallel_copy_move_phase_matches_event(
    const PreparedObjectTraversalEvent& event,
    const PreparedMoveBundle& move_bundle) {
  switch (event.kind) {
    case PreparedObjectTraversalEventKind::BlockEntryCopies:
      return move_bundle.phase == PreparedMovePhase::BlockEntry &&
             move_bundle.instruction_index == 0;
    case PreparedObjectTraversalEventKind::BeforeInstructionCopies:
      return move_bundle.phase == PreparedMovePhase::BeforeInstruction &&
             move_bundle.instruction_index == event.instruction_index;
    case PreparedObjectTraversalEventKind::PreTerminatorCopies:
      return move_bundle.phase == PreparedMovePhase::BeforeReturn &&
             move_bundle.instruction_index == event.instruction_index;
    case PreparedObjectTraversalEventKind::Label:
    case PreparedObjectTraversalEventKind::Instruction:
    case PreparedObjectTraversalEventKind::Terminator:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_move_bundle_matches_parallel_copy(
    const PreparedMoveBundle& move_bundle,
    const PreparedParallelCopyBundle& parallel_copy_bundle,
    std::size_t execution_block_index) {
  return move_bundle.phase == PreparedMovePhase::BlockEntry &&
         move_bundle.authority_kind ==
             PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
         move_bundle.block_index == execution_block_index &&
         move_bundle.source_parallel_copy_predecessor_label ==
             parallel_copy_bundle.predecessor_label &&
         move_bundle.source_parallel_copy_successor_label ==
             parallel_copy_bundle.successor_label;
}

[[nodiscard]] const PreparedValueHome* prepared_object_value_home_for_id(
    const PreparedValueHomeLookups* value_home_lookups,
    PreparedValueId value_id) {
  if (value_home_lookups == nullptr) {
    return nullptr;
  }
  const auto it = value_home_lookups->homes_by_id.find(value_id);
  return it == value_home_lookups->homes_by_id.end() ? nullptr : it->second;
}

[[nodiscard]] bool prepared_stack_homes_same_destination(
    const PreparedValueHome& lhs,
    const PreparedValueHome& rhs) {
  if (lhs.kind != PreparedValueHomeKind::StackSlot ||
      rhs.kind != PreparedValueHomeKind::StackSlot) {
    return false;
  }
  if (lhs.slot_id.has_value() && rhs.slot_id.has_value() &&
      lhs.slot_id == rhs.slot_id) {
    return true;
  }
  return lhs.offset_bytes.has_value() && rhs.offset_bytes.has_value() &&
         lhs.offset_bytes == rhs.offset_bytes;
}

[[nodiscard]] bool
prepared_move_bundle_has_ambiguous_multi_source_stack_destination(
    const PreparedMoveBundle& move_bundle,
    const PreparedValueHomeLookups* value_home_lookups) {
  if (move_bundle.authority_kind != PreparedMoveAuthorityKind::None ||
      move_bundle.phase != PreparedMovePhase::BeforeInstruction ||
      move_bundle.moves.size() < 2) {
    return false;
  }

  for (std::size_t lhs_index = 0; lhs_index < move_bundle.moves.size();
       ++lhs_index) {
    const auto& lhs = move_bundle.moves[lhs_index];
    if (lhs.authority_kind != PreparedMoveAuthorityKind::None ||
        lhs.source_parallel_copy_step_index.has_value() ||
        lhs.destination_kind != PreparedMoveDestinationKind::Value) {
      continue;
    }
    const auto* lhs_source_home =
        prepared_object_value_home_for_id(value_home_lookups, lhs.from_value_id);
    const auto* lhs_destination_home =
        prepared_object_value_home_for_id(value_home_lookups, lhs.to_value_id);
    if (lhs_source_home == nullptr || lhs_destination_home == nullptr ||
        lhs_source_home->kind != PreparedValueHomeKind::Register ||
        lhs_destination_home->kind != PreparedValueHomeKind::StackSlot) {
      continue;
    }

    for (std::size_t rhs_index = lhs_index + 1;
         rhs_index < move_bundle.moves.size();
         ++rhs_index) {
      const auto& rhs = move_bundle.moves[rhs_index];
      if (rhs.authority_kind != PreparedMoveAuthorityKind::None ||
          rhs.source_parallel_copy_step_index.has_value() ||
          rhs.destination_kind != PreparedMoveDestinationKind::Value) {
        continue;
      }
      const auto* rhs_source_home = prepared_object_value_home_for_id(
          value_home_lookups, rhs.from_value_id);
      const auto* rhs_destination_home = prepared_object_value_home_for_id(
          value_home_lookups, rhs.to_value_id);
      if (rhs_source_home == nullptr || rhs_destination_home == nullptr ||
          rhs_source_home->kind != PreparedValueHomeKind::Register ||
          rhs_destination_home->kind != PreparedValueHomeKind::StackSlot) {
        continue;
      }
      if (lhs.to_value_id == rhs.to_value_id ||
          prepared_stack_homes_same_destination(*lhs_destination_home,
                                               *rhs_destination_home)) {
        return true;
      }
    }
  }

  return false;
}

[[nodiscard]] const PreparedMoveBundle* find_parallel_copy_move_bundle(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const PreparedParallelCopyBundle& parallel_copy_bundle) {
  if (value_locations == nullptr ||
      !parallel_copy_bundle.execution_block_label.has_value()) {
    return nullptr;
  }
  const auto execution_block_index =
      prepared_block_index_by_label(control_flow,
                                    *parallel_copy_bundle.execution_block_label);
  if (!execution_block_index.has_value()) {
    return nullptr;
  }

  const PreparedMoveBundle* match = nullptr;
  for (const auto& move_bundle : value_locations->move_bundles) {
    if (!prepared_move_bundle_matches_parallel_copy(
            move_bundle, parallel_copy_bundle, *execution_block_index)) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &move_bundle;
  }
  return match;
}

[[nodiscard]] bool move_bundle_has_parallel_copy_owner(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const PreparedMoveBundle& move_bundle) {
  for (const auto& parallel_copy_bundle : control_flow.parallel_copy_bundles) {
    const auto execution_block_index =
        parallel_copy_bundle.execution_block_label.has_value()
            ? prepared_block_index_by_label(
                  control_flow, *parallel_copy_bundle.execution_block_label)
            : std::nullopt;
    if (execution_block_index.has_value() &&
        prepared_move_bundle_matches_parallel_copy(
            move_bundle, parallel_copy_bundle, *execution_block_index) &&
        find_parallel_copy_move_bundle(control_flow,
                                       value_locations,
                                       parallel_copy_bundle) == &move_bundle) {
      return true;
    }
  }
  return false;
}

void append_parallel_copy_events_for_block(
    std::vector<PreparedObjectTraversalEvent>& events,
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const PreparedControlFlowBlock& block,
    std::size_t block_index,
    PreparedObjectTraversalEventKind event_kind,
    const bir::Block* bir_block) {
  for (const auto& parallel_copy_bundle : control_flow.parallel_copy_bundles) {
    const auto kind = prepared_object_parallel_copy_event_kind(parallel_copy_bundle);
    if (!kind.has_value() || *kind != event_kind ||
        parallel_copy_bundle.execution_block_label != block.block_label) {
      continue;
    }
    events.push_back(PreparedObjectTraversalEvent{
        .kind = event_kind,
        .block_index = block_index,
        .prepared_block = &block,
        .bir_block = bir_block,
        .move_bundle = find_parallel_copy_move_bundle(
            control_flow, value_locations, parallel_copy_bundle),
        .parallel_copy_bundle = &parallel_copy_bundle,
    });
  }
}

void append_unowned_block_entry_move_events(
    std::vector<PreparedObjectTraversalEvent>& events,
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const PreparedControlFlowBlock& block,
    std::size_t block_index,
    const bir::Block* bir_block) {
  if (value_locations == nullptr) {
    return;
  }
  for (const auto& move_bundle : value_locations->move_bundles) {
    if (move_bundle.phase != PreparedMovePhase::BlockEntry ||
        move_bundle.block_index != block_index ||
        move_bundle_has_parallel_copy_owner(control_flow,
                                            value_locations,
                                            move_bundle)) {
      continue;
    }
    events.push_back(PreparedObjectTraversalEvent{
        .kind = PreparedObjectTraversalEventKind::BlockEntryCopies,
        .block_index = block_index,
        .prepared_block = &block,
        .bir_block = bir_block,
        .move_bundle = &move_bundle,
    });
  }
}

[[nodiscard]] bool move_bundle_has_stack_destination(
    const PreparedMoveBundle& move_bundle) {
  return std::any_of(move_bundle.moves.begin(),
                     move_bundle.moves.end(),
                     [](const PreparedMoveResolution& move) {
                       return move.destination_storage_kind ==
                             PreparedMoveStorageKind::StackSlot;
                     });
}

[[nodiscard]] bool move_bundle_has_register_destination(
    const PreparedMoveBundle& move_bundle) {
  return std::any_of(move_bundle.moves.begin(),
                     move_bundle.moves.end(),
                     [](const PreparedMoveResolution& move) {
                       return move.destination_storage_kind ==
                              PreparedMoveStorageKind::Register;
                     });
}

void append_before_instruction_move_events(
    std::vector<PreparedObjectTraversalEvent>& events,
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const PreparedControlFlowBlock& block,
    std::size_t block_index,
    std::size_t instruction_index,
    const bir::Block* bir_block,
    const PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements) {
  if (value_locations == nullptr) {
    return;
  }
  for (const auto& move_bundle : value_locations->move_bundles) {
    if (move_bundle.phase != PreparedMovePhase::BeforeInstruction ||
        move_bundle.block_index != block_index ||
        move_bundle.instruction_index != instruction_index) {
      continue;
    }
    const bool has_authorized_register_suppression =
        move_bundle_has_register_destination(move_bundle) &&
        select_edge_source_producer_placements != nullptr &&
        std::any_of(
            select_edge_source_producer_placements->records.begin(),
            select_edge_source_producer_placements->records.end(),
            [&](const PreparedSelectEdgeSourceProducerPlacementRecord& record) {
              return prepared_select_edge_source_producer_placement_matches_move_bundle(
                  record, control_flow.function_name, block.block_label, move_bundle);
            });
    if (!move_bundle_has_stack_destination(move_bundle) &&
        !has_authorized_register_suppression) {
      continue;
    }
    events.push_back(PreparedObjectTraversalEvent{
        .kind = PreparedObjectTraversalEventKind::BeforeInstructionCopies,
        .block_index = block_index,
        .instruction_index = instruction_index,
        .prepared_block = &block,
        .bir_block = bir_block,
        .move_bundle = &move_bundle,
    });
  }
}

void append_before_return_move_events(std::vector<PreparedObjectTraversalEvent>& events,
                                      const PreparedValueLocationFunction* value_locations,
                                      const PreparedControlFlowBlock& block,
                                      std::size_t block_index,
                                      const bir::Block* bir_block) {
  if (value_locations == nullptr) {
    return;
  }
  for (const auto& move_bundle : value_locations->move_bundles) {
    if (move_bundle.phase != PreparedMovePhase::BeforeReturn ||
        move_bundle.block_index != block_index) {
      continue;
    }
    events.push_back(PreparedObjectTraversalEvent{
        .kind = PreparedObjectTraversalEventKind::PreTerminatorCopies,
        .block_index = block_index,
        .instruction_index = move_bundle.instruction_index,
        .prepared_block = &block,
        .bir_block = bir_block,
        .move_bundle = &move_bundle,
    });
  }
}

[[nodiscard]] PreparedObjectConsumerDiagnostic make_consumer_diagnostic(
    PreparedObjectConsumerDiagnosticCategory category,
    std::string message) {
  return PreparedObjectConsumerDiagnostic{
      .category = category,
      .message = std::move(message),
  };
}

}  // namespace

std::optional<PreparedObjectTraversalEventKind> prepared_object_parallel_copy_event_kind(
    const PreparedParallelCopyBundle& parallel_copy_bundle) {
  switch (parallel_copy_bundle.execution_site) {
    case PreparedParallelCopyExecutionSite::SuccessorEntry:
      return PreparedObjectTraversalEventKind::BlockEntryCopies;
    case PreparedParallelCopyExecutionSite::PredecessorTerminator:
      return PreparedObjectTraversalEventKind::PreTerminatorCopies;
    case PreparedParallelCopyExecutionSite::CriticalEdge:
      return std::nullopt;
  }
  return std::nullopt;
}

PreparedObjectSelectConsumerClassification classify_prepared_object_select_consumer(
    const PreparedObjectSelectConsumerQuery& query) {
  if (query.instruction == nullptr) {
    return PreparedObjectSelectConsumerClassification{};
  }

  const auto* select = std::get_if<bir::SelectInst>(query.instruction);
  if (select == nullptr) {
    return PreparedObjectSelectConsumerClassification{};
  }

  PreparedObjectSelectConsumerClassification result{
      .kind = PreparedObjectSelectConsumerKind::OrdinarySelect,
      .select = select,
  };

  if (select->result.kind != bir::Value::Kind::Named) {
    result.kind = PreparedObjectSelectConsumerKind::UnsupportedSelectResult;
    return result;
  }

  if (query.control_flow == nullptr || query.block_label == kInvalidBlockLabel) {
    result.kind = query.require_prepared_join_transfer
                      ? PreparedObjectSelectConsumerKind::MissingPreparedJoinTransfer
                      : PreparedObjectSelectConsumerKind::OrdinarySelect;
    return result;
  }

  const PreparedJoinTransfer* match = nullptr;
  for (const auto& join_transfer : query.control_flow->join_transfers) {
    if (!prepared_join_transfer_matches_select(
            join_transfer, query.block_label, *select)) {
      continue;
    }
    if (match != nullptr) {
      result.kind = PreparedObjectSelectConsumerKind::AmbiguousPreparedJoinTransfer;
      result.join_transfer = nullptr;
      return result;
    }
    match = &join_transfer;
  }

  if (match == nullptr) {
    result.kind = query.require_prepared_join_transfer
                      ? PreparedObjectSelectConsumerKind::MissingPreparedJoinTransfer
                      : PreparedObjectSelectConsumerKind::OrdinarySelect;
    return result;
  }

  result.join_transfer = match;
  result.carrier_kind = effective_prepared_join_transfer_carrier_kind(*match);

  if (match->kind != PreparedJoinTransferKind::PhiEdge) {
    result.kind =
        PreparedObjectSelectConsumerKind::UnsupportedPreparedJoinTransferKind;
    return result;
  }

  if (result.carrier_kind !=
      PreparedJoinTransferCarrierKind::SelectMaterialization) {
    result.kind = PreparedObjectSelectConsumerKind::
        UnsupportedPreparedJoinTransferCarrierKind;
    return result;
  }

  if (!prepared_select_materialization_carrier_is_complete(*match)) {
    result.kind =
        PreparedObjectSelectConsumerKind::MalformedPreparedJoinTransferCarrier;
    return result;
  }

  result.kind = PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier;
  return result;
}

PreparedObjectSelectConsumerClassification classify_prepared_object_select_consumer(
    const PreparedControlFlowFunction* control_flow,
    BlockLabelId block_label,
    const bir::Inst& instruction,
    bool require_prepared_join_transfer) {
  return classify_prepared_object_select_consumer(
      PreparedObjectSelectConsumerQuery{
          .control_flow = control_flow,
          .block_label = block_label,
          .instruction = &instruction,
          .require_prepared_join_transfer = require_prepared_join_transfer,
      });
}

PreparedObjectValueHomeConsumerClassification
classify_prepared_object_value_home_consumer(
    const PreparedObjectValueHomeConsumerQuery& query) {
  PreparedObjectValueHomeConsumerClassification result;
  if (query.value == nullptr) {
    return result;
  }

  if (query.value->kind != bir::Value::Kind::Named || query.value->name.empty()) {
    result.status = PreparedObjectValueHomeConsumerStatus::NonNamedValue;
    return result;
  }

  if (query.names == nullptr) {
    result.status = PreparedObjectValueHomeConsumerStatus::MissingNames;
    return result;
  }

  result.value_name = query.names->value_names.find(query.value->name);
  if (result.value_name == kInvalidValueName) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::MissingPreparedValueName;
    return result;
  }

  if (query.value_locations == nullptr) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::MissingValueLocations;
    return result;
  }

  const PreparedValueHome* match = nullptr;
  for (const auto& home : query.value_locations->value_homes) {
    if (home.value_name != result.value_name) {
      continue;
    }
    if (match != nullptr) {
      result.status =
          PreparedObjectValueHomeConsumerStatus::AmbiguousPreparedValueHome;
      return result;
    }
    match = &home;
  }
  if (match == nullptr) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::MissingPreparedValueHome;
    return result;
  }

  for (const auto& home : query.value_locations->value_homes) {
    if (home.value_id == match->value_id && &home != match) {
      result.status =
          PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueId;
      return result;
    }
  }

  result.home = match;
  result.value_id = match->value_id;
  result.home_kind = match->kind;

  if (!prepared_object_regalloc_value_id_agrees(
          query.regalloc, result.value_name, result.value_id)) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueId;
    return result;
  }

  if (!prepared_object_value_home_lookup_agrees(query.value_home_lookups, *match)) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueHomeLookup;
    return result;
  }

  if (!prepared_object_value_home_kind_is_supported(match->kind)) {
    result.status =
        PreparedObjectValueHomeConsumerStatus::UnsupportedValueHomeKind;
    return result;
  }

  if (!prepared_object_value_home_is_complete(*match)) {
    result.status = PreparedObjectValueHomeConsumerStatus::IncompleteValueHome;
    return result;
  }

  result.status = PreparedObjectValueHomeConsumerStatus::Available;
  return result;
}

PreparedObjectValueHomeConsumerClassification
classify_prepared_object_value_home_consumer(
    const PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& value,
    const PreparedRegallocFunction* regalloc,
    const PreparedValueHomeLookups* value_home_lookups) {
  return classify_prepared_object_value_home_consumer(
      PreparedObjectValueHomeConsumerQuery{
          .names = &names,
          .regalloc = regalloc,
          .value_locations = value_locations,
          .value_home_lookups = value_home_lookups,
          .value = &value,
      });
}

PreparedObjectMoveBundleConsumerClassification
classify_prepared_object_move_bundle_consumer(
    const PreparedObjectMoveBundleConsumerQuery& query) {
  PreparedObjectMoveBundleConsumerClassification result;
  if (query.event == nullptr) {
    return result;
  }

  const auto& event = *query.event;
  result.event = &event;
  result.event_kind = event.kind;
  result.block_index = event.block_index;
  result.instruction_index = event.instruction_index;

  if (!prepared_object_event_kind_can_consume_move_bundle(event.kind)) {
    result.status =
        PreparedObjectMoveBundleConsumerStatus::UnsupportedEventKind;
    return result;
  }

  if (event.move_bundle == nullptr) {
    result.status = PreparedObjectMoveBundleConsumerStatus::MissingMoveBundle;
    return result;
  }

  const auto& move_bundle = *event.move_bundle;
  result.move_bundle = &move_bundle;
  result.parallel_copy_bundle = event.parallel_copy_bundle;
  result.phase = move_bundle.phase;
  result.move_count = move_bundle.moves.size();

  if (move_bundle.moves.empty()) {
    result.status = PreparedObjectMoveBundleConsumerStatus::EmptyMoveBundle;
    return result;
  }

  if (event.parallel_copy_bundle != nullptr) {
    if (move_bundle.authority_kind !=
        PreparedMoveAuthorityKind::OutOfSsaParallelCopy) {
      result.status = PreparedObjectMoveBundleConsumerStatus::
          UnsupportedParallelCopyMoveBundleAuthority;
      return result;
    }

    const auto expected_kind =
        prepared_object_parallel_copy_event_kind(*event.parallel_copy_bundle);
    if (!expected_kind.has_value()) {
      result.status = PreparedObjectMoveBundleConsumerStatus::
          UnsupportedParallelCopyExecutionSite;
      return result;
    }
    if (*expected_kind != event.kind) {
      result.status = PreparedObjectMoveBundleConsumerStatus::
          MismatchedParallelCopyExecutionSite;
      return result;
    }
    if (!prepared_move_bundle_matches_parallel_copy(
            move_bundle, *event.parallel_copy_bundle, event.block_index)) {
      result.status = PreparedObjectMoveBundleConsumerStatus::
          MismatchedParallelCopyMoveBundle;
      return result;
    }

    result.status = PreparedObjectMoveBundleConsumerStatus::Available;
    return result;
  }

  if (move_bundle.authority_kind ==
      PreparedMoveAuthorityKind::OutOfSsaParallelCopy) {
    result.status =
        PreparedObjectMoveBundleConsumerStatus::MissingParallelCopyBundle;
    return result;
  }

  if (move_bundle.block_index != event.block_index) {
    result.status =
        PreparedObjectMoveBundleConsumerStatus::MismatchedMoveBundleBlock;
    return result;
  }

  if (!prepared_object_non_parallel_copy_move_phase_matches_event(event,
                                                                 move_bundle)) {
    result.status =
        PreparedObjectMoveBundleConsumerStatus::MismatchedMoveBundlePhase;
    return result;
  }

  if (event.kind == PreparedObjectTraversalEventKind::BeforeInstructionCopies &&
      prepared_move_bundle_has_ambiguous_multi_source_stack_destination(
          move_bundle, query.value_home_lookups)) {
    result.status = PreparedObjectMoveBundleConsumerStatus::
        AmbiguousNonParallelMultiSourceStackDestination;
    return result;
  }

  result.status = PreparedObjectMoveBundleConsumerStatus::Available;
  return result;
}

PreparedObjectMoveBundleConsumerClassification
classify_prepared_object_move_bundle_consumer(
    const PreparedObjectTraversalEvent& event) {
  return classify_prepared_object_move_bundle_consumer(
      PreparedObjectMoveBundleConsumerQuery{.event = &event});
}

PreparedObjectFrameSlotConsumerClassification
classify_prepared_object_frame_slot_consumer(
    const PreparedObjectFrameSlotConsumerQuery& query) {
  PreparedObjectFrameSlotConsumerClassification result;
  if (query.value_home == nullptr) {
    return result;
  }

  const auto& home = *query.value_home;
  result.value_home = &home;

  if (home.kind != PreparedValueHomeKind::StackSlot) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::UnsupportedValueHomeKind;
    return result;
  }

  if (!prepared_object_stack_slot_home_is_complete(home)) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::IncompleteStackSlotHome;
    return result;
  }

  result.slot_id = *home.slot_id;
  result.offset_bytes = *home.offset_bytes;
  result.size_bytes = *home.size_bytes;
  result.align_bytes = *home.align_bytes;

  if (query.stack_layout == nullptr) {
    result.status = PreparedObjectFrameSlotConsumerStatus::MissingStackLayout;
    return result;
  }

  bool ambiguous = false;
  const auto* frame_slot =
      find_unique_frame_slot_owner(*query.stack_layout, result.slot_id, ambiguous);
  if (ambiguous) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::AmbiguousFrameSlotOwner;
    return result;
  }
  if (frame_slot == nullptr) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::MissingFrameSlotOwner;
    return result;
  }
  result.frame_slot = frame_slot;

  if (frame_slot->function_name != home.function_name) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotFunction;
    return result;
  }
  if (frame_slot->offset_bytes != result.offset_bytes) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotOffset;
    return result;
  }
  if (frame_slot->size_bytes != result.size_bytes) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotSize;
    return result;
  }
  if (frame_slot->align_bytes != result.align_bytes) {
    result.status =
        PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotAlignment;
    return result;
  }

  result.status = PreparedObjectFrameSlotConsumerStatus::Available;
  return result;
}

PreparedObjectFrameSlotConsumerClassification
classify_prepared_object_frame_slot_consumer(
    const PreparedStackLayout& stack_layout,
    const PreparedValueHome& value_home) {
  return classify_prepared_object_frame_slot_consumer(
      PreparedObjectFrameSlotConsumerQuery{
          .stack_layout = &stack_layout,
          .value_home = &value_home,
      });
}

std::optional<PreparedObjectConsumerDiagnostic> diagnose_prepared_object_consumer(
    const PreparedObjectSelectConsumerClassification& classification) {
  switch (classification.kind) {
    case PreparedObjectSelectConsumerKind::NotSelectInstruction:
    case PreparedObjectSelectConsumerKind::OrdinarySelect:
    case PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier:
      return std::nullopt;
    case PreparedObjectSelectConsumerKind::MissingPreparedJoinTransfer:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingPreparedJoinTransfer,
          "prepared select carrier is missing required join-transfer authority");
    case PreparedObjectSelectConsumerKind::AmbiguousPreparedJoinTransfer:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::AmbiguousPreparedJoinTransfer,
          "prepared select carrier matched multiple join-transfer authorities");
    case PreparedObjectSelectConsumerKind::UnsupportedSelectResult:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::UnsupportedSelectResult,
          "prepared select carrier requires a named select result");
    case PreparedObjectSelectConsumerKind::UnsupportedPreparedJoinTransferKind:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedPreparedJoinTransferKind,
          "prepared select carrier requires phi-edge join-transfer authority");
    case PreparedObjectSelectConsumerKind::
        UnsupportedPreparedJoinTransferCarrierKind:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedPreparedJoinTransferCarrierKind,
          "prepared select carrier uses an unsupported join-transfer carrier");
    case PreparedObjectSelectConsumerKind::MalformedPreparedJoinTransferCarrier:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              MalformedPreparedJoinTransferCarrier,
          "prepared select carrier is missing complete materialization edge facts");
  }
  return std::nullopt;
}

std::optional<PreparedObjectConsumerDiagnostic> diagnose_prepared_object_consumer(
    const PreparedObjectValueHomeConsumerClassification& classification) {
  switch (classification.status) {
    case PreparedObjectValueHomeConsumerStatus::Available:
      return std::nullopt;
    case PreparedObjectValueHomeConsumerStatus::MissingValue:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingValue,
          "prepared value-home consumer query is missing a value operand");
    case PreparedObjectValueHomeConsumerStatus::NonNamedValue:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::NonNamedValue,
          "prepared value-home consumer requires a named value operand");
    case PreparedObjectValueHomeConsumerStatus::MissingNames:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingNames,
          "prepared value-home consumer is missing prepared name tables");
    case PreparedObjectValueHomeConsumerStatus::MissingValueLocations:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingValueLocations,
          "prepared value-home consumer is missing value-location authority");
    case PreparedObjectValueHomeConsumerStatus::MissingPreparedValueName:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingPreparedValueName,
          "prepared value-home consumer cannot find the prepared value name");
    case PreparedObjectValueHomeConsumerStatus::MissingPreparedValueHome:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingPreparedValueHome,
          "prepared value-home consumer is missing prepared value-home authority");
    case PreparedObjectValueHomeConsumerStatus::AmbiguousPreparedValueHome:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::AmbiguousPreparedValueHome,
          "prepared value-home consumer found multiple prepared homes for one value");
    case PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueId:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::ConflictingPreparedValueId,
          "prepared value-home consumer found conflicting prepared value ids");
    case PreparedObjectValueHomeConsumerStatus::
        ConflictingPreparedValueHomeLookup:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              ConflictingPreparedValueHomeLookup,
          "prepared value-home lookup disagrees with value-home authority");
    case PreparedObjectValueHomeConsumerStatus::UnsupportedValueHomeKind:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::UnsupportedValueHomeKind,
          "prepared value-home consumer found an unsupported value-home kind");
    case PreparedObjectValueHomeConsumerStatus::IncompleteValueHome:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::IncompleteValueHome,
          "prepared value-home consumer found incomplete value-home facts");
  }
  return std::nullopt;
}

std::optional<PreparedObjectConsumerDiagnostic> diagnose_prepared_object_consumer(
    const PreparedObjectMoveBundleConsumerClassification& classification) {
  switch (classification.status) {
    case PreparedObjectMoveBundleConsumerStatus::Available:
      return std::nullopt;
    case PreparedObjectMoveBundleConsumerStatus::MissingEvent:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingEvent,
          "prepared move-bundle consumer query is missing a traversal event");
    case PreparedObjectMoveBundleConsumerStatus::UnsupportedEventKind:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::UnsupportedEventKind,
          "prepared move-bundle consumer requires a copy traversal event");
    case PreparedObjectMoveBundleConsumerStatus::MissingMoveBundle:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingMoveBundle,
          "prepared copy traversal event is missing move-bundle authority");
    case PreparedObjectMoveBundleConsumerStatus::EmptyMoveBundle:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::EmptyMoveBundle,
          "prepared move-bundle authority contains no moves");
    case PreparedObjectMoveBundleConsumerStatus::MismatchedMoveBundlePhase:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedMoveBundlePhase,
          "prepared move-bundle phase does not match its traversal event");
    case PreparedObjectMoveBundleConsumerStatus::MismatchedMoveBundleBlock:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedMoveBundleBlock,
          "prepared move-bundle block does not match its traversal event");
    case PreparedObjectMoveBundleConsumerStatus::MissingParallelCopyBundle:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingParallelCopyBundle,
          "prepared out-of-SSA move bundle is missing parallel-copy authority");
    case PreparedObjectMoveBundleConsumerStatus::
        UnsupportedParallelCopyExecutionSite:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedParallelCopyExecutionSite,
          "prepared parallel-copy authority has no target-consumable block event");
    case PreparedObjectMoveBundleConsumerStatus::
        MismatchedParallelCopyExecutionSite:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              MismatchedParallelCopyExecutionSite,
          "prepared parallel-copy authority is attached to the wrong traversal site");
    case PreparedObjectMoveBundleConsumerStatus::
        MismatchedParallelCopyMoveBundle:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              MismatchedParallelCopyMoveBundle,
          "prepared move-bundle authority does not match its parallel-copy owner");
    case PreparedObjectMoveBundleConsumerStatus::
        UnsupportedParallelCopyMoveBundleAuthority:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedParallelCopyMoveBundleAuthority,
          "prepared parallel-copy event requires out-of-SSA move-bundle authority");
    case PreparedObjectMoveBundleConsumerStatus::
        AmbiguousNonParallelMultiSourceStackDestination:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              AmbiguousNonParallelMultiSourceStackDestination,
          "prepared move-bundle classifier rejected ambiguous non-parallel "
          "multi-source stack-destination authority");
  }
  return std::nullopt;
}

std::optional<PreparedObjectConsumerDiagnostic> diagnose_prepared_object_consumer(
    const PreparedObjectFrameSlotConsumerClassification& classification) {
  switch (classification.status) {
    case PreparedObjectFrameSlotConsumerStatus::Available:
      return std::nullopt;
    case PreparedObjectFrameSlotConsumerStatus::MissingValueHome:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingFrameSlotValueHome,
          "prepared frame-slot consumer query is missing a value-home authority");
    case PreparedObjectFrameSlotConsumerStatus::UnsupportedValueHomeKind:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedFrameSlotValueHomeKind,
          "prepared frame-slot consumer requires a stack-slot value home");
    case PreparedObjectFrameSlotConsumerStatus::IncompleteStackSlotHome:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::IncompleteStackSlotHome,
          "prepared stack-slot home is missing complete frame-slot facts");
    case PreparedObjectFrameSlotConsumerStatus::MissingStackLayout:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingStackLayout,
          "prepared frame-slot consumer is missing stack-layout authority");
    case PreparedObjectFrameSlotConsumerStatus::MissingFrameSlotOwner:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MissingFrameSlotOwner,
          "prepared stack-slot home is missing frame-slot owner authority");
    case PreparedObjectFrameSlotConsumerStatus::AmbiguousFrameSlotOwner:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::AmbiguousFrameSlotOwner,
          "prepared stack-slot home matched multiple frame-slot owners");
    case PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotFunction:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedFrameSlotFunction,
          "prepared frame-slot owner belongs to a different function");
    case PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotOffset:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedFrameSlotOffset,
          "prepared frame-slot owner offset disagrees with stack-slot home");
    case PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotSize:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedFrameSlotSize,
          "prepared frame-slot owner size disagrees with stack-slot home");
    case PreparedObjectFrameSlotConsumerStatus::MismatchedFrameSlotAlignment:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::MismatchedFrameSlotAlignment,
          "prepared frame-slot owner alignment disagrees with stack-slot home");
  }
  return std::nullopt;
}

std::optional<PreparedObjectConsumerDiagnostic> diagnose_prepared_object_consumer(
    const PreparedObjectParallelCopyObligation& obligation) {
  if (obligation.parallel_copy_bundle == nullptr) {
    return std::nullopt;
  }

  switch (obligation.execution_site) {
    case PreparedParallelCopyExecutionSite::PredecessorTerminator:
    case PreparedParallelCopyExecutionSite::SuccessorEntry:
      return std::nullopt;
    case PreparedParallelCopyExecutionSite::CriticalEdge:
      return make_consumer_diagnostic(
          PreparedObjectConsumerDiagnosticCategory::
              UnsupportedParallelCopyExecutionSite,
          "prepared critical-edge parallel-copy obligation has no target-consumable block event");
  }
  return std::nullopt;
}

std::vector<PreparedObjectParallelCopyObligation>
collect_unplaced_prepared_object_parallel_copy_obligations(
    const PreparedControlFlowFunction& control_flow) {
  std::vector<PreparedObjectParallelCopyObligation> obligations;
  for (const auto& parallel_copy_bundle : control_flow.parallel_copy_bundles) {
    if (prepared_object_parallel_copy_event_kind(parallel_copy_bundle).has_value()) {
      continue;
    }
    obligations.push_back(PreparedObjectParallelCopyObligation{
        .parallel_copy_bundle = &parallel_copy_bundle,
        .execution_site = parallel_copy_bundle.execution_site,
        .predecessor_label = parallel_copy_bundle.predecessor_label,
        .successor_label = parallel_copy_bundle.successor_label,
        .execution_block_label = parallel_copy_bundle.execution_block_label,
        .move_count = parallel_copy_bundle.moves.size(),
        .step_count = parallel_copy_bundle.steps.size(),
    });
  }
  return obligations;
}

std::vector<PreparedObjectTraversalEvent> make_prepared_object_function_traversal(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const bir::Function* bir_function,
    const PreparedSelectEdgeSourceProducerPlacementRecords*
        select_edge_source_producer_placements) {
  std::vector<PreparedObjectTraversalEvent> events;
  for (std::size_t block_index = 0; block_index < control_flow.blocks.size();
       ++block_index) {
    const auto& block = control_flow.blocks[block_index];
    const bir::Block* bir_block =
        bir_function != nullptr && block_index < bir_function->blocks.size()
            ? &bir_function->blocks[block_index]
            : nullptr;

    events.push_back(PreparedObjectTraversalEvent{
        .kind = PreparedObjectTraversalEventKind::Label,
        .block_index = block_index,
        .prepared_block = &block,
        .bir_block = bir_block,
    });

    append_parallel_copy_events_for_block(events,
                                          control_flow,
                                          value_locations,
                                          block,
                                          block_index,
                                          PreparedObjectTraversalEventKind::
                                              BlockEntryCopies,
                                          bir_block);
    append_unowned_block_entry_move_events(
        events, control_flow, value_locations, block, block_index, bir_block);

    if (bir_block != nullptr) {
      for (std::size_t instruction_index = 0;
           instruction_index < bir_block->insts.size();
           ++instruction_index) {
        append_before_instruction_move_events(
            events,
            control_flow,
            value_locations,
            block,
            block_index,
            instruction_index,
            bir_block,
            select_edge_source_producer_placements);
        events.push_back(PreparedObjectTraversalEvent{
            .kind = PreparedObjectTraversalEventKind::Instruction,
            .block_index = block_index,
            .instruction_index = instruction_index,
            .prepared_block = &block,
            .bir_block = bir_block,
            .instruction = &bir_block->insts[instruction_index],
        });
      }
    }

    append_parallel_copy_events_for_block(events,
                                          control_flow,
                                          value_locations,
                                          block,
                                          block_index,
                                          PreparedObjectTraversalEventKind::
                                              PreTerminatorCopies,
                                          bir_block);
    append_before_return_move_events(
        events, value_locations, block, block_index, bir_block);

    events.push_back(PreparedObjectTraversalEvent{
        .kind = PreparedObjectTraversalEventKind::Terminator,
        .block_index = block_index,
        .prepared_block = &block,
        .bir_block = bir_block,
        .terminator = bir_block != nullptr ? &bir_block->terminator : nullptr,
    });
  }
  return events;
}

}  // namespace c4c::backend::prepare
