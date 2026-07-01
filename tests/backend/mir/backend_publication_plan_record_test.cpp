#include "src/backend/prealloc/publication_plans.hpp"
#include "src/backend/prealloc/module.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedValueHome home_with_kind(
    prepare::PreparedValueHomeKind kind,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = 11,
      .value_name = value_name,
      .kind = kind,
  };
}

int records_register_home_intent() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::Register, 1, 101);
  home.register_name = "target-register-token";
  const auto source = bir::Value::named(bir::TypeKind::I64, "%reg_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind != prepare::PreparedScalarPublicationHookKind::RegisterHome ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::Register ||
      plan.destination_home_kind != prepare::PreparedValueHomeKind::Register ||
      plan.source_value_id != 1 || plan.source_value_name != 101 ||
      plan.source_value.name != "%reg_source") {
    return fail("expected register-home publication intent");
  }
  return 0;
}

int records_stack_slot_home_intent() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::StackSlot, 2, 102);
  home.slot_id = 7;
  home.offset_bytes = 32;
  home.size_bytes = 8;
  home.align_bytes = 8;
  const auto source = bir::Value::named(bir::TypeKind::I64, "%stack_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind != prepare::PreparedScalarPublicationHookKind::StackSlotHome ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      plan.slot_id != std::optional<prepare::PreparedFrameSlotId>{7} ||
      plan.stack_offset_bytes != std::optional<std::size_t>{32} ||
      plan.size_bytes != std::optional<std::size_t>{8} ||
      plan.align_bytes != std::optional<std::size_t>{8}) {
    return fail("expected stack-slot publication intent");
  }
  return 0;
}

int records_rematerializable_immediate_intent() {
  auto home = home_with_kind(
      prepare::PreparedValueHomeKind::RematerializableImmediate, 3, 103);
  home.immediate_i32 = -42;
  const auto source = bir::Value::named(bir::TypeKind::I32, "%imm_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind !=
          prepare::PreparedScalarPublicationHookKind::RematerializableImmediate ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      plan.immediate_i32 != std::optional<std::int64_t>{-42}) {
    return fail("expected rematerializable-immediate publication intent");
  }
  return 0;
}

int records_pointer_base_plus_offset_intent() {
  auto home = home_with_kind(
      prepare::PreparedValueHomeKind::PointerBasePlusOffset, 4, 104);
  home.pointer_base_value_name = 204;
  home.pointer_byte_delta = 24;
  const auto source = bir::Value::named(bir::TypeKind::Ptr, "%derived_pointer");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind !=
          prepare::PreparedScalarPublicationHookKind::PointerBasePlusOffset ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::ComputedAddress ||
      plan.pointer_base_value_name != std::optional<c4c::ValueNameId>{204} ||
      plan.pointer_byte_delta != std::optional<std::int64_t>{24}) {
    return fail("expected pointer-base-plus-offset publication intent");
  }
  return 0;
}

int records_shape_without_aarch64_policy() {
  if (prepare::prepared_scalar_publication_hook_kind_name(
          prepare::PreparedScalarPublicationHookKind::RegisterHome) !=
      "register_home") {
    return fail("expected target-neutral hook spelling");
  }
  if (prepare::prepared_scalar_publication_status_name(
          prepare::PreparedScalarPublicationStatus::MissingPointerDelta) !=
      "missing_pointer_delta") {
    return fail("expected target-neutral status spelling");
  }
  if (prepare::prepared_publication_storage_encoding_from_home(
          prepare::PreparedValueHomeKind::PointerBasePlusOffset) !=
      prepare::PreparedStorageEncodingKind::ComputedAddress) {
    return fail("expected pointer homes to map to computed-address storage");
  }
  return 0;
}

bir::LocalArrayElementPathRecord effect_stream_path() {
  return bir::LocalArrayElementPathRecord{
      .result_name = "%elt.ptr",
      .source_object_name = "%lv.arr",
      .derivation_result_name = "%elt.ptr",
      .indices = {bir::LocalArrayIndexRecord{
          .kind = bir::LocalArrayIndexKind::Dynamic,
          .value = bir::Value::named(bir::TypeKind::I64, "%idx"),
      }},
      .element_type = bir::TypeKind::I32,
      .element_size_bytes = 4,
      .byte_offset = 0,
      .element_count = 4,
      .scalar_in_bounds = true,
      .status = bir::LocalArrayCarrierStatus::MissingIndexRangeProof,
      .lir_producer_function_name = "stream_fixture",
      .lir_producer_block_label = "body",
      .lir_producer_instruction_index = std::size_t{2},
      .lir_producer_operation_role =
          bir::LocalArrayLirProducerOperationRole::AddressDerivation,
      .lir_producer_lookup_key =
          "lir-producer:stream_fixture:body:2:%elt.ptr:%lv.arr:%elt.ptr:%idx",
      .lir_producer_coordinate_status =
          bir::LocalArrayLirProducerCoordinateStatus::Available,
  };
}

bir::LocalArraySelectedProofEdgePathRecord effect_stream_selected_path(
    const bir::LocalArrayElementPathRecord* path) {
  return bir::evaluate_local_array_selected_proof_edge_path(
      bir::LocalArraySelectedProofEdgePathInputs{
          .element_path = path,
          .proof_function_name = "stream_fixture",
          .proof_block_label = "guard",
          .proof_condition_value = bir::Value::named(bir::TypeKind::I1, "%cmp"),
          .proof_source_available = true,
          .proof_predicate = bir::LocalArraySelectedProofEdgePredicate::Ult,
          .proof_compare_type = bir::TypeKind::I64,
          .proof_lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .proof_rhs = bir::Value::immediate_i64(4),
          .proof_instruction_index = std::size_t{0},
          .bound_contribution =
              bir::LocalArraySelectedProofEdgeBoundContribution::Upper,
          .normalized_bound = std::int64_t{4},
          .bound_inclusive = false,
          .selected_outcome = bir::LocalArraySelectedProofEdgeOutcome::True,
          .selected_successor_label = "body",
          .non_selected_successor_label = "exit",
          .path_validity_known = true,
          .selected_edge_reaches_lir_producer = true,
          .selected_edge_covers_lir_producer = true,
          .proof_dominates_lir_producer = true,
      });
}

bir::LocalArrayEndpointBridgeRecord effect_stream_endpoint_bridge(
    const bir::LocalArrayElementPathRecord* path) {
  return bir::evaluate_local_array_endpoint_bridge(
      bir::LocalArrayEndpointBridgeInputs{
          .element_path = path,
          .endpoint_available = true,
          .prepared_function_name = "stream_fixture",
          .prepared_block_label = "body",
          .prepared_block_index = std::size_t{1},
          .bir_block_label = "body",
          .endpoint_instruction_index = std::size_t{2},
          .address_materialization_kind = "frame_slot",
          .result_value_name = "%elt.ptr",
          .matched_source_object_name = "%lv.arr",
          .matched_derivation_result_name = "%elt.ptr",
      });
}

bir::LoadLocalInst effect_stream_scalar_load(
    std::size_t load_byte_offset = 0,
    std::int64_t address_byte_offset = 0) {
  return bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .slot_name = "%lv.arr",
      .byte_offset = load_byte_offset,
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%elt.ptr",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .byte_offset = address_byte_offset,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
}

prepare::PreparedBirModule make_effect_stream_prepared_module(
    std::vector<bir::Inst> body_insts) {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("stream_fixture");
  const auto guard_label = prepared.names.block_labels.intern("guard");
  const auto body_label = prepared.names.block_labels.intern("body");
  const auto exit_label = prepared.names.block_labels.intern("exit");
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {
          prepare::PreparedControlFlowBlock{
              .block_label = guard_label,
              .terminator_kind = bir::TerminatorKind::CondBranch,
              .true_label = body_label,
              .false_label = exit_label,
          },
          prepare::PreparedControlFlowBlock{.block_label = body_label},
          prepare::PreparedControlFlowBlock{.block_label = exit_label},
      },
      .branch_conditions = {
          prepare::PreparedBranchCondition{
              .function_name = function_name,
              .block_label = guard_label,
              .kind = prepare::PreparedBranchConditionKind::FusedCompare,
              .condition_value = bir::Value::named(bir::TypeKind::I1, "%cmp"),
              .predicate = bir::BinaryOpcode::Ult,
              .compare_type = bir::TypeKind::I64,
              .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
              .rhs = bir::Value::immediate_i64(4),
              .can_fuse_with_branch = true,
              .true_label = body_label,
              .false_label = exit_label,
          },
      },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "stream_fixture",
      .local_array_element_paths = {effect_stream_path()},
      .blocks = {
          bir::Block{
              .label = "guard",
              .insts = {bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Ult,
                  .result = bir::Value::named(bir::TypeKind::I1, "%cmp"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
                  .rhs = bir::Value::immediate_i64(4),
              }},
              .label_id = guard_label,
          },
          bir::Block{
              .label = "body",
              .insts = std::move(body_insts),
              .label_id = body_label,
          },
          bir::Block{
              .label = "exit",
              .label_id = exit_label,
          },
      },
  });
  auto& function = prepared.module.functions.front();
  const auto& path = function.local_array_element_paths.front();
  function.local_array_source_objects.push_back(bir::LocalArraySourceObjectRecord{
      .object_name = "%lv.arr",
      .element_type = bir::TypeKind::I32,
      .type_text = "[4 x i32]",
      .element_count = 4,
      .element_size_bytes = 4,
      .total_size_bytes = 16,
      .align_bytes = 4,
      .element_slots = {"%lv.arr.0", "%lv.arr.1", "%lv.arr.2", "%lv.arr.3"},
      .status = bir::LocalArrayCarrierStatus::Available,
  });
  function.local_array_derivations.push_back(
      bir::LocalArrayAddressDerivationRecord{
          .result_name = "%elt.ptr",
          .source_object_name = "%lv.arr",
          .base_view_name = "%lv.arr",
          .kind = bir::LocalArrayDerivationKind::LocalAddressOfElement,
          .status = bir::LocalArrayCarrierStatus::Available,
      });
  function.local_array_selected_proof_edge_paths.push_back(
      effect_stream_selected_path(&path));
  function.local_array_endpoint_bridges.push_back(
      effect_stream_endpoint_bridge(&path));
  return prepared;
}

int production_publish_contract_plans_populates_local_array_interval_effects() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
      effect_stream_scalar_load(),
  });
  auto& function = prepared.module.functions.front();
  function.local_array_selected_proof_edge_paths.clear();
  function.local_array_endpoint_bridges.clear();
  function.local_array_ordered_effect_source_streams.clear();
  function.local_array_interval_effects.clear();

  prepare::BirPreAlloc prealloc(std::move(prepared));
  prealloc.publish_contract_plans();
  const auto& published_function = prealloc.prepared().module.functions.front();
  if (published_function.local_array_selected_proof_edge_paths.size() != 1 ||
      published_function.local_array_selected_proof_edge_paths.front().status !=
          bir::LocalArraySelectedProofEdgePathStatus::Available) {
    return fail("expected production publication to rebuild selected proof-edge path");
  }
  if (published_function.local_array_endpoint_bridges.size() != 1 ||
      published_function.local_array_endpoint_bridges.front().status !=
          bir::LocalArrayEndpointBridgeStatus::Available) {
    return fail("expected production publication to rebuild endpoint bridge");
  }
  if (published_function.local_array_ordered_effect_source_streams.size() != 1 ||
      published_function.local_array_ordered_effect_source_streams.front().status !=
          bir::LocalArrayOrderedEffectSourceStreamStatus::MissingLowerBoundaryCoordinate) {
    return fail("expected production publication to build ordered stream before interval fact");
  }
  if (published_function.local_array_interval_effects.size() != 1 ||
      published_function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate) {
    return fail("expected production publication to populate interval effect fact");
  }
  if (published_function.local_array_index_range_proofs.size() != 1 ||
      published_function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingLirProducerCoordinate) {
    return fail("expected production publication to populate fail-closed range proof certificate");
  }
  if (published_function.local_array_proof_facts.size() != 1 ||
      published_function.local_array_proof_facts.front().status !=
          bir::LocalArrayRangeProofStatus::MissingLirProducerCoordinate) {
    return fail("expected production publication to populate fail-closed proof fact");
  }
  if (published_function.local_array_index_range_checker_inputs.size() != 1 ||
      published_function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingLirProducerCoordinate) {
    return fail("expected production publication to populate fail-closed checker input");
  }
  if (published_function.local_array_local_address_provenances.size() != 1 ||
      published_function.local_array_local_address_provenances.front().status !=
          bir::LocalArrayCarrierStatus::MissingIndexRangeProof ||
      published_function.local_array_local_address_provenances.front()
              .checker_status !=
          bir::LocalArrayRangeProofStatus::MissingLirProducerCoordinate) {
    return fail("expected production publication to populate fail-closed local-address provenance");
  }
  return 0;
}

int populates_clean_local_array_ordered_effect_stream() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
      effect_stream_scalar_load(),
  });

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& streams =
      prepared.module.functions.front().local_array_ordered_effect_source_streams;
  if (streams.size() != 1 ||
      streams.front().status !=
          bir::LocalArrayOrderedEffectSourceStreamStatus::Available) {
    return fail("expected one available local-array ordered effect stream");
  }
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status != bir::LocalArrayIntervalEffectStatus::Available) {
    return fail("expected clean production stream to classify as available");
  }
  prepare::populate_local_array_interval_effects(prepared);
  const auto& facts = function.local_array_interval_effects;
  if (facts.size() != 1 ||
      facts.front().status != bir::LocalArrayIntervalEffectStatus::Available ||
      facts.front().lir_producer_lookup_key !=
          function.local_array_selected_proof_edge_paths.front()
              .lir_producer_lookup_key ||
      facts.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx")) {
    return fail("expected stored production stream to publish one available interval fact");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::Available ||
      function.local_array_index_range_proofs.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx")) {
    return fail("expected stored lower authorities to publish one available range proof certificate");
  }
  prepare::populate_local_array_proof_facts(prepared);
  if (function.local_array_proof_facts.size() != 1 ||
      function.local_array_proof_facts.front().status !=
          bir::LocalArrayRangeProofStatus::Available ||
      function.local_array_proof_facts.front().range_proof !=
          &function.local_array_index_range_proofs.front() ||
      function.local_array_proof_facts.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx")) {
    return fail("expected production range certificate to publish one available proof fact");
  }
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  if (function.local_array_index_range_checker_inputs.size() != 1 ||
      function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::Available ||
      function.local_array_index_range_checker_inputs.front().proof_fact !=
          &function.local_array_proof_facts.front() ||
      function.local_array_index_range_checker_inputs.front()
              .checker_record.status !=
          bir::LocalArrayRangeProofStatus::Available) {
    return fail("expected production proof fact to publish one available checker input");
  }
  prepare::populate_local_array_local_address_provenances(prepared);
  if (function.local_array_local_address_provenances.size() != 1 ||
      function.local_array_local_address_provenances.front().status !=
          bir::LocalArrayCarrierStatus::Available ||
      function.local_array_local_address_provenances.front().checker_input !=
          &function.local_array_index_range_checker_inputs.front() ||
      function.local_array_local_address_provenances.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx") ||
      function.local_array_local_address_provenances.front()
              .source_object_name != "%lv.arr") {
    return fail("expected production checker input to publish available local-address provenance");
  }
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_semantic_geps.size() != 1 ||
      function.local_array_semantic_geps.front().status !=
          bir::LocalArrayCarrierStatus::Available ||
      function.local_array_semantic_geps.front().provenance !=
          &function.local_array_local_address_provenances.front() ||
      function.local_array_semantic_geps.front().source_object_name !=
          "%lv.arr" ||
      function.local_array_semantic_geps.front().element_result_name !=
          "%elt.ptr" ||
      function.local_array_semantic_geps.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx") ||
      function.local_array_semantic_geps.front()
              .lir_producer_operation_role !=
          bir::LocalArrayLirProducerOperationRole::AddressDerivation) {
    return fail("expected production local-address provenance to publish available semantic GEP admission");
  }
  if (function.local_array_scalar_local_loads.empty()) {
    return fail("expected production scalar load scan to find pointer-addressed load");
  }
  if (function.local_array_scalar_local_loads.size() != 1) {
    return fail("expected production local-address provenance to publish one scalar load fact");
  }
  if (function.local_array_scalar_local_loads.front().status !=
      bir::LocalArrayScalarLocalLoadStatus::Available) {
    return fail("expected production scalar load fact to be available");
  }
  if (function.local_array_scalar_local_loads.front().provenance !=
      &function.local_array_local_address_provenances.front()) {
    return fail("expected production scalar load fact to consume local-address provenance");
  }
  if (function.local_array_scalar_local_loads.front().load_result_name !=
          "%loaded" ||
      function.local_array_scalar_local_loads.front().dynamic_index !=
          bir::Value::named(bir::TypeKind::I64, "%idx")) {
    return fail("expected production scalar load fact to package load identity");
  }
  return 0;
}

int local_array_semantic_gep_population_requires_matching_provenance() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (!function.local_array_semantic_geps.empty()) {
    return fail("expected semantic GEP publication to require lower provenance records");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  prepare::populate_local_array_interval_effects(prepared);
  prepare::populate_local_array_index_range_proofs(prepared);
  prepare::populate_local_array_proof_facts(prepared);
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  prepare::populate_local_array_local_address_provenances(prepared);
  function.local_array_local_address_provenances.front().status =
      bir::LocalArrayCarrierStatus::GlobalSourceObject;
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_semantic_geps.size() != 1 ||
      function.local_array_semantic_geps.front().status !=
          bir::LocalArrayCarrierStatus::GlobalSourceObject) {
    return fail("expected semantic GEP publication to preserve global/static boundary status");
  }

  prepare::populate_local_array_local_address_provenances(prepared);
  auto duplicate = function.local_array_local_address_provenances.front();
  function.local_array_local_address_provenances.push_back(duplicate);
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_semantic_geps.size() != 2 ||
      function.local_array_semantic_geps.front().status !=
          bir::LocalArrayCarrierStatus::PreparedBirCoordinateConfusion ||
      function.local_array_semantic_geps.back().status !=
          bir::LocalArrayCarrierStatus::PreparedBirCoordinateConfusion) {
    return fail("expected semantic GEP publication to reject duplicate provenance identities");
  }

  return 0;
}

bir::GlobalStaticGepAuthorityRecord make_available_global_static_gep_authority() {
  return bir::GlobalStaticGepAuthorityRecord{
      .status = bir::GlobalStaticGepAuthorityStatus::Available,
      .derivation_kind = bir::GlobalStaticGepDerivationKind::DirectGlobal,
      .global_name = "numbers",
      .global_link_name_id = c4c::LinkNameId{42},
      .result_name = "%elt",
      .base_pointer_name = "@numbers",
      .source_type_text = "[4 x i32]",
      .layout_path_type_text = "[4 x i32]",
      .source_size_bytes = 16,
      .byte_offset = 8,
      .element_type = bir::TypeKind::I32,
      .element_type_text = "i32",
      .element_size_bytes = 4,
      .element_count = 1,
      .element_stride_bytes = 4,
      .has_constant_range = true,
      .has_dynamic_range = false,
      .layout_authority = bir::MemoryLayoutAuthorityKind::StructuredLayout,
      .range_verdict = bir::MemoryRangeVerdict::ProvenInBounds,
      .coordinate_status = bir::GlobalStaticGepCoordinateStatus::Available,
      .lir_producer_function_name = "global_fixture",
      .lir_producer_block_label = "entry",
      .lir_producer_instruction_index = std::size_t{0},
      .lir_producer_lookup_key = "global_fixture:entry:0:numbers:%elt:8",
  };
}

int global_static_semantic_gep_population_requires_matching_authority() {
  if (bir::evaluate_global_static_semantic_gep(
          bir::GlobalStaticSemanticGepInputs{})
          .status !=
      bir::GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject) {
    return fail("expected global/static semantic GEP admission to reject missing authority");
  }

  prepare::PreparedBirModule prepared;
  prepared.module.functions.push_back(bir::Function{
      .name = "global_fixture",
      .global_static_gep_authorities =
          {make_available_global_static_gep_authority()},
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_global_static_semantic_geps(prepared);
  if (function.global_static_semantic_geps.size() != 1 ||
      function.global_static_semantic_geps.front().status !=
          bir::GlobalStaticGepAuthorityStatus::Available ||
      function.global_static_semantic_geps.front().authority !=
          &function.global_static_gep_authorities.front() ||
      function.global_static_semantic_geps.front().global_name != "numbers" ||
      function.global_static_semantic_geps.front().global_link_name_id !=
          c4c::LinkNameId{42} ||
      function.global_static_semantic_geps.front().result_name != "%elt" ||
      function.global_static_semantic_geps.front().byte_offset != 8 ||
      function.global_static_semantic_geps.front().element_type !=
          bir::TypeKind::I32 ||
      function.global_static_semantic_geps.front().element_size_bytes != 4 ||
      function.global_static_semantic_geps.front().range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      function.global_static_semantic_geps.front().coordinate_status !=
          bir::GlobalStaticGepCoordinateStatus::Available) {
    return fail("expected available authority to publish final global/static semantic GEP admission");
  }

  function.global_static_gep_authorities.front().status =
      bir::GlobalStaticGepAuthorityStatus::StringOrGlobalPointerProvenanceBoundary;
  prepare::populate_global_static_semantic_geps(prepared);
  if (function.global_static_semantic_geps.size() != 1 ||
      function.global_static_semantic_geps.front().status !=
          bir::GlobalStaticGepAuthorityStatus::
              StringOrGlobalPointerProvenanceBoundary) {
    return fail("expected final global/static semantic GEP admission to preserve pointer/string boundary");
  }

  function.global_static_gep_authorities.front() =
      make_available_global_static_gep_authority();
  function.global_static_gep_authorities.front().coordinate_status =
      bir::GlobalStaticGepCoordinateStatus::MissingLirInstructionIndex;
  prepare::populate_global_static_semantic_geps(prepared);
  if (function.global_static_semantic_geps.size() != 1 ||
      function.global_static_semantic_geps.front().status !=
          bir::GlobalStaticGepAuthorityStatus::PreparedBirCoordinateConfusion) {
    return fail("expected final global/static semantic GEP admission to reject coordinate confusion");
  }

  function.global_static_gep_authorities = {
      make_available_global_static_gep_authority(),
      make_available_global_static_gep_authority(),
  };
  prepare::populate_global_static_semantic_geps(prepared);
  if (function.global_static_semantic_geps.size() != 2 ||
      function.global_static_semantic_geps.front().status !=
          bir::GlobalStaticGepAuthorityStatus::PreparedBirCoordinateConfusion ||
      function.global_static_semantic_geps.back().status !=
          bir::GlobalStaticGepAuthorityStatus::PreparedBirCoordinateConfusion) {
    return fail("expected final global/static semantic GEP admission to reject duplicate authority identities");
  }

  return 0;
}

int local_array_scalar_local_load_population_requires_matching_provenance() {
  const auto expect_shifted_load_status =
      [](bir::LoadLocalInst shifted_load,
         const char* failure_message) -> int {
    auto prepared = make_effect_stream_prepared_module({
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
            .operand_type = bir::TypeKind::I64,
            .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
            .rhs = bir::Value::immediate_i64(1),
        },
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
            .operand_type = bir::TypeKind::I64,
            .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
            .rhs = bir::Value::immediate_i64(1),
        },
        bir::CastInst{
            .opcode = bir::CastOpcode::Bitcast,
            .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
            .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
        },
        shifted_load,
    });
    auto& function = prepared.module.functions.front();
    prepare::populate_local_array_ordered_effect_source_streams(prepared);
    prepare::populate_local_array_interval_effects(prepared);
    prepare::populate_local_array_index_range_proofs(prepared);
    prepare::populate_local_array_proof_facts(prepared);
    prepare::populate_local_array_index_range_checker_inputs(prepared);
    prepare::populate_local_array_local_address_provenances(prepared);
    prepare::populate_local_array_scalar_local_loads(prepared);
    if (function.local_array_scalar_local_loads.size() != 1 ||
        function.local_array_scalar_local_loads.front().status !=
            bir::LocalArrayScalarLocalLoadStatus::PreparedBirCoordinateConfusion) {
      return fail(failure_message);
    }
    return 0;
  };

  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
      effect_stream_scalar_load(),
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_scalar_local_loads.size() != 1 ||
      function.local_array_scalar_local_loads.front().status !=
          bir::LocalArrayScalarLocalLoadStatus::MissingProvenance) {
    return fail("expected scalar local-load publication to reject missing provenance");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  prepare::populate_local_array_interval_effects(prepared);
  prepare::populate_local_array_index_range_proofs(prepared);
  prepare::populate_local_array_proof_facts(prepared);
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  prepare::populate_local_array_local_address_provenances(prepared);
  function.local_array_local_address_provenances.front().status =
      bir::LocalArrayCarrierStatus::IntegerPointerRoundTrip;
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_scalar_local_loads.size() != 1 ||
      function.local_array_scalar_local_loads.front().status !=
          bir::LocalArrayScalarLocalLoadStatus::IntegerPointerRoundTrip ||
      function.local_array_scalar_local_loads.front().provenance_status !=
          bir::LocalArrayCarrierStatus::IntegerPointerRoundTrip) {
    return fail("expected scalar local-load publication to preserve non-available provenance status");
  }

  prepare::populate_local_array_local_address_provenances(prepared);
  auto duplicate = function.local_array_local_address_provenances.front();
  function.local_array_local_address_provenances.push_back(duplicate);
  prepare::populate_local_array_scalar_local_loads(prepared);
  if (function.local_array_scalar_local_loads.size() != 1 ||
      function.local_array_scalar_local_loads.front().status !=
          bir::LocalArrayScalarLocalLoadStatus::PreparedBirCoordinateConfusion) {
    return fail("expected scalar local-load publication to reject duplicate provenance");
  }
  if (const int rc = expect_shifted_load_status(
          effect_stream_scalar_load(4, 0),
          "expected scalar local-load publication to reject shifted load instruction offsets");
      rc != 0) {
    return rc;
  }
  if (const int rc = expect_shifted_load_status(
          effect_stream_scalar_load(0, 4),
          "expected scalar local-load publication to reject shifted address offsets");
      rc != 0) {
    return rc;
  }
  return 0;
}

int local_array_local_address_provenance_requires_matching_checker_input() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_local_array_local_address_provenances(prepared);
  if (function.local_array_local_address_provenances.size() != 1 ||
      function.local_array_local_address_provenances.front().status !=
          bir::LocalArrayCarrierStatus::MissingIndexRangeProof ||
      function.local_array_local_address_provenances.front().checker_status !=
          bir::LocalArrayRangeProofStatus::MissingProofFact) {
    return fail("expected provenance publication to reject missing checker inputs");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  prepare::populate_local_array_interval_effects(prepared);
  prepare::populate_local_array_index_range_proofs(prepared);
  prepare::populate_local_array_proof_facts(prepared);
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  function.local_array_index_range_checker_inputs.front().status =
      bir::LocalArrayRangeProofStatus::OperandRoleMismatch;
  function.local_array_index_range_checker_inputs.front().checker_record.status =
      bir::LocalArrayRangeProofStatus::OperandRoleMismatch;
  prepare::populate_local_array_local_address_provenances(prepared);
  if (function.local_array_local_address_provenances.size() != 1 ||
      function.local_array_local_address_provenances.front().status !=
          bir::LocalArrayCarrierStatus::MissingIndexRangeProof ||
      function.local_array_local_address_provenances.front().checker_status !=
          bir::LocalArrayRangeProofStatus::OperandRoleMismatch) {
    return fail("expected provenance publication to preserve non-available checker input status");
  }

  prepare::populate_local_array_index_range_checker_inputs(prepared);
  auto confused = function.local_array_index_range_checker_inputs.front();
  function.local_array_index_range_checker_inputs.push_back(confused);
  prepare::populate_local_array_local_address_provenances(prepared);
  if (function.local_array_local_address_provenances.size() != 1 ||
      function.local_array_local_address_provenances.front().status !=
          bir::LocalArrayCarrierStatus::PreparedBirCoordinateConfusion) {
    return fail("expected provenance publication to reject duplicate checker inputs");
  }
  return 0;
}

int local_array_checker_input_population_requires_matching_proof_fact() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  if (function.local_array_index_range_checker_inputs.size() != 1 ||
      function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingProofFact) {
    return fail("expected checker input publication to reject missing proof facts");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  prepare::populate_local_array_interval_effects(prepared);
  prepare::populate_local_array_index_range_proofs(prepared);
  prepare::populate_local_array_proof_facts(prepared);
  function.local_array_proof_facts.front().status =
      bir::LocalArrayRangeProofStatus::IntervalEffectOnlyInference;
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  if (function.local_array_index_range_checker_inputs.size() != 1 ||
      function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::IntervalEffectOnlyInference) {
    return fail("expected checker input publication to preserve non-available proof fact status");
  }

  prepare::populate_local_array_proof_facts(prepared);
  function.local_array_proof_facts.front().proof_lhs =
      bir::Value::named(bir::TypeKind::I64, "%other.lhs");
  function.local_array_proof_facts.front().proof_rhs =
      bir::Value::immediate_i64(4);
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  if (function.local_array_index_range_checker_inputs.size() != 1 ||
      function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::OperandRoleMismatch) {
    return fail("expected checker input publication to preserve operand-role mismatch");
  }
  if (function.local_array_index_range_checker_inputs.front()
          .inputs.operand_roles_match_index) {
    return fail("expected checker input publication to mark operand roles mismatched");
  }

  prepare::populate_local_array_proof_facts(prepared);
  function.local_array_proof_facts.front().status =
      bir::LocalArrayRangeProofStatus::Available;
  function.local_array_proof_facts.push_back(function.local_array_proof_facts.front());
  prepare::populate_local_array_index_range_checker_inputs(prepared);
  if (function.local_array_index_range_checker_inputs.size() != 1 ||
      function.local_array_index_range_checker_inputs.front().status !=
          bir::LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion) {
    return fail("expected checker input publication to reject duplicate proof facts");
  }
  return 0;
}

int local_array_proof_fact_population_requires_matching_range_certificate() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& function = prepared.module.functions.front();
  prepare::populate_local_array_proof_facts(prepared);
  if (function.local_array_proof_facts.size() != 1 ||
      function.local_array_proof_facts.front().status !=
          bir::LocalArrayRangeProofStatus::MissingRangeProofCertificate) {
    return fail("expected proof fact publication to reject missing range certificates");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  prepare::populate_local_array_interval_effects(prepared);
  prepare::populate_local_array_index_range_proofs(prepared);
  function.local_array_index_range_proofs.front().status =
      bir::LocalArrayRangeProofStatus::SelectedPathOnlyInference;
  prepare::populate_local_array_proof_facts(prepared);
  if (function.local_array_proof_facts.size() != 1 ||
      function.local_array_proof_facts.front().status !=
          bir::LocalArrayRangeProofStatus::SelectedPathOnlyInference) {
    return fail("expected proof fact publication to preserve non-available certificate status");
  }

  function.local_array_index_range_proofs.front().status =
      bir::LocalArrayRangeProofStatus::Available;
  function.local_array_index_range_proofs.push_back(
      function.local_array_index_range_proofs.front());
  prepare::populate_local_array_proof_facts(prepared);
  if (function.local_array_proof_facts.size() != 1 ||
      function.local_array_proof_facts.front().status !=
          bir::LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion) {
    return fail("expected proof fact publication to reject duplicate coordinate-confused certificates");
  }
  return 0;
}

int local_array_interval_consumer_requires_populated_matching_stream() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& function = prepared.module.functions.front();
  auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream) {
    return fail("expected empty production stream records to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream) {
    return fail("expected interval fact publication to reject missing stored stream");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingIntervalEffect) {
    return fail("expected range proof publication to reject missing interval effects");
  }

  function.local_array_ordered_effect_source_streams.push_back(
      bir::LocalArrayOrderedEffectSourceStream{
          .status = bir::LocalArrayOrderedEffectSourceStreamStatus::Available,
          .interval = {
              .proof_source =
                  bir::LocalArrayEffectSourceCoordinate{
                      .prepared_block_index = std::size_t{0},
                      .bir_block_label = "guard",
                      .instruction_index = std::size_t{0},
                  },
              .endpoint =
                  bir::LocalArrayEffectSourceCoordinate{
                      .prepared_block_index = std::size_t{1},
                      .bir_block_label = "body",
                      .instruction_index = std::size_t{2},
                  },
          },
      });
  effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream) {
    return fail("expected synthetic path-only stream evidence to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream) {
    return fail("expected interval fact publication to reject synthetic path-only stream evidence");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingIntervalEffect) {
    return fail("expected range proof publication to reject synthetic path-only stream evidence");
  }

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front(),
      true);
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::PreparedBirCoordinateConfusion) {
    return fail("expected coordinate confusion to fail closed before consuming stored streams");
  }

  auto missing_endpoint = prepared;
  missing_endpoint.module.functions.front()
      .local_array_endpoint_bridges.front()
      .status =
      bir::LocalArrayEndpointBridgeStatus::MissingPreparedBirEndpointBridge;
  prepare::populate_local_array_ordered_effect_source_streams(missing_endpoint);
  const auto& missing_endpoint_function =
      missing_endpoint.module.functions.front();
  effect = bir::evaluate_local_array_interval_effect(
      missing_endpoint_function,
      &missing_endpoint_function.local_array_selected_proof_edge_paths.front(),
      &missing_endpoint_function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::MissingPreparedBirEndpointBridge) {
    return fail("expected missing endpoint bridge to fail closed");
  }
  prepare::populate_local_array_interval_effects(missing_endpoint);
  if (missing_endpoint_function.local_array_interval_effects.size() != 1 ||
      missing_endpoint_function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::MissingPreparedBirEndpointBridge) {
    return fail("expected interval fact publication to reject missing endpoint bridge");
  }
  prepare::populate_local_array_index_range_proofs(missing_endpoint);
  if (missing_endpoint_function.local_array_index_range_proofs.size() != 1 ||
      missing_endpoint_function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::UnsupportedBoundary) {
    return fail("expected range proof publication to reject missing endpoint bridge");
  }

  function.local_array_ordered_effect_source_streams.push_back(
      function.local_array_ordered_effect_source_streams.front());
  effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::DuplicateOrderedEffectSourceStream) {
    return fail("expected duplicate matching stream records to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::DuplicateOrderedEffectSourceStream) {
    return fail("expected interval fact publication to reject duplicate stored streams");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingIntervalEffect) {
    return fail("expected range proof publication to reject duplicate stored streams");
  }
  return 0;
}

int ordered_effect_stream_fails_closed_on_missing_proof_coordinate() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp2"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  prepared.module.functions.front()
      .local_array_selected_proof_edge_paths.front()
      .proof_instruction_index.reset();

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& stream =
      prepared.module.functions.front().local_array_ordered_effect_source_streams.front();
  if (stream.status !=
      bir::LocalArrayOrderedEffectSourceStreamStatus::MissingLowerBoundaryCoordinate) {
    return fail("expected missing proof coordinate to fail closed");
  }
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate) {
    return fail("expected missing proof coordinate to reach interval classifier");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate) {
    return fail("expected interval fact publication to reject missing proof coordinate");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::MissingLirProducerCoordinate) {
    return fail("expected range proof publication to reject missing proof coordinate");
  }
  return 0;
}

int ordered_effect_stream_fails_closed_on_unordered_boundary() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });
  auto& bridge =
      prepared.module.functions.front().local_array_endpoint_bridges.front();
  bridge.prepared_block_label = "guard";
  bridge.prepared_block_index = std::size_t{0};
  bridge.bir_block_label = "guard";
  bridge.endpoint_instruction_index = std::size_t{0};

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& stream =
      prepared.module.functions.front().local_array_ordered_effect_source_streams.front();
  if (stream.status !=
      bir::LocalArrayOrderedEffectSourceStreamStatus::UnorderedBoundaryCoordinate) {
    return fail("expected unordered proof-to-endpoint boundary to fail closed");
  }
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary) {
    return fail("expected unordered boundary to reach interval classifier");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary) {
    return fail("expected interval fact publication to reject unordered boundaries");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::UnsupportedBoundary) {
    return fail("expected range proof publication to reject unordered boundaries");
  }
  return 0;
}

int ordered_effect_stream_records_unknown_and_clobber_sources() {
  bir::InlineAsmMetadata asm_metadata;
  asm_metadata.side_effects = true;
  asm_metadata.clobbers = {"memory"};
  auto prepared = make_effect_stream_prepared_module({
      bir::CallInst{
          .callee = "helper",
          .return_type = bir::TypeKind::Void,
      },
      bir::CallInst{
          .callee = "asm",
          .return_type = bir::TypeKind::Void,
          .inline_asm = asm_metadata,
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& stream =
      prepared.module.functions.front().local_array_ordered_effect_source_streams.front();
  bool saw_unknown_call = false;
  bool saw_inline_asm_clobber = false;
  for (const auto& source : stream.sources) {
    saw_unknown_call =
        saw_unknown_call ||
        (source.family == bir::LocalArrayEffectSourceFamily::CallOrHelper &&
         source.status == bir::LocalArrayEffectSourceStatus::UnknownModeledEffect);
    saw_inline_asm_clobber =
        saw_inline_asm_clobber ||
        (source.family == bir::LocalArrayEffectSourceFamily::InlineAsm &&
         source.status == bir::LocalArrayEffectSourceStatus::ClobbersIndex);
  }
  if (!saw_unknown_call || !saw_inline_asm_clobber) {
    return fail("expected production stream to record unknown call and inline asm clobber sources");
  }
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::CallOrHelperEffectUnknown) {
    return fail("expected first in-interval unknown call source to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::CallOrHelperEffectUnknown) {
    return fail("expected interval fact publication to reject unknown call effects");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::CallOrHelperEffectUnknown) {
    return fail("expected range proof publication to reject unknown call effects");
  }
  return 0;
}

int ordered_effect_stream_records_clobber_sources() {
  bir::InlineAsmMetadata asm_metadata;
  asm_metadata.side_effects = true;
  asm_metadata.clobbers = {"memory"};
  auto prepared = make_effect_stream_prepared_module({
      bir::CallInst{
          .callee = "asm",
          .return_type = bir::TypeKind::Void,
          .inline_asm = asm_metadata,
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::InlineAsmClobbersIndex) {
    return fail("expected production clobber source to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::InlineAsmClobbersIndex) {
    return fail("expected interval fact publication to reject clobbering effects");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::InlineAsmClobbersIndex) {
    return fail("expected range proof publication to reject clobbering effects");
  }
  return 0;
}

int ordered_effect_stream_fails_closed_on_coordinate_confusion() {
  auto prepared = make_effect_stream_prepared_module({
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front(),
      true);
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::PreparedBirCoordinateConfusion) {
    return fail("expected coordinate confusion to reject stored stream availability");
  }
  return 0;
}

int ordered_effect_stream_records_phi_alias_sources() {
  auto prepared = make_effect_stream_prepared_module({
      bir::PhiInst{
          .result = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .incomings = {
              bir::PhiIncoming{
                  .label = "guard",
                  .value = bir::Value::named(bir::TypeKind::I64, "%idx.in"),
              },
          },
      },
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, "%tmp"),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::named(bir::TypeKind::I64, "%idx"),
          .rhs = bir::Value::immediate_i64(1),
      },
      bir::CastInst{
          .opcode = bir::CastOpcode::Bitcast,
          .result = bir::Value::named(bir::TypeKind::Ptr, "%elt.ptr"),
          .operand = bir::Value::named(bir::TypeKind::Ptr, "%base"),
      },
  });

  prepare::populate_local_array_ordered_effect_source_streams(prepared);
  const auto& stream =
      prepared.module.functions.front().local_array_ordered_effect_source_streams.front();
  const bool saw_phi_alias = std::any_of(
      stream.sources.begin(),
      stream.sources.end(),
      [](const bir::LocalArrayOrderedEffectSourceRecord& source) {
        return source.family ==
                   bir::LocalArrayEffectSourceFamily::PhiOrAliasTransfer &&
               source.status ==
                   bir::LocalArrayEffectSourceStatus::PhiOrAliasUnresolved;
      });
  if (!saw_phi_alias) {
    return fail("expected production stream to record phi/alias index sources");
  }
  const auto& function = prepared.module.functions.front();
  const auto effect = bir::evaluate_local_array_interval_effect(
      function,
      &function.local_array_selected_proof_edge_paths.front(),
      &function.local_array_endpoint_bridges.front());
  if (effect.status !=
      bir::LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved) {
    return fail("expected production phi/alias source to fail closed");
  }
  prepare::populate_local_array_interval_effects(prepared);
  if (function.local_array_interval_effects.size() != 1 ||
      function.local_array_interval_effects.front().status !=
          bir::LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved) {
    return fail("expected interval fact publication to reject phi/alias ambiguity");
  }
  prepare::populate_local_array_index_range_proofs(prepared);
  if (function.local_array_index_range_proofs.size() != 1 ||
      function.local_array_index_range_proofs.front().status !=
          bir::LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved) {
    return fail("expected range proof publication to reject phi/alias ambiguity");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = records_register_home_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_stack_slot_home_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_rematerializable_immediate_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_pointer_base_plus_offset_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_shape_without_aarch64_policy(); rc != 0) {
    return rc;
  }
  if (int rc =
          production_publish_contract_plans_populates_local_array_interval_effects();
      rc != 0) {
    return rc;
  }
  if (int rc = populates_clean_local_array_ordered_effect_stream(); rc != 0) {
    return rc;
  }
  if (int rc =
          local_array_local_address_provenance_requires_matching_checker_input();
      rc != 0) {
    return rc;
  }
  if (int rc =
          local_array_semantic_gep_population_requires_matching_provenance();
      rc != 0) {
    return rc;
  }
  if (int rc =
          global_static_semantic_gep_population_requires_matching_authority();
      rc != 0) {
    return rc;
  }
  if (int rc =
          local_array_scalar_local_load_population_requires_matching_provenance();
      rc != 0) {
    return rc;
  }
  if (int rc = local_array_checker_input_population_requires_matching_proof_fact();
      rc != 0) {
    return rc;
  }
  if (int rc = local_array_proof_fact_population_requires_matching_range_certificate();
      rc != 0) {
    return rc;
  }
  if (int rc = local_array_interval_consumer_requires_populated_matching_stream();
      rc != 0) {
    return rc;
  }
  if (int rc = ordered_effect_stream_fails_closed_on_missing_proof_coordinate();
      rc != 0) {
    return rc;
  }
  if (int rc = ordered_effect_stream_fails_closed_on_unordered_boundary();
      rc != 0) {
    return rc;
  }
  if (int rc = ordered_effect_stream_records_unknown_and_clobber_sources();
      rc != 0) {
    return rc;
  }
  if (int rc = ordered_effect_stream_records_clobber_sources(); rc != 0) {
    return rc;
  }
  if (int rc = ordered_effect_stream_fails_closed_on_coordinate_confusion();
      rc != 0) {
    return rc;
  }
  return ordered_effect_stream_records_phi_alias_sources();
}
