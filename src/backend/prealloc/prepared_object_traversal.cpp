#include "prepared_object_traversal.hpp"

#include <algorithm>

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

  result.status = PreparedObjectMoveBundleConsumerStatus::Available;
  return result;
}

PreparedObjectMoveBundleConsumerClassification
classify_prepared_object_move_bundle_consumer(
    const PreparedObjectTraversalEvent& event) {
  return classify_prepared_object_move_bundle_consumer(
      PreparedObjectMoveBundleConsumerQuery{.event = &event});
}

std::vector<PreparedObjectTraversalEvent> make_prepared_object_function_traversal(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const bir::Function* bir_function) {
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
