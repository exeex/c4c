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
      .element_count = 4,
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

prepare::PreparedBirModule make_effect_stream_prepared_module(
    std::vector<bir::Inst> body_insts) {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("stream_fixture");
  const auto guard_label = prepared.names.block_labels.intern("guard");
  const auto body_label = prepared.names.block_labels.intern("body");
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {
          prepare::PreparedControlFlowBlock{.block_label = guard_label},
          prepare::PreparedControlFlowBlock{.block_label = body_label},
      },
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
      },
  });
  auto& function = prepared.module.functions.front();
  const auto& path = function.local_array_element_paths.front();
  function.local_array_selected_proof_edge_paths.push_back(
      effect_stream_selected_path(&path));
  function.local_array_endpoint_bridges.push_back(
      effect_stream_endpoint_bridge(&path));
  return prepared;
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
  if (int rc = populates_clean_local_array_ordered_effect_stream(); rc != 0) {
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
