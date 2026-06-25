#include "src/backend/prealloc/prepared_object_traversal.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

struct Fixture {
  c4c::FunctionNameId function_name = 1;
  c4c::BlockLabelId predecessor_label = 10;
  c4c::BlockLabelId consumer_label = 20;
  c4c::BlockLabelId exit_label = 30;
  prepare::PreparedControlFlowFunction control_flow;
  prepare::PreparedValueLocationFunction locations;
  bir::Function bir_function;
};

bir::Inst binary_inst(std::string result, std::string lhs, std::string rhs) {
  return bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, std::move(result)),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, std::move(lhs)),
      .rhs = bir::Value::named(bir::TypeKind::I32, std::move(rhs)),
  };
}

Fixture make_fixture() {
  Fixture fixture;
  fixture.control_flow = prepare::PreparedControlFlowFunction{
      .function_name = fixture.function_name,
      .blocks = {
          prepare::PreparedControlFlowBlock{
              .block_label = fixture.predecessor_label,
              .terminator_kind = bir::TerminatorKind::Branch,
              .branch_target_label = fixture.consumer_label,
          },
          prepare::PreparedControlFlowBlock{
              .block_label = fixture.consumer_label,
              .terminator_kind = bir::TerminatorKind::Branch,
              .branch_target_label = fixture.exit_label,
          },
          prepare::PreparedControlFlowBlock{
              .block_label = fixture.exit_label,
              .terminator_kind = bir::TerminatorKind::Return,
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = fixture.predecessor_label,
              .successor_label = fixture.consumer_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::SuccessorEntry,
              .execution_block_label = fixture.consumer_label,
          },
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = fixture.consumer_label,
              .successor_label = fixture.exit_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
              .execution_block_label = fixture.consumer_label,
          },
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = fixture.predecessor_label,
              .successor_label = fixture.exit_label,
              .execution_site = prepare::PreparedParallelCopyExecutionSite::CriticalEdge,
          },
      },
  };

  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = fixture.function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 1,
              .source_parallel_copy_predecessor_label =
                  fixture.predecessor_label,
              .source_parallel_copy_successor_label = fixture.consumer_label,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 101,
                  .to_value_id = 201,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              }},
          },
          prepare::PreparedMoveBundle{
              .function_name = fixture.function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 1,
              .source_parallel_copy_predecessor_label = fixture.consumer_label,
              .source_parallel_copy_successor_label = fixture.exit_label,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 202,
                  .to_value_id = 302,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              }},
          },
      },
  };

  fixture.bir_function.blocks = {
      bir::Block{
          .label = "pred",
          .terminator =
              bir::BranchTerminator{.target_label = "consumer"},
          .label_id = fixture.predecessor_label,
      },
      bir::Block{
          .label = "consumer",
          .insts = {
              binary_inst("%v0", "%a", "%b"),
              binary_inst("%v1", "%v0", "%c"),
          },
          .terminator = bir::BranchTerminator{.target_label = "exit"},
          .label_id = fixture.consumer_label,
      },
      bir::Block{
          .label = "exit",
          .terminator = bir::ReturnTerminator{},
          .label_id = fixture.exit_label,
      },
  };

  return fixture;
}

int verify_canonical_consumer_schedule() {
  const auto fixture = make_fixture();
  const auto traversal = prepare::make_prepared_object_function_traversal(
      fixture.control_flow, &fixture.locations, &fixture.bir_function);

  std::vector<const prepare::PreparedObjectTraversalEvent*> consumer_events;
  for (const auto& event : traversal) {
    if (event.block_index == 1) {
      consumer_events.push_back(&event);
    }
  }

  const std::vector<prepare::PreparedObjectTraversalEventKind> expected_kinds{
      prepare::PreparedObjectTraversalEventKind::Label,
      prepare::PreparedObjectTraversalEventKind::BlockEntryCopies,
      prepare::PreparedObjectTraversalEventKind::Instruction,
      prepare::PreparedObjectTraversalEventKind::Instruction,
      prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies,
      prepare::PreparedObjectTraversalEventKind::Terminator,
  };

  if (!expect(consumer_events.size() == expected_kinds.size(),
              "consumer block should publish the canonical event count")) {
    return 1;
  }
  for (std::size_t index = 0; index < expected_kinds.size(); ++index) {
    if (!expect(consumer_events[index]->kind == expected_kinds[index],
                "consumer block should publish canonical event order")) {
      return 1;
    }
  }

  if (!expect(consumer_events[0]->prepared_block ==
                  &fixture.control_flow.blocks[1] &&
              consumer_events[0]->bir_block == &fixture.bir_function.blocks[1],
              "label event should carry prepared and BIR block identity") ||
      !expect(consumer_events[2]->instruction ==
                  &fixture.bir_function.blocks[1].insts[0] &&
              consumer_events[2]->instruction_index == 0 &&
              consumer_events[3]->instruction ==
                  &fixture.bir_function.blocks[1].insts[1] &&
              consumer_events[3]->instruction_index == 1,
              "instruction events should preserve instruction pointers and indexes") ||
      !expect(consumer_events[5]->terminator ==
                  &fixture.bir_function.blocks[1].terminator,
              "terminator event should carry the BIR terminator pointer")) {
    return 1;
  }

  return 0;
}

int verify_edge_copy_execution_site_placement() {
  const auto fixture = make_fixture();
  const auto traversal = prepare::make_prepared_object_function_traversal(
      fixture.control_flow, &fixture.locations, &fixture.bir_function);

  const prepare::PreparedObjectTraversalEvent* block_entry = nullptr;
  const prepare::PreparedObjectTraversalEvent* pre_terminator = nullptr;
  for (const auto& event : traversal) {
    if (event.block_index != 1) {
      continue;
    }
    if (event.kind == prepare::PreparedObjectTraversalEventKind::BlockEntryCopies) {
      block_entry = &event;
    } else if (event.kind ==
               prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies) {
      pre_terminator = &event;
    }
  }

  if (!expect(block_entry != nullptr && pre_terminator != nullptr,
              "consumer traversal should contain both edge-copy placement events")) {
    return 1;
  }
  if (!expect(block_entry->parallel_copy_bundle ==
                  &fixture.control_flow.parallel_copy_bundles[0] &&
              block_entry->move_bundle == &fixture.locations.move_bundles[0],
              "successor-entry parallel copy should become block-entry copies") ||
      !expect(pre_terminator->parallel_copy_bundle ==
                  &fixture.control_flow.parallel_copy_bundles[1] &&
              pre_terminator->move_bundle == &fixture.locations.move_bundles[1],
              "predecessor-terminator parallel copy should become pre-terminator copies")) {
    return 1;
  }

  if (!expect(prepare::prepared_object_parallel_copy_event_kind(
                  fixture.control_flow.parallel_copy_bundles[0]) ==
                  prepare::PreparedObjectTraversalEventKind::BlockEntryCopies &&
              prepare::prepared_object_parallel_copy_event_kind(
                  fixture.control_flow.parallel_copy_bundles[1]) ==
                  prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies,
              "parallel-copy execution sites should map to consumer event sites") ||
      !expect(!prepare::prepared_object_parallel_copy_event_kind(
                   fixture.control_flow.parallel_copy_bundles[2])
                   .has_value(),
              "critical-edge copies should not be silently placed in a block schedule")) {
    return 1;
  }

  return 0;
}

int verify_event_kind_names() {
  return expect(prepare::prepared_object_traversal_event_kind_name(
                    prepare::PreparedObjectTraversalEventKind::
                        BlockEntryCopies) == "block_entry_copies" &&
                    prepare::prepared_object_traversal_event_kind_name(
                        prepare::PreparedObjectTraversalEventKind::
                            PreTerminatorCopies) == "pre_terminator_copies",
                "prepared object traversal event names should remain stable")
             ? 0
             : 1;
}

}  // namespace

int main() {
  if (const auto result = verify_event_kind_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_canonical_consumer_schedule(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_edge_copy_execution_site_placement();
      result != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
