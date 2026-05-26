#include "src/backend/prealloc/module.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedControlFlowBlock return_block(c4c::BlockLabelId label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Return,
  };
}

prepare::PreparedControlFlowBlock branch_block(c4c::BlockLabelId label,
                                               c4c::BlockLabelId target) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = target,
  };
}

prepare::PreparedControlFlowBlock cond_branch_block(c4c::BlockLabelId label,
                                                    c4c::BlockLabelId true_label,
                                                    c4c::BlockLabelId false_label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
}

bool expect_same(const void* actual, const void* expected, std::string_view message) {
  if (actual != expected) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

int verify_linear_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("linear");
  const auto other_function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("linear.entry");
  const auto exit_label = prepared.names.block_labels.intern("linear.exit");
  const auto value_name = prepared.names.value_names.intern("%linear.value");
  const auto other_value_name = prepared.names.value_names.intern("%diamond.value");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          branch_block(entry_label, exit_label),
          return_block(exit_label),
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("callee_a"),
              .preserved_values = {
                  prepare::PreparedCallPreservedValue{
                      .value_id = 4,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
                      .contiguous_width = 1,
                      .register_name = std::string("x19"),
                      .register_bank = prepare::PreparedRegisterBank::Gpr,
                      .occupied_register_names = {"x19"},
                      .register_placement = prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 9,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{3},
                      .stack_offset_bytes = std::size_t{24},
                      .stack_size_bytes = std::size_t{8},
                      .stack_align_bytes = std::size_t{8},
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 9,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{4},
                      .stack_offset_bytes = std::size_t{32},
                      .stack_size_bytes = std::size_t{8},
                      .stack_align_bytes = std::size_t{8},
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 10,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
                  },
              },
          },
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
              .direct_callee_name = std::string("callee_b"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = other_function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
          },
      },
  });

  prepare::PreparedAddressingFunction addressing{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 1,
              .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 3,
              .kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = exit_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::Label,
          },
      },
  };
  prepared.addressing.functions.push_back(addressing);
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = other_function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = other_function_id,
              .block_label = entry_label,
              .inst_index = 9,
              .kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          },
      },
  });

  prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("r10"),
          },
          prepare::PreparedValueHome{
              .value_id = 8,
              .function_name = function_id,
              .value_name = c4c::kInvalidValueName,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 1,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeInstruction,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 0,
              .instruction_index = 2,
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::AfterCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 1,
              .instruction_index = 0,
          },
      },
  };
  prepared.value_locations.functions.push_back(value_locations);
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = other_function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = other_function_id,
              .value_name = other_value_name,
              .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
              .immediate_i32 = 7,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for linear function");
  }

  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 0, 2),
                   &call_plans->calls[0],
                   "indexed call-plan lookup missed the first linear call")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 1, 0),
                   &call_plans->calls[1],
                   "indexed call-plan lookup missed the second linear call")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_call_plan(&lookups.call_plans, call_plans, 0, 9) !=
      nullptr) {
    return fail("indexed call-plan lookup returned an unmatched linear call");
  }
  const prepare::PreparedCallPlan same_block_current{
      .block_index = 0,
      .instruction_index = 4,
  };
  const auto same_block_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, same_block_current, prepare::PreparedValueId{4});
  if (same_block_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::Found ||
      same_block_prior.preserved != &call_plans->calls[0].preserved_values[0] ||
      same_block_prior.entry == nullptr ||
      same_block_prior.entry->block_index != 0 ||
      same_block_prior.entry->instruction_index != 2) {
    return fail("unique prior-preservation lookup missed same-block indexed source");
  }
  const auto cross_block_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{4});
  if (cross_block_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::Found ||
      cross_block_prior.preserved != &call_plans->calls[0].preserved_values[0] ||
      cross_block_prior.entry == nullptr ||
      cross_block_prior.entry->block_index != 0 ||
      cross_block_prior.entry->instruction_index != 2) {
    return fail("unique prior-preservation lookup missed cross-block dominating source");
  }
  const auto no_prior = prepare::find_unique_indexed_prior_preserved_value_source(
      lookups.call_plans, &control_flow, same_block_current, prepare::PreparedValueId{88});
  if (no_prior.status != prepare::PreparedPriorPreservedValueLookupStatus::NotFound ||
      no_prior.preserved != nullptr || no_prior.entry != nullptr) {
    return fail("unique prior-preservation lookup did not explicitly reject no-source case");
  }
  const auto ambiguous_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{9});
  if (ambiguous_prior.status !=
      prepare::PreparedPriorPreservedValueLookupStatus::Ambiguous) {
    return fail("unique prior-preservation lookup did not explicitly reject ambiguity");
  }
  const auto invalid_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{10});
  if (invalid_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::InvalidPreservation ||
      invalid_prior.preserved != &call_plans->calls[0].preserved_values[3]) {
    return fail("unique prior-preservation lookup did not explicitly reject incomplete source");
  }

  const auto* entry_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, entry_label);
  if (entry_materializations == nullptr || entry_materializations->size() != 2 ||
      (*entry_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0] ||
      (*entry_materializations)[1] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup did not preserve block grouping");
  }
  const auto* exit_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, exit_label);
  if (exit_materializations == nullptr || exit_materializations->size() != 1 ||
      (*exit_materializations)[0] != &prepared.addressing.functions[0].address_materializations[2]) {
    return fail("indexed address-materialization lookup missed exit block materialization");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeInstruction,
                       0,
                       2),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed the before-instruction bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::AfterCall,
                       1,
                       0),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed the after-call bundle")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_move_bundle(&lookups.move_bundles,
                                                 function_locations,
                                                 prepare::PreparedMovePhase::BeforeCall,
                                                 0,
                                                 2) != nullptr) {
    return fail("indexed move-bundle lookup ignored move phase");
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{4}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-id home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 4) {
    return fail("indexed value-id lookup missed the value-name mapping");
  }
  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, nullptr, function_locations, value_name),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-name home")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_value_id(
          &lookups.value_homes, nullptr, function_locations, other_value_name)
      .has_value()) {
    return fail("indexed value-id lookup leaked another function's value home");
  }

  return 0;
}

int verify_diamond_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("diamond.entry");
  const auto left_label = prepared.names.block_labels.intern("diamond.left");
  const auto right_label = prepared.names.block_labels.intern("diamond.right");
  const auto join_label = prepared.names.block_labels.intern("diamond.join");
  const auto value_name = prepared.names.value_names.intern("%diamond.value");
  const auto left_source_name = prepared.names.value_names.intern("%diamond.left");
  const auto right_source_name = prepared.names.value_names.intern("%diamond.right");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          cond_branch_block(entry_label, left_label, right_label),
          branch_block(left_label, join_label),
          branch_block(right_label, join_label),
          return_block(join_label),
      },
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_id,
              .join_block_label = join_label,
              .result = bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .carrier_kind =
                  prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = left_label,
                      .successor_label = join_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.left"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = right_label,
                      .successor_label = join_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.right"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
                  },
              },
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = left_label,
              .successor_label = join_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          },
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = right_label,
              .successor_label = join_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          },
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("left_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 2,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("right_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 3,
              .instruction_index = 1,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternVariadic,
              .direct_callee_name = std::string("join_call"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);

  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = left_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = right_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
          },
      },
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 11,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              .pointer_byte_delta = 16,
          },
          prepare::PreparedValueHome{
              .value_id = 12,
              .function_name = function_id,
              .value_name = left_source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("left_source"),
          },
          prepare::PreparedValueHome{
              .value_id = 13,
              .function_name = function_id,
              .value_name = right_source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("right_source"),
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 3,
              .instruction_index = 0,
              .source_parallel_copy_predecessor_label = left_label,
              .source_parallel_copy_successor_label = join_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = 12,
                      .to_value_id = 11,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_stack_offset_bytes = std::size_t{16},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 3,
              .instruction_index = 1,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for diamond function");
  }

  for (std::size_t index = 0; index < call_plans->calls.size(); ++index) {
    const auto& call = call_plans->calls[index];
    if (!expect_same(prepare::find_indexed_prepared_call_plan(
                         &lookups.call_plans,
                         call_plans,
                         call.block_index,
                         call.instruction_index),
                     &call_plans->calls[index],
                     "indexed call-plan lookup missed a diamond call")) {
      return 1;
    }
  }

  const auto* left_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, left_label);
  const auto* right_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, right_label);
  if (left_materializations == nullptr || left_materializations->size() != 1 ||
      (*left_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0]) {
    return fail("indexed address-materialization lookup missed diamond left block");
  }
  if (right_materializations == nullptr || right_materializations->size() != 1 ||
      (*right_materializations)[0] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup missed diamond right block");
  }
  if (prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, join_label) != nullptr) {
    return fail("indexed address-materialization lookup returned an empty diamond join block");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BlockEntry,
                       3,
                       0),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed diamond block-entry bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeCall,
                       3,
                       1),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed diamond before-call bundle")) {
    return 1;
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{11}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed diamond home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 11) {
    return fail("indexed value-id lookup missed diamond value-name mapping");
  }

  const auto* left_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, left_label, join_label, prepare::PreparedValueId{11});
  if (left_publication == nullptr ||
      left_publication->status !=
          prepare::PreparedEdgePublicationLookupStatus::Available ||
      left_publication->predecessor_label != left_label ||
      left_publication->successor_label != join_label ||
      left_publication->destination_value_id != 11 ||
      left_publication->destination_value_name != value_name ||
      !left_publication->source_value_id.has_value() ||
      *left_publication->source_value_id != 12 ||
      left_publication->source_value_name != left_source_name ||
      left_publication->destination_home != &function_locations->value_homes[0] ||
      left_publication->destination_home_kind !=
          prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      left_publication->destination_storage_kind !=
          prepare::PreparedMoveStorageKind::StackSlot ||
      left_publication->phase != prepare::PreparedMovePhase::BlockEntry ||
      left_publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      left_publication->join_transfer != &control_flow.join_transfers[0] ||
      left_publication->edge_transfer !=
          &control_flow.join_transfers[0].edge_transfers[0] ||
      left_publication->parallel_copy_bundle !=
          &control_flow.parallel_copy_bundles[0] ||
      left_publication->move_bundle != &function_locations->move_bundles[0] ||
      left_publication->move != &function_locations->move_bundles[0].moves[0]) {
    return fail("edge-publication lookup did not preserve prepared join, source, home, carrier, and move facts");
  }

  const auto* right_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, right_label, join_label, prepare::PreparedValueId{11});
  if (right_publication == nullptr ||
      right_publication->source_value_id != prepare::PreparedValueId{13} ||
      right_publication->parallel_copy_bundle !=
          &control_flow.parallel_copy_bundles[1] ||
      right_publication->move != nullptr ||
      right_publication->destination_storage_kind != prepare::PreparedMoveStorageKind::None) {
    return fail("edge-publication lookup should link the published bundle while leaving missing move data absent");
  }

  return 0;
}

int verify_edge_publication_lookup_key_preserves_full_tuple() {
  const auto successor_destination_a = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{10},
      c4c::BlockLabelId{20},
      prepare::PreparedValueId{(std::size_t{1} << 16U) ^ 100U});
  const auto successor_destination_b = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{10},
      c4c::BlockLabelId{21},
      prepare::PreparedValueId{100});
  if (successor_destination_a == successor_destination_b) {
    return fail("edge-publication key collapsed overlapping successor/value-id bits");
  }

  const auto predecessor_successor_a = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{42}, c4c::BlockLabelId{5}, prepare::PreparedValueId{7});
  const auto predecessor_successor_b = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{43},
      static_cast<c4c::BlockLabelId>((std::size_t{1} << 24U) + 5U),
      prepare::PreparedValueId{7});
  if (predecessor_successor_a == predecessor_successor_b) {
    return fail("edge-publication key collapsed overlapping predecessor/successor bits");
  }

  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("edge_key_growth");
  const auto source_name = names.value_names.intern("%edge_key.source");
  const auto high_destination_name = names.value_names.intern("%edge_key.high_destination");
  const auto low_destination_name = names.value_names.intern("%edge_key.low_destination");

  const c4c::BlockLabelId predecessor_label{10};
  const c4c::BlockLabelId first_successor_label{20};
  const c4c::BlockLabelId second_successor_label{21};
  const prepare::PreparedValueId source_value_id{50};
  const prepare::PreparedValueId high_destination_value_id{(std::size_t{1} << 16U) ^
                                                           100U};
  const prepare::PreparedValueId low_destination_value_id{100};

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = first_successor_label,
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = first_successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_key.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_key.high_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = second_successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_key.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_key.low_destination"),
                  },
              },
          },
      },
  };

  const prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"source_home"},
          },
          prepare::PreparedValueHome{
              .value_id = high_destination_value_id,
              .function_name = function_name,
              .value_name = high_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"high_home"},
          },
          prepare::PreparedValueHome{
              .value_id = low_destination_value_id,
              .function_name = function_name,
              .value_name = low_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"low_home"},
          },
      },
  };

  const auto lookups = prepare::make_prepared_edge_publication_lookups(
      names, control_flow, &locations);
  const auto* high_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups,
          predecessor_label,
          first_successor_label,
          high_destination_value_id);
  const auto* low_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, second_successor_label, low_destination_value_id);

  if (high_publication == nullptr || low_publication == nullptr) {
    return fail("edge-publication lookup should preserve distinct high-id tuples");
  }
  if (high_publication == low_publication ||
      high_publication->successor_label != first_successor_label ||
      high_publication->destination_value_id != high_destination_value_id ||
      low_publication->successor_label != second_successor_label ||
      low_publication->destination_value_id != low_destination_value_id) {
    return fail("edge-publication lookup returned a colliding edge/destination record");
  }
  if (prepare::find_unique_indexed_prepared_edge_publication(
          &lookups, predecessor_label, first_successor_label, low_destination_value_id) !=
      nullptr) {
    return fail("edge-publication lookup should not cross-match successor/value-id tuples");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = verify_linear_function_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_diamond_function_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_edge_publication_lookup_key_preserves_full_tuple();
      result != 0) {
    return result;
  }
  return EXIT_SUCCESS;
}
