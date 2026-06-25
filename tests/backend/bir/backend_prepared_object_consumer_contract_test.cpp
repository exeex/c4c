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
  prepare::PreparedNameTables names;
  prepare::PreparedControlFlowFunction control_flow;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedStackLayout stack_layout;
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

bir::Inst select_inst(std::string result) {
  return bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, std::move(result)),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%cond"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = bir::Value::named(bir::TypeKind::I32, "%true.in"),
      .false_value = bir::Value::named(bir::TypeKind::I32, "%false.in"),
  };
}

prepare::PreparedJoinTransfer select_materialization_join_transfer(
    const Fixture& fixture,
    std::string result) {
  return prepare::PreparedJoinTransfer{
      .function_name = fixture.function_name,
      .join_block_label = fixture.consumer_label,
      .result = bir::Value::named(bir::TypeKind::I32, std::move(result)),
      .kind = prepare::PreparedJoinTransferKind::PhiEdge,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .edge_transfers = {
          prepare::PreparedEdgeValueTransfer{
              .predecessor_label = fixture.predecessor_label,
              .successor_label = fixture.consumer_label,
              .incoming_value =
                  bir::Value::named(bir::TypeKind::I32, "%true.in"),
              .destination_value =
                  bir::Value::named(bir::TypeKind::I32, "%selected"),
          },
          prepare::PreparedEdgeValueTransfer{
              .predecessor_label = fixture.exit_label,
              .successor_label = fixture.consumer_label,
              .incoming_value =
                  bir::Value::named(bir::TypeKind::I32, "%false.in"),
              .destination_value =
                  bir::Value::named(bir::TypeKind::I32, "%selected"),
          },
      },
      .source_true_transfer_index = 0,
      .source_false_transfer_index = 1,
  };
}

prepare::PreparedValueHome value_home(Fixture& fixture,
                                      std::string_view value_name,
                                      prepare::PreparedValueId value_id,
                                      prepare::PreparedValueHomeKind kind) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = fixture.function_name,
      .value_name = fixture.names.value_names.intern(value_name),
      .kind = kind,
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

int verify_select_consumer_kind_names() {
  return expect(
             prepare::prepared_object_select_consumer_kind_name(
                 prepare::PreparedObjectSelectConsumerKind::OrdinarySelect) ==
                     "ordinary_select" &&
                 prepare::prepared_object_select_consumer_kind_name(
                     prepare::PreparedObjectSelectConsumerKind::
                         PreparedJoinTransferCarrier) ==
                     "prepared_join_transfer_carrier" &&
                 prepare::prepared_object_select_consumer_kind_name(
                     prepare::PreparedObjectSelectConsumerKind::
                         MissingPreparedJoinTransfer) ==
                     "missing_prepared_join_transfer" &&
                 prepare::prepared_object_select_consumer_kind_name(
                     prepare::PreparedObjectSelectConsumerKind::
                         AmbiguousPreparedJoinTransfer) ==
                     "ambiguous_prepared_join_transfer",
             "prepared object select consumer kind names should remain stable")
             ? 0
             : 1;
}

int verify_value_home_consumer_status_names() {
  return expect(
             prepare::prepared_object_value_home_consumer_status_name(
                 prepare::PreparedObjectValueHomeConsumerStatus::Available) ==
                     "available" &&
                 prepare::prepared_object_value_home_consumer_status_name(
                     prepare::PreparedObjectValueHomeConsumerStatus::
                         MissingPreparedValueHome) ==
                     "missing_prepared_value_home" &&
                 prepare::prepared_object_value_home_consumer_status_name(
                     prepare::PreparedObjectValueHomeConsumerStatus::
                         AmbiguousPreparedValueHome) ==
                     "ambiguous_prepared_value_home" &&
                 prepare::prepared_object_value_home_consumer_status_name(
                     prepare::PreparedObjectValueHomeConsumerStatus::
                         UnsupportedValueHomeKind) ==
                     "unsupported_value_home_kind",
             "prepared object value-home consumer status names should remain stable")
             ? 0
             : 1;
}

int verify_move_bundle_consumer_status_names() {
  return expect(
             prepare::prepared_object_move_bundle_consumer_status_name(
                 prepare::PreparedObjectMoveBundleConsumerStatus::Available) ==
                     "available" &&
                 prepare::prepared_object_move_bundle_consumer_status_name(
                     prepare::PreparedObjectMoveBundleConsumerStatus::
                         MissingMoveBundle) ==
                     "missing_move_bundle" &&
                 prepare::prepared_object_move_bundle_consumer_status_name(
                     prepare::PreparedObjectMoveBundleConsumerStatus::
                         MissingParallelCopyBundle) ==
                     "missing_parallel_copy_bundle" &&
                 prepare::prepared_object_move_bundle_consumer_status_name(
                     prepare::PreparedObjectMoveBundleConsumerStatus::
                         MismatchedParallelCopyExecutionSite) ==
                     "mismatched_parallel_copy_execution_site",
             "prepared object move-bundle consumer status names should remain stable")
             ? 0
             : 1;
}

int verify_frame_slot_consumer_status_names() {
  return expect(
             prepare::prepared_object_frame_slot_consumer_status_name(
                 prepare::PreparedObjectFrameSlotConsumerStatus::Available) ==
                     "available" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MissingValueHome) ==
                     "missing_value_home" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         UnsupportedValueHomeKind) ==
                     "unsupported_value_home_kind" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         IncompleteStackSlotHome) ==
                     "incomplete_stack_slot_home" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MissingStackLayout) ==
                     "missing_stack_layout" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MissingFrameSlotOwner) ==
                     "missing_frame_slot_owner" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         AmbiguousFrameSlotOwner) ==
                     "ambiguous_frame_slot_owner" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MismatchedFrameSlotFunction) ==
                     "mismatched_frame_slot_function" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MismatchedFrameSlotOffset) ==
                     "mismatched_frame_slot_offset" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MismatchedFrameSlotSize) ==
                     "mismatched_frame_slot_size" &&
                 prepare::prepared_object_frame_slot_consumer_status_name(
                     prepare::PreparedObjectFrameSlotConsumerStatus::
                         MismatchedFrameSlotAlignment) ==
                     "mismatched_frame_slot_alignment",
             "prepared object frame-slot consumer status names should remain stable")
             ? 0
             : 1;
}

const prepare::PreparedObjectTraversalEvent* find_event(
    const std::vector<prepare::PreparedObjectTraversalEvent>& traversal,
    prepare::PreparedObjectTraversalEventKind kind,
    std::size_t block_index) {
  for (const auto& event : traversal) {
    if (event.kind == kind && event.block_index == block_index) {
      return &event;
    }
  }
  return nullptr;
}

int verify_real_select_vs_required_missing_carrier_classification() {
  auto fixture = make_fixture();
  fixture.bir_function.blocks[1].insts.push_back(select_inst("%ordinary"));
  const auto& select = fixture.bir_function.blocks[1].insts.back();
  const auto& binary = fixture.bir_function.blocks[1].insts.front();

  const auto ordinary = prepare::classify_prepared_object_select_consumer(
      &fixture.control_flow, fixture.consumer_label, select);
  const auto missing = prepare::classify_prepared_object_select_consumer(
      &fixture.control_flow, fixture.consumer_label, select, true);
  const auto non_select = prepare::classify_prepared_object_select_consumer(
      &fixture.control_flow, fixture.consumer_label, binary);

  if (!expect(ordinary.kind ==
                  prepare::PreparedObjectSelectConsumerKind::OrdinarySelect &&
              ordinary.select == std::get_if<bir::SelectInst>(&select) &&
              ordinary.join_transfer == nullptr,
              "select without a prepared join transfer should remain an ordinary select") ||
      !expect(missing.kind == prepare::PreparedObjectSelectConsumerKind::
                                  MissingPreparedJoinTransfer &&
              missing.select == std::get_if<bir::SelectInst>(&select) &&
              missing.join_transfer == nullptr,
              "required prepared select-carrier query should fail closed when the contract is missing") ||
      !expect(non_select.kind ==
                  prepare::PreparedObjectSelectConsumerKind::NotSelectInstruction &&
              non_select.select == nullptr,
              "non-select instructions should not be classified as selects")) {
    return 1;
  }

  return 0;
}

int verify_select_materialized_join_transfer_carrier_classification() {
  auto fixture = make_fixture();
  fixture.bir_function.blocks[1].insts.push_back(select_inst("%selected"));
  fixture.control_flow.join_transfers.push_back(
      select_materialization_join_transfer(fixture, "%selected"));

  const auto& select = fixture.bir_function.blocks[1].insts.back();
  const auto classification = prepare::classify_prepared_object_select_consumer(
      &fixture.control_flow, fixture.consumer_label, select);

  if (!expect(classification.kind == prepare::PreparedObjectSelectConsumerKind::
                                         PreparedJoinTransferCarrier &&
              classification.select == std::get_if<bir::SelectInst>(&select) &&
              classification.join_transfer ==
                  &fixture.control_flow.join_transfers.front() &&
              classification.carrier_kind == prepare::
                                                 PreparedJoinTransferCarrierKind::
                                                     SelectMaterialization,
              "matching select-materialized join transfer should classify as a prepared carrier")) {
    return 1;
  }

  return 0;
}

int verify_ambiguous_and_unsupported_select_carrier_classification() {
  auto ambiguous_fixture = make_fixture();
  ambiguous_fixture.bir_function.blocks[1].insts.push_back(select_inst("%selected"));
  ambiguous_fixture.control_flow.join_transfers.push_back(
      select_materialization_join_transfer(ambiguous_fixture, "%selected"));
  ambiguous_fixture.control_flow.join_transfers.push_back(
      select_materialization_join_transfer(ambiguous_fixture, "%selected"));

  const auto ambiguous = prepare::classify_prepared_object_select_consumer(
      &ambiguous_fixture.control_flow,
      ambiguous_fixture.consumer_label,
      ambiguous_fixture.bir_function.blocks[1].insts.back());
  if (!expect(ambiguous.kind == prepare::PreparedObjectSelectConsumerKind::
                                    AmbiguousPreparedJoinTransfer &&
              ambiguous.join_transfer == nullptr,
              "duplicate matching join transfers should fail closed as ambiguous")) {
    return 1;
  }

  auto unsupported_fixture = make_fixture();
  unsupported_fixture.bir_function.blocks[1].insts.push_back(select_inst("%selected"));
  auto unsupported_transfer =
      select_materialization_join_transfer(unsupported_fixture, "%selected");
  unsupported_transfer.carrier_kind =
      prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot;
  unsupported_fixture.control_flow.join_transfers.push_back(
      std::move(unsupported_transfer));
  const auto unsupported = prepare::classify_prepared_object_select_consumer(
      &unsupported_fixture.control_flow,
      unsupported_fixture.consumer_label,
      unsupported_fixture.bir_function.blocks[1].insts.back());
  if (!expect(unsupported.kind ==
                  prepare::PreparedObjectSelectConsumerKind::
                      UnsupportedPreparedJoinTransferCarrierKind &&
              unsupported.join_transfer ==
                  &unsupported_fixture.control_flow.join_transfers.front(),
              "non-select join-transfer carrier should not be consumed as a select materialization")) {
    return 1;
  }

  auto malformed_fixture = make_fixture();
  malformed_fixture.bir_function.blocks[1].insts.push_back(select_inst("%selected"));
  auto malformed_transfer =
      select_materialization_join_transfer(malformed_fixture, "%selected");
  malformed_transfer.source_false_transfer_index = 7;
  malformed_fixture.control_flow.join_transfers.push_back(
      std::move(malformed_transfer));
  const auto malformed = prepare::classify_prepared_object_select_consumer(
      &malformed_fixture.control_flow,
      malformed_fixture.consumer_label,
      malformed_fixture.bir_function.blocks[1].insts.back());
  if (!expect(malformed.kind ==
                  prepare::PreparedObjectSelectConsumerKind::
                      MalformedPreparedJoinTransferCarrier,
              "select-materialized join transfer with invalid edge indexes should fail closed")) {
    return 1;
  }

  return 0;
}

int verify_move_bundle_consumer_available_classification() {
  auto fixture = make_fixture();
  fixture.locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = fixture.function_name,
      .phase = prepare::PreparedMovePhase::BeforeReturn,
      .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      .block_index = 2,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = 801,
          .to_value_id = 802,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind =
              prepare::PreparedMoveStorageKind::Register,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      }},
  });

  const auto traversal = prepare::make_prepared_object_function_traversal(
      fixture.control_flow, &fixture.locations, &fixture.bir_function);
  const auto* block_entry = find_event(
      traversal, prepare::PreparedObjectTraversalEventKind::BlockEntryCopies, 1);
  const auto* predecessor_terminator = find_event(
      traversal, prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies, 1);
  const auto* before_return = find_event(
      traversal, prepare::PreparedObjectTraversalEventKind::PreTerminatorCopies, 2);
  if (!expect(block_entry != nullptr && predecessor_terminator != nullptr &&
                  before_return != nullptr,
              "move-bundle consumer test should find traversal move events")) {
    return 1;
  }

  const auto block_entry_classification =
      prepare::classify_prepared_object_move_bundle_consumer(*block_entry);
  const auto predecessor_terminator_classification =
      prepare::classify_prepared_object_move_bundle_consumer(
          *predecessor_terminator);
  const auto before_return_classification =
      prepare::classify_prepared_object_move_bundle_consumer(*before_return);
  if (!expect(block_entry_classification.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::Available &&
              block_entry_classification.move_bundle ==
                  &fixture.locations.move_bundles[0] &&
              block_entry_classification.parallel_copy_bundle ==
                  &fixture.control_flow.parallel_copy_bundles[0] &&
              block_entry_classification.phase ==
                  prepare::PreparedMovePhase::BlockEntry &&
              block_entry_classification.move_count == 1,
              "successor-entry move bundle should classify as available") ||
      !expect(predecessor_terminator_classification.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::Available &&
              predecessor_terminator_classification.move_bundle ==
                  &fixture.locations.move_bundles[1] &&
              predecessor_terminator_classification.parallel_copy_bundle ==
                  &fixture.control_flow.parallel_copy_bundles[1] &&
              predecessor_terminator_classification.phase ==
                  prepare::PreparedMovePhase::BlockEntry,
              "predecessor-terminator edge copy should classify through traversal placement") ||
      !expect(before_return_classification.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::Available &&
              before_return_classification.move_bundle ==
                  &fixture.locations.move_bundles[2] &&
              before_return_classification.parallel_copy_bundle == nullptr &&
              before_return_classification.phase ==
                  prepare::PreparedMovePhase::BeforeReturn,
              "ordinary before-return move bundle should classify as available")) {
    return 1;
  }

  return 0;
}

int verify_move_bundle_consumer_fail_closed_statuses() {
  auto fixture = make_fixture();
  const auto traversal = prepare::make_prepared_object_function_traversal(
      fixture.control_flow, &fixture.locations, &fixture.bir_function);
  const auto* label = find_event(
      traversal, prepare::PreparedObjectTraversalEventKind::Label, 1);
  const auto* block_entry = find_event(
      traversal, prepare::PreparedObjectTraversalEventKind::BlockEntryCopies, 1);
  if (!expect(label != nullptr && block_entry != nullptr,
              "move-bundle fail-closed test should find label and copy events")) {
    return 1;
  }

  const auto unsupported =
      prepare::classify_prepared_object_move_bundle_consumer(*label);
  auto missing_bundle_event = *block_entry;
  missing_bundle_event.move_bundle = nullptr;
  const auto missing_bundle =
      prepare::classify_prepared_object_move_bundle_consumer(
          missing_bundle_event);
  auto missing_parallel_copy_event = *block_entry;
  missing_parallel_copy_event.parallel_copy_bundle = nullptr;
  const auto missing_parallel_copy =
      prepare::classify_prepared_object_move_bundle_consumer(
          missing_parallel_copy_event);
  auto mismatched_execution_event = *block_entry;
  mismatched_execution_event.parallel_copy_bundle =
      &fixture.control_flow.parallel_copy_bundles[1];
  const auto mismatched_execution =
      prepare::classify_prepared_object_move_bundle_consumer(
          mismatched_execution_event);
  auto critical_edge_event = *block_entry;
  critical_edge_event.parallel_copy_bundle =
      &fixture.control_flow.parallel_copy_bundles[2];
  const auto critical_edge =
      prepare::classify_prepared_object_move_bundle_consumer(
          critical_edge_event);

  if (!expect(unsupported.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      UnsupportedEventKind,
              "non-copy traversal events should not classify as move-bundle consumers") ||
      !expect(missing_bundle.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      MissingMoveBundle,
              "copy traversal events without a prepared bundle should fail closed") ||
      !expect(missing_parallel_copy.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      MissingParallelCopyBundle,
              "out-of-ssa copy bundles without a prepared parallel-copy owner should fail closed") ||
      !expect(mismatched_execution.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      MismatchedParallelCopyExecutionSite,
              "parallel-copy owners placed at the wrong traversal site should fail closed") ||
      !expect(critical_edge.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      UnsupportedParallelCopyExecutionSite,
              "critical-edge copy bundles should not be silently consumed from a block event")) {
    return 1;
  }

  auto empty_fixture = make_fixture();
  empty_fixture.locations.move_bundles.front().moves.clear();
  const auto empty_traversal = prepare::make_prepared_object_function_traversal(
      empty_fixture.control_flow, &empty_fixture.locations, &empty_fixture.bir_function);
  const auto* empty_block_entry = find_event(
      empty_traversal,
      prepare::PreparedObjectTraversalEventKind::BlockEntryCopies,
      1);
  const auto empty =
      empty_block_entry == nullptr
          ? prepare::PreparedObjectMoveBundleConsumerClassification{}
          : prepare::classify_prepared_object_move_bundle_consumer(
                *empty_block_entry);
  if (!expect(empty.status ==
                  prepare::PreparedObjectMoveBundleConsumerStatus::
                      EmptyMoveBundle,
              "empty move bundles should fail closed before target consumption")) {
    return 1;
  }

  return 0;
}

int verify_supported_value_home_consumer_classification() {
  auto fixture = make_fixture();
  auto reg_home = value_home(
      fixture, "%reg", 401, prepare::PreparedValueHomeKind::Register);
  reg_home.register_name = "r.prepared";
  fixture.locations.value_homes.push_back(std::move(reg_home));

  auto stack_home = value_home(
      fixture, "%stack", 402, prepare::PreparedValueHomeKind::StackSlot);
  stack_home.slot_id = prepare::PreparedFrameSlotId{7};
  stack_home.offset_bytes = std::size_t{32};
  stack_home.size_bytes = std::size_t{4};
  stack_home.align_bytes = std::size_t{4};
  fixture.locations.value_homes.push_back(std::move(stack_home));

  auto immediate_home =
      value_home(fixture,
                 "%imm",
                 403,
                 prepare::PreparedValueHomeKind::RematerializableImmediate);
  immediate_home.immediate_i32 = 42;
  fixture.locations.value_homes.push_back(std::move(immediate_home));

  const auto base_name = fixture.names.value_names.intern("%base");
  auto pointer_home =
      value_home(fixture,
                 "%ptr",
                 404,
                 prepare::PreparedValueHomeKind::PointerBasePlusOffset);
  pointer_home.pointer_base_value_name = base_name;
  pointer_home.pointer_byte_delta = 16;
  fixture.locations.value_homes.push_back(std::move(pointer_home));

  const auto lookups =
      prepare::make_prepared_value_home_lookups(&fixture.locations);
  const auto query = [&](std::string value_name) {
    const auto value = bir::Value::named(bir::TypeKind::I32, std::move(value_name));
    return prepare::classify_prepared_object_value_home_consumer(
        fixture.names, &fixture.locations, value, nullptr, &lookups);
  };

  const auto reg = query("%reg");
  const auto stack = query("%stack");
  const auto immediate = query("%imm");
  const auto pointer = query("%ptr");
  if (!expect(reg.status ==
                  prepare::PreparedObjectValueHomeConsumerStatus::Available &&
              reg.home == &fixture.locations.value_homes[0] &&
              reg.home_kind == prepare::PreparedValueHomeKind::Register,
              "register value home should classify as a supported prepared object home") ||
      !expect(stack.status ==
                  prepare::PreparedObjectValueHomeConsumerStatus::Available &&
              stack.home == &fixture.locations.value_homes[1] &&
              stack.home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "stack-slot value home should classify as a supported prepared object home") ||
      !expect(immediate.status ==
                  prepare::PreparedObjectValueHomeConsumerStatus::Available &&
              immediate.home == &fixture.locations.value_homes[2] &&
              immediate.home_kind ==
                  prepare::PreparedValueHomeKind::RematerializableImmediate,
              "rematerializable immediate home should classify as a supported prepared object home") ||
      !expect(pointer.status ==
                  prepare::PreparedObjectValueHomeConsumerStatus::Available &&
              pointer.home == &fixture.locations.value_homes[3] &&
              pointer.home_kind ==
                  prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              "pointer base-plus-offset home should classify as a supported prepared object home")) {
    return 1;
  }

  return 0;
}

int verify_value_home_consumer_missing_and_unsupported_statuses() {
  auto missing_name_fixture = make_fixture();
  const auto unknown_value = bir::Value::named(bir::TypeKind::I32, "%unknown");
  const auto missing_name = prepare::classify_prepared_object_value_home_consumer(
      missing_name_fixture.names,
      &missing_name_fixture.locations,
      unknown_value);
  if (!expect(missing_name.status == prepare::
                                       PreparedObjectValueHomeConsumerStatus::
                                           MissingPreparedValueName,
              "value-home query should distinguish missing prepared value names")) {
    return 1;
  }

  auto missing_home_fixture = make_fixture();
  missing_home_fixture.names.value_names.intern("%missing.home");
  const auto missing_home_value =
      bir::Value::named(bir::TypeKind::I32, "%missing.home");
  const auto missing_home = prepare::classify_prepared_object_value_home_consumer(
      missing_home_fixture.names,
      &missing_home_fixture.locations,
      missing_home_value);
  if (!expect(missing_home.status == prepare::
                                       PreparedObjectValueHomeConsumerStatus::
                                           MissingPreparedValueHome,
              "value-home query should distinguish missing prepared homes")) {
    return 1;
  }

  auto ambiguous_fixture = make_fixture();
  auto ambiguous_a = value_home(
      ambiguous_fixture, "%ambiguous", 501, prepare::PreparedValueHomeKind::Register);
  ambiguous_a.register_name = "r.a";
  ambiguous_fixture.locations.value_homes.push_back(std::move(ambiguous_a));
  auto ambiguous_b = value_home(
      ambiguous_fixture, "%ambiguous", 502, prepare::PreparedValueHomeKind::Register);
  ambiguous_b.register_name = "r.b";
  ambiguous_fixture.locations.value_homes.push_back(std::move(ambiguous_b));
  const auto ambiguous_value =
      bir::Value::named(bir::TypeKind::I32, "%ambiguous");
  const auto ambiguous = prepare::classify_prepared_object_value_home_consumer(
      ambiguous_fixture.names,
      &ambiguous_fixture.locations,
      ambiguous_value);
  if (!expect(ambiguous.status == prepare::
                                      PreparedObjectValueHomeConsumerStatus::
                                          AmbiguousPreparedValueHome,
              "duplicate prepared homes for one value name should fail closed")) {
    return 1;
  }

  auto unsupported_fixture = make_fixture();
  unsupported_fixture.locations.value_homes.push_back(
      value_home(unsupported_fixture,
                 "%none",
                 601,
                 prepare::PreparedValueHomeKind::None));
  const auto none_value = bir::Value::named(bir::TypeKind::I32, "%none");
  const auto unsupported = prepare::classify_prepared_object_value_home_consumer(
      unsupported_fixture.names,
      &unsupported_fixture.locations,
      none_value);
  if (!expect(unsupported.status == prepare::
                                       PreparedObjectValueHomeConsumerStatus::
                                           UnsupportedValueHomeKind,
              "none homes should not be consumed as concrete prepared object homes")) {
    return 1;
  }

  auto incomplete_fixture = make_fixture();
  incomplete_fixture.locations.value_homes.push_back(
      value_home(incomplete_fixture,
                 "%incomplete.reg",
                 701,
                 prepare::PreparedValueHomeKind::Register));
  const auto incomplete_value =
      bir::Value::named(bir::TypeKind::I32, "%incomplete.reg");
  const auto incomplete = prepare::classify_prepared_object_value_home_consumer(
      incomplete_fixture.names,
      &incomplete_fixture.locations,
      incomplete_value);
  if (!expect(incomplete.status == prepare::
                                      PreparedObjectValueHomeConsumerStatus::
                                          IncompleteValueHome,
              "incomplete register homes should fail closed before target consumption")) {
    return 1;
  }

  return 0;
}

prepare::PreparedValueHome stack_slot_value_home(Fixture& fixture,
                                                 std::string_view value_name,
                                                 prepare::PreparedValueId value_id,
                                                 prepare::PreparedFrameSlotId slot_id,
                                                 std::size_t offset_bytes,
                                                 std::size_t size_bytes,
                                                 std::size_t align_bytes) {
  auto home =
      value_home(fixture, value_name, value_id, prepare::PreparedValueHomeKind::StackSlot);
  home.slot_id = slot_id;
  home.offset_bytes = offset_bytes;
  home.size_bytes = size_bytes;
  home.align_bytes = align_bytes;
  return home;
}

prepare::PreparedFrameSlot frame_slot(const Fixture& fixture,
                                      prepare::PreparedFrameSlotId slot_id,
                                      std::size_t offset_bytes,
                                      std::size_t size_bytes,
                                      std::size_t align_bytes) {
  return prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = slot_id + 100,
      .function_name = fixture.function_name,
      .offset_bytes = offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
  };
}

int verify_frame_slot_consumer_available_classification() {
  auto fixture = make_fixture();
  fixture.locations.value_homes.push_back(
      stack_slot_value_home(fixture, "%stack.owner", 901, 11, 48, 8, 8));
  fixture.stack_layout.frame_slots.push_back(frame_slot(fixture, 11, 48, 8, 8));

  const auto classification = prepare::classify_prepared_object_frame_slot_consumer(
      fixture.stack_layout, fixture.locations.value_homes.front());
  if (!expect(classification.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::Available &&
              classification.value_home == &fixture.locations.value_homes.front() &&
              classification.frame_slot == &fixture.stack_layout.frame_slots.front() &&
              classification.slot_id == prepare::PreparedFrameSlotId{11} &&
              classification.offset_bytes == 48 &&
              classification.size_bytes == 8 &&
              classification.align_bytes == 8,
              "stack-slot value home should classify with its prepared frame-slot owner")) {
    return 1;
  }

  return 0;
}

int verify_frame_slot_consumer_fail_closed_statuses() {
  auto fixture = make_fixture();
  auto available_home =
      stack_slot_value_home(fixture, "%stack.owner", 901, 11, 48, 8, 8);
  fixture.stack_layout.frame_slots.push_back(frame_slot(fixture, 11, 48, 8, 8));

  const auto missing_home =
      prepare::classify_prepared_object_frame_slot_consumer(
          prepare::PreparedObjectFrameSlotConsumerQuery{
              .stack_layout = &fixture.stack_layout});

  auto reg_home =
      value_home(fixture, "%reg", 902, prepare::PreparedValueHomeKind::Register);
  reg_home.register_name = "r.prepared";
  const auto unsupported = prepare::classify_prepared_object_frame_slot_consumer(
      fixture.stack_layout, reg_home);

  auto incomplete_home =
      stack_slot_value_home(fixture, "%incomplete.stack", 903, 12, 64, 4, 4);
  incomplete_home.slot_id = std::nullopt;
  const auto incomplete = prepare::classify_prepared_object_frame_slot_consumer(
      fixture.stack_layout, incomplete_home);

  const auto missing_layout =
      prepare::classify_prepared_object_frame_slot_consumer(
          prepare::PreparedObjectFrameSlotConsumerQuery{.value_home = &available_home});

  auto missing_owner_fixture = make_fixture();
  const auto missing_owner_home =
      stack_slot_value_home(missing_owner_fixture, "%missing.owner", 904, 13, 80, 4, 4);
  const auto missing_owner = prepare::classify_prepared_object_frame_slot_consumer(
      missing_owner_fixture.stack_layout, missing_owner_home);

  auto ambiguous_fixture = make_fixture();
  ambiguous_fixture.stack_layout.frame_slots.push_back(
      frame_slot(ambiguous_fixture, 14, 96, 4, 4));
  ambiguous_fixture.stack_layout.frame_slots.push_back(
      frame_slot(ambiguous_fixture, 14, 96, 4, 4));
  const auto ambiguous_home =
      stack_slot_value_home(ambiguous_fixture, "%ambiguous.owner", 905, 14, 96, 4, 4);
  const auto ambiguous = prepare::classify_prepared_object_frame_slot_consumer(
      ambiguous_fixture.stack_layout, ambiguous_home);

  auto mismatched_function_fixture = make_fixture();
  mismatched_function_fixture.stack_layout.frame_slots.push_back(
      frame_slot(mismatched_function_fixture, 15, 112, 8, 8));
  mismatched_function_fixture.stack_layout.frame_slots.front().function_name = 999;
  const auto mismatched_function_home = stack_slot_value_home(
      mismatched_function_fixture, "%mismatched.function", 906, 15, 112, 8, 8);
  const auto mismatched_function =
      prepare::classify_prepared_object_frame_slot_consumer(
          mismatched_function_fixture.stack_layout, mismatched_function_home);

  auto mismatched_offset_fixture = make_fixture();
  mismatched_offset_fixture.stack_layout.frame_slots.push_back(
      frame_slot(mismatched_offset_fixture, 16, 128, 8, 8));
  const auto mismatched_offset_home = stack_slot_value_home(
      mismatched_offset_fixture, "%mismatched.offset", 907, 16, 136, 8, 8);
  const auto mismatched_offset =
      prepare::classify_prepared_object_frame_slot_consumer(
          mismatched_offset_fixture.stack_layout, mismatched_offset_home);

  auto mismatched_size_fixture = make_fixture();
  mismatched_size_fixture.stack_layout.frame_slots.push_back(
      frame_slot(mismatched_size_fixture, 17, 144, 8, 8));
  const auto mismatched_size_home = stack_slot_value_home(
      mismatched_size_fixture, "%mismatched.size", 908, 17, 144, 4, 8);
  const auto mismatched_size = prepare::classify_prepared_object_frame_slot_consumer(
      mismatched_size_fixture.stack_layout, mismatched_size_home);

  auto mismatched_align_fixture = make_fixture();
  mismatched_align_fixture.stack_layout.frame_slots.push_back(
      frame_slot(mismatched_align_fixture, 18, 160, 8, 8));
  const auto mismatched_align_home = stack_slot_value_home(
      mismatched_align_fixture, "%mismatched.align", 909, 18, 160, 8, 4);
  const auto mismatched_align = prepare::classify_prepared_object_frame_slot_consumer(
      mismatched_align_fixture.stack_layout, mismatched_align_home);

  if (!expect(missing_home.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::MissingValueHome,
              "frame-slot ownership query should require a prepared value home") ||
      !expect(unsupported.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      UnsupportedValueHomeKind,
              "non-stack homes should not classify as frame-slot consumers") ||
      !expect(incomplete.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      IncompleteStackSlotHome,
              "stack homes without slot layout facts should fail closed") ||
      !expect(missing_layout.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::MissingStackLayout,
              "frame-slot ownership query should require prepared stack layout") ||
      !expect(missing_owner.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      MissingFrameSlotOwner,
              "stack homes without a prepared frame-slot owner should fail closed") ||
      !expect(ambiguous.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      AmbiguousFrameSlotOwner,
              "duplicate frame-slot owners should fail closed") ||
      !expect(mismatched_function.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      MismatchedFrameSlotFunction,
              "frame-slot owner from another function should fail closed") ||
      !expect(mismatched_offset.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      MismatchedFrameSlotOffset,
              "frame-slot owner with a different offset should fail closed") ||
      !expect(mismatched_size.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      MismatchedFrameSlotSize,
              "frame-slot owner with a different size should fail closed") ||
      !expect(mismatched_align.status ==
                  prepare::PreparedObjectFrameSlotConsumerStatus::
                      MismatchedFrameSlotAlignment,
              "frame-slot owner with a different alignment should fail closed")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const auto result = verify_event_kind_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_select_consumer_kind_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_value_home_consumer_status_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_move_bundle_consumer_status_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_frame_slot_consumer_status_names(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_canonical_consumer_schedule(); result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_edge_copy_execution_site_placement();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result =
          verify_real_select_vs_required_missing_carrier_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result =
          verify_select_materialized_join_transfer_carrier_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result =
          verify_ambiguous_and_unsupported_select_carrier_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_supported_value_home_consumer_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result =
          verify_value_home_consumer_missing_and_unsupported_statuses();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_frame_slot_consumer_available_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_frame_slot_consumer_fail_closed_statuses();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result = verify_move_bundle_consumer_available_classification();
      result != 0) {
    return EXIT_FAILURE;
  }
  if (const auto result =
          verify_move_bundle_consumer_fail_closed_statuses();
      result != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
