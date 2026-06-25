#pragma once

#include "control_flow.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedObjectTraversalEventKind {
  Label,
  BlockEntryCopies,
  Instruction,
  PreTerminatorCopies,
  Terminator,
};

enum class PreparedObjectSelectConsumerKind {
  NotSelectInstruction,
  OrdinarySelect,
  PreparedJoinTransferCarrier,
  MissingPreparedJoinTransfer,
  AmbiguousPreparedJoinTransfer,
  UnsupportedSelectResult,
  UnsupportedPreparedJoinTransferKind,
  UnsupportedPreparedJoinTransferCarrierKind,
  MalformedPreparedJoinTransferCarrier,
};

enum class PreparedObjectValueHomeConsumerStatus {
  Available,
  MissingValue,
  NonNamedValue,
  MissingNames,
  MissingValueLocations,
  MissingPreparedValueName,
  MissingPreparedValueHome,
  AmbiguousPreparedValueHome,
  ConflictingPreparedValueId,
  ConflictingPreparedValueHomeLookup,
  UnsupportedValueHomeKind,
  IncompleteValueHome,
};

[[nodiscard]] constexpr std::string_view prepared_object_traversal_event_kind_name(
    PreparedObjectTraversalEventKind kind) {
  switch (kind) {
    case PreparedObjectTraversalEventKind::Label:
      return "label";
    case PreparedObjectTraversalEventKind::BlockEntryCopies:
      return "block_entry_copies";
    case PreparedObjectTraversalEventKind::Instruction:
      return "instruction";
    case PreparedObjectTraversalEventKind::PreTerminatorCopies:
      return "pre_terminator_copies";
    case PreparedObjectTraversalEventKind::Terminator:
      return "terminator";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view prepared_object_select_consumer_kind_name(
    PreparedObjectSelectConsumerKind kind) {
  switch (kind) {
    case PreparedObjectSelectConsumerKind::NotSelectInstruction:
      return "not_select_instruction";
    case PreparedObjectSelectConsumerKind::OrdinarySelect:
      return "ordinary_select";
    case PreparedObjectSelectConsumerKind::PreparedJoinTransferCarrier:
      return "prepared_join_transfer_carrier";
    case PreparedObjectSelectConsumerKind::MissingPreparedJoinTransfer:
      return "missing_prepared_join_transfer";
    case PreparedObjectSelectConsumerKind::AmbiguousPreparedJoinTransfer:
      return "ambiguous_prepared_join_transfer";
    case PreparedObjectSelectConsumerKind::UnsupportedSelectResult:
      return "unsupported_select_result";
    case PreparedObjectSelectConsumerKind::UnsupportedPreparedJoinTransferKind:
      return "unsupported_prepared_join_transfer_kind";
    case PreparedObjectSelectConsumerKind::UnsupportedPreparedJoinTransferCarrierKind:
      return "unsupported_prepared_join_transfer_carrier_kind";
    case PreparedObjectSelectConsumerKind::MalformedPreparedJoinTransferCarrier:
      return "malformed_prepared_join_transfer_carrier";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view
prepared_object_value_home_consumer_status_name(
    PreparedObjectValueHomeConsumerStatus status) {
  switch (status) {
    case PreparedObjectValueHomeConsumerStatus::Available:
      return "available";
    case PreparedObjectValueHomeConsumerStatus::MissingValue:
      return "missing_value";
    case PreparedObjectValueHomeConsumerStatus::NonNamedValue:
      return "non_named_value";
    case PreparedObjectValueHomeConsumerStatus::MissingNames:
      return "missing_names";
    case PreparedObjectValueHomeConsumerStatus::MissingValueLocations:
      return "missing_value_locations";
    case PreparedObjectValueHomeConsumerStatus::MissingPreparedValueName:
      return "missing_prepared_value_name";
    case PreparedObjectValueHomeConsumerStatus::MissingPreparedValueHome:
      return "missing_prepared_value_home";
    case PreparedObjectValueHomeConsumerStatus::AmbiguousPreparedValueHome:
      return "ambiguous_prepared_value_home";
    case PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueId:
      return "conflicting_prepared_value_id";
    case PreparedObjectValueHomeConsumerStatus::ConflictingPreparedValueHomeLookup:
      return "conflicting_prepared_value_home_lookup";
    case PreparedObjectValueHomeConsumerStatus::UnsupportedValueHomeKind:
      return "unsupported_value_home_kind";
    case PreparedObjectValueHomeConsumerStatus::IncompleteValueHome:
      return "incomplete_value_home";
  }
  return "unknown";
}

struct PreparedObjectTraversalEvent {
  PreparedObjectTraversalEventKind kind = PreparedObjectTraversalEventKind::Label;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const PreparedControlFlowBlock* prepared_block = nullptr;
  const bir::Block* bir_block = nullptr;
  const bir::Inst* instruction = nullptr;
  const bir::Terminator* terminator = nullptr;
  const PreparedMoveBundle* move_bundle = nullptr;
  const PreparedParallelCopyBundle* parallel_copy_bundle = nullptr;
};

struct PreparedObjectSelectConsumerQuery {
  const PreparedControlFlowFunction* control_flow = nullptr;
  BlockLabelId block_label = kInvalidBlockLabel;
  const bir::Inst* instruction = nullptr;
  bool require_prepared_join_transfer = false;
};

struct PreparedObjectSelectConsumerClassification {
  PreparedObjectSelectConsumerKind kind =
      PreparedObjectSelectConsumerKind::NotSelectInstruction;
  const bir::SelectInst* select = nullptr;
  const PreparedJoinTransfer* join_transfer = nullptr;
  PreparedJoinTransferCarrierKind carrier_kind = PreparedJoinTransferCarrierKind::None;
};

struct PreparedObjectValueHomeConsumerQuery {
  const PreparedNameTables* names = nullptr;
  const PreparedRegallocFunction* regalloc = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
  const bir::Value* value = nullptr;
};

struct PreparedObjectValueHomeConsumerClassification {
  PreparedObjectValueHomeConsumerStatus status =
      PreparedObjectValueHomeConsumerStatus::MissingValue;
  const PreparedValueHome* home = nullptr;
  ValueNameId value_name = kInvalidValueName;
  PreparedValueId value_id = 0;
  PreparedValueHomeKind home_kind = PreparedValueHomeKind::None;
};

[[nodiscard]] std::optional<PreparedObjectTraversalEventKind>
prepared_object_parallel_copy_event_kind(
    const PreparedParallelCopyBundle& parallel_copy_bundle);

[[nodiscard]] PreparedObjectSelectConsumerClassification
classify_prepared_object_select_consumer(
    const PreparedObjectSelectConsumerQuery& query);

[[nodiscard]] PreparedObjectSelectConsumerClassification
classify_prepared_object_select_consumer(
    const PreparedControlFlowFunction* control_flow,
    BlockLabelId block_label,
    const bir::Inst& instruction,
    bool require_prepared_join_transfer = false);

[[nodiscard]] PreparedObjectValueHomeConsumerClassification
classify_prepared_object_value_home_consumer(
    const PreparedObjectValueHomeConsumerQuery& query);

[[nodiscard]] PreparedObjectValueHomeConsumerClassification
classify_prepared_object_value_home_consumer(
    const PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& value,
    const PreparedRegallocFunction* regalloc = nullptr,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] std::vector<PreparedObjectTraversalEvent>
make_prepared_object_function_traversal(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const bir::Function* bir_function = nullptr);

}  // namespace c4c::backend::prepare
