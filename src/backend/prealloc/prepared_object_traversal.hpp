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

[[nodiscard]] std::optional<PreparedObjectTraversalEventKind>
prepared_object_parallel_copy_event_kind(
    const PreparedParallelCopyBundle& parallel_copy_bundle);

[[nodiscard]] std::vector<PreparedObjectTraversalEvent>
make_prepared_object_function_traversal(
    const PreparedControlFlowFunction& control_flow,
    const PreparedValueLocationFunction* value_locations,
    const bir::Function* bir_function = nullptr);

}  // namespace c4c::backend::prepare
