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
