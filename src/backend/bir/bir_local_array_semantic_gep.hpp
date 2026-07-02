#pragma once

namespace c4c::backend::bir {

enum class LocalArrayCarrierStatus : unsigned char {
  Available,
  MissingSourceObject,
  SourceObjectNotLocal,
  MissingSourceObjectLayout,
  MissingObjectToSlotRelation,
  MissingDerivation,
  DerivationNotProvenLocal,
  MissingDerivedPointerIdentity,
  MissingElementPath,
  MissingIndexIdentity,
  MissingIndexRangeProof,
  RangeProofNotDominatingConsumer,
  RangeProofPathNotCoveringConsumer,
  IndexValueClobberedBeforeConsumer,
  ElementOutOfBounds,
  ElementNotScalar,
  LayoutRangeMismatch,
  UnknownProvenance,
  IntegerPointerRoundTrip,
  GlobalSourceObject,
  AggregateOrMemberBoundary,
  UnionOrObjectRepresentationBoundary,
  VariadicOrVaArgBoundary,
  RuntimeOrCallBoundary,
  F128ComplexVectorOrVolatileAtomicBoundary,
  BootstrapBoundary,
  RawShapeOnly,
  TargetOnlyOrFinalHomeOnly,
  PreparedBirCoordinateConfusion,
};

[[nodiscard]] constexpr std::string_view local_array_carrier_status_name(
    LocalArrayCarrierStatus status) {
  switch (status) {
    case LocalArrayCarrierStatus::Available:
      return "available";
    case LocalArrayCarrierStatus::MissingSourceObject:
      return "missing_source_object";
    case LocalArrayCarrierStatus::SourceObjectNotLocal:
      return "source_object_not_local";
    case LocalArrayCarrierStatus::MissingSourceObjectLayout:
      return "missing_source_object_layout";
    case LocalArrayCarrierStatus::MissingObjectToSlotRelation:
      return "missing_object_to_slot_relation";
    case LocalArrayCarrierStatus::MissingDerivation:
      return "missing_derivation";
    case LocalArrayCarrierStatus::DerivationNotProvenLocal:
      return "derivation_not_proven_local";
    case LocalArrayCarrierStatus::MissingDerivedPointerIdentity:
      return "missing_derived_pointer_identity";
    case LocalArrayCarrierStatus::MissingElementPath:
      return "missing_element_path";
    case LocalArrayCarrierStatus::MissingIndexIdentity:
      return "missing_index_identity";
    case LocalArrayCarrierStatus::MissingIndexRangeProof:
      return "missing_index_range_proof";
    case LocalArrayCarrierStatus::RangeProofNotDominatingConsumer:
      return "range_proof_not_dominating_consumer";
    case LocalArrayCarrierStatus::RangeProofPathNotCoveringConsumer:
      return "range_proof_path_not_covering_consumer";
    case LocalArrayCarrierStatus::IndexValueClobberedBeforeConsumer:
      return "index_value_clobbered_before_consumer";
    case LocalArrayCarrierStatus::ElementOutOfBounds:
      return "element_out_of_bounds";
    case LocalArrayCarrierStatus::ElementNotScalar:
      return "element_not_scalar";
    case LocalArrayCarrierStatus::LayoutRangeMismatch:
      return "layout_range_mismatch";
    case LocalArrayCarrierStatus::UnknownProvenance:
      return "unknown_provenance";
    case LocalArrayCarrierStatus::IntegerPointerRoundTrip:
      return "integer_pointer_round_trip";
    case LocalArrayCarrierStatus::GlobalSourceObject:
      return "global_source_object";
    case LocalArrayCarrierStatus::AggregateOrMemberBoundary:
      return "aggregate_or_member_boundary";
    case LocalArrayCarrierStatus::UnionOrObjectRepresentationBoundary:
      return "union_or_object_representation_boundary";
    case LocalArrayCarrierStatus::VariadicOrVaArgBoundary:
      return "variadic_or_va_arg_boundary";
    case LocalArrayCarrierStatus::RuntimeOrCallBoundary:
      return "runtime_or_call_boundary";
    case LocalArrayCarrierStatus::F128ComplexVectorOrVolatileAtomicBoundary:
      return "f128_complex_vector_or_volatile_atomic_boundary";
    case LocalArrayCarrierStatus::BootstrapBoundary:
      return "bootstrap_boundary";
    case LocalArrayCarrierStatus::RawShapeOnly:
      return "raw_shape_only";
    case LocalArrayCarrierStatus::TargetOnlyOrFinalHomeOnly:
      return "target_only_or_final_home_only";
    case LocalArrayCarrierStatus::PreparedBirCoordinateConfusion:
      return "prepared_bir_coordinate_confusion";
  }
  return "unknown";
}

enum class LocalArrayDerivationKind : unsigned char {
  Unknown,
  ArrayDecay,
  LocalAddressOfElement,
  DirectLocalArrayElement,
};

[[nodiscard]] constexpr std::string_view local_array_derivation_kind_name(
    LocalArrayDerivationKind kind) {
  switch (kind) {
    case LocalArrayDerivationKind::Unknown:
      return "unknown";
    case LocalArrayDerivationKind::ArrayDecay:
      return "array_decay";
    case LocalArrayDerivationKind::LocalAddressOfElement:
      return "local_address_of_element";
    case LocalArrayDerivationKind::DirectLocalArrayElement:
      return "direct_local_array_element";
  }
  return "unknown";
}

enum class LocalArrayIndexKind : unsigned char {
  Constant,
  Dynamic,
};

enum class LocalArrayLirProducerOperationRole : unsigned char {
  None,
  AddressDerivation,
  LoadConsumer,
  StoreConsumer,
  UnknownConsumer,
};

[[nodiscard]] constexpr std::string_view local_array_lir_producer_operation_role_name(
    LocalArrayLirProducerOperationRole role) {
  switch (role) {
    case LocalArrayLirProducerOperationRole::None:
      return "none";
    case LocalArrayLirProducerOperationRole::AddressDerivation:
      return "address_derivation";
    case LocalArrayLirProducerOperationRole::LoadConsumer:
      return "load_consumer";
    case LocalArrayLirProducerOperationRole::StoreConsumer:
      return "store_consumer";
    case LocalArrayLirProducerOperationRole::UnknownConsumer:
      return "unknown_consumer";
  }
  return "unknown";
}

enum class LocalArrayLirProducerCoordinateStatus : unsigned char {
  Available,
  MissingLirProducerCoordinate,
  MissingBlockLabel,
  MissingLirInstructionIndex,
  MissingLirProducerLookupKey,
  DuplicateCoordinateCandidate,
  DuplicatePathRecord,
  MismatchedFunction,
  MismatchedBlock,
  MismatchedInstructionResult,
  MismatchedSourceObject,
  MismatchedDerivationResult,
  MismatchedDynamicIndex,
  UnsupportedOperationRole,
  UnsupportedBoundary,
  RawShapeOnly,
  TargetOnlyOrFinalHomeOnly,
};

[[nodiscard]] constexpr std::string_view local_array_lir_producer_coordinate_status_name(
    LocalArrayLirProducerCoordinateStatus status) {
  switch (status) {
    case LocalArrayLirProducerCoordinateStatus::Available:
      return "available";
    case LocalArrayLirProducerCoordinateStatus::MissingLirProducerCoordinate:
      return "missing_lir_producer_coordinate";
    case LocalArrayLirProducerCoordinateStatus::MissingBlockLabel:
      return "missing_block_label";
    case LocalArrayLirProducerCoordinateStatus::MissingLirInstructionIndex:
      return "missing_lir_instruction_index";
    case LocalArrayLirProducerCoordinateStatus::MissingLirProducerLookupKey:
      return "missing_lir_producer_lookup_key";
    case LocalArrayLirProducerCoordinateStatus::DuplicateCoordinateCandidate:
      return "duplicate_coordinate_candidate";
    case LocalArrayLirProducerCoordinateStatus::DuplicatePathRecord:
      return "duplicate_path_record";
    case LocalArrayLirProducerCoordinateStatus::MismatchedFunction:
      return "mismatched_function";
    case LocalArrayLirProducerCoordinateStatus::MismatchedBlock:
      return "mismatched_block";
    case LocalArrayLirProducerCoordinateStatus::MismatchedInstructionResult:
      return "mismatched_instruction_result";
    case LocalArrayLirProducerCoordinateStatus::MismatchedSourceObject:
      return "mismatched_source_object";
    case LocalArrayLirProducerCoordinateStatus::MismatchedDerivationResult:
      return "mismatched_derivation_result";
    case LocalArrayLirProducerCoordinateStatus::MismatchedDynamicIndex:
      return "mismatched_dynamic_index";
    case LocalArrayLirProducerCoordinateStatus::UnsupportedOperationRole:
      return "unsupported_operation_role";
    case LocalArrayLirProducerCoordinateStatus::UnsupportedBoundary:
      return "unsupported_boundary";
    case LocalArrayLirProducerCoordinateStatus::RawShapeOnly:
      return "raw_shape_only";
    case LocalArrayLirProducerCoordinateStatus::TargetOnlyOrFinalHomeOnly:
      return "target_only_or_final_home_only";
  }
  return "unknown";
}

struct LocalArrayIndexRecord {
  LocalArrayIndexKind kind = LocalArrayIndexKind::Constant;
  std::int64_t constant = 0;
  Value value;
};

struct LocalArraySourceObjectRecord {
  std::string object_name;
  TypeKind element_type = TypeKind::Void;
  std::string type_text;
  std::size_t element_count = 0;
  std::size_t element_size_bytes = 0;
  std::size_t total_size_bytes = 0;
  std::size_t align_bytes = 0;
  std::vector<std::string> element_slots;
  LocalArrayCarrierStatus status = LocalArrayCarrierStatus::Available;
};

struct LocalArrayAddressDerivationRecord {
  std::string result_name;
  std::string source_object_name;
  std::string base_view_name;
  LocalArrayDerivationKind kind = LocalArrayDerivationKind::Unknown;
  std::size_t base_index = 0;
  LocalArrayCarrierStatus status = LocalArrayCarrierStatus::Available;
};

struct LocalArrayElementPathRecord {
  std::string result_name;
  std::string source_object_name;
  std::string derivation_result_name;
  std::vector<LocalArrayIndexRecord> indices;
  TypeKind element_type = TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  bool scalar_in_bounds = false;
  LocalArrayCarrierStatus status = LocalArrayCarrierStatus::Available;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  LocalArrayLirProducerOperationRole lir_producer_operation_role =
      LocalArrayLirProducerOperationRole::None;
  std::string lir_producer_lookup_key;
  LocalArrayLirProducerCoordinateStatus lir_producer_coordinate_status =
      LocalArrayLirProducerCoordinateStatus::MissingLirProducerCoordinate;
};

enum class LocalArraySelectedProofEdgeOutcome : unsigned char {
  None,
  True,
  False,
};

[[nodiscard]] constexpr std::string_view local_array_selected_proof_edge_outcome_name(
    LocalArraySelectedProofEdgeOutcome outcome) {
  switch (outcome) {
    case LocalArraySelectedProofEdgeOutcome::None:
      return "none";
    case LocalArraySelectedProofEdgeOutcome::True:
      return "true";
    case LocalArraySelectedProofEdgeOutcome::False:
      return "false";
  }
  return "unknown";
}

enum class LocalArraySelectedProofEdgeBoundContribution : unsigned char {
  None,
  Lower,
  Upper,
  Unknown,
};

[[nodiscard]] constexpr std::string_view
local_array_selected_proof_edge_bound_contribution_name(
    LocalArraySelectedProofEdgeBoundContribution contribution) {
  switch (contribution) {
    case LocalArraySelectedProofEdgeBoundContribution::None:
      return "none";
    case LocalArraySelectedProofEdgeBoundContribution::Lower:
      return "lower";
    case LocalArraySelectedProofEdgeBoundContribution::Upper:
      return "upper";
    case LocalArraySelectedProofEdgeBoundContribution::Unknown:
      return "unknown";
  }
  return "unknown";
}

enum class LocalArraySelectedProofEdgePredicate : unsigned char {
  Unknown,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
  Eq,
  Ne,
};

[[nodiscard]] constexpr std::string_view
local_array_selected_proof_edge_predicate_name(
    LocalArraySelectedProofEdgePredicate predicate) {
  switch (predicate) {
    case LocalArraySelectedProofEdgePredicate::Unknown:
      return "unknown";
    case LocalArraySelectedProofEdgePredicate::Slt:
      return "slt";
    case LocalArraySelectedProofEdgePredicate::Sle:
      return "sle";
    case LocalArraySelectedProofEdgePredicate::Sgt:
      return "sgt";
    case LocalArraySelectedProofEdgePredicate::Sge:
      return "sge";
    case LocalArraySelectedProofEdgePredicate::Ult:
      return "ult";
    case LocalArraySelectedProofEdgePredicate::Ule:
      return "ule";
    case LocalArraySelectedProofEdgePredicate::Ugt:
      return "ugt";
    case LocalArraySelectedProofEdgePredicate::Uge:
      return "uge";
    case LocalArraySelectedProofEdgePredicate::Eq:
      return "eq";
    case LocalArraySelectedProofEdgePredicate::Ne:
      return "ne";
  }
  return "unknown";
}

enum class LocalArraySelectedProofEdgePathStatus : unsigned char {
  Available,
  MissingLocalArrayPath,
  MissingLirProducerLookupKey,
  MissingLirProducerCoordinate,
  UnsupportedLirProducerRole,
  MissingProofSource,
  ProofFunctionMismatch,
  MissingSelectedEdge,
  MissingSelectedOutcome,
  NonCoveringPath,
  NonDominatingOrGuardingProof,
  UnsupportedBoundary,
  MissingSameBlockOrdering,
  PreparedBirCoordinateConfusion,
  RawShapeOnly,
  TargetOrFinalHomeOnly,
};

[[nodiscard]] constexpr std::string_view local_array_selected_proof_edge_path_status_name(
    LocalArraySelectedProofEdgePathStatus status) {
  switch (status) {
    case LocalArraySelectedProofEdgePathStatus::Available:
      return "available";
    case LocalArraySelectedProofEdgePathStatus::MissingLocalArrayPath:
      return "missing_local_array_path";
    case LocalArraySelectedProofEdgePathStatus::MissingLirProducerLookupKey:
      return "missing_lir_producer_lookup_key";
    case LocalArraySelectedProofEdgePathStatus::MissingLirProducerCoordinate:
      return "missing_lir_producer_coordinate";
    case LocalArraySelectedProofEdgePathStatus::UnsupportedLirProducerRole:
      return "unsupported_lir_producer_role";
    case LocalArraySelectedProofEdgePathStatus::MissingProofSource:
      return "missing_proof_source";
    case LocalArraySelectedProofEdgePathStatus::ProofFunctionMismatch:
      return "proof_function_mismatch";
    case LocalArraySelectedProofEdgePathStatus::MissingSelectedEdge:
      return "missing_selected_edge";
    case LocalArraySelectedProofEdgePathStatus::MissingSelectedOutcome:
      return "missing_selected_outcome";
    case LocalArraySelectedProofEdgePathStatus::NonCoveringPath:
      return "non_covering_path";
    case LocalArraySelectedProofEdgePathStatus::NonDominatingOrGuardingProof:
      return "non_dominating_or_guarding_proof";
    case LocalArraySelectedProofEdgePathStatus::UnsupportedBoundary:
      return "unsupported_boundary";
    case LocalArraySelectedProofEdgePathStatus::MissingSameBlockOrdering:
      return "missing_same_block_ordering";
    case LocalArraySelectedProofEdgePathStatus::PreparedBirCoordinateConfusion:
      return "prepared_bir_coordinate_confusion";
    case LocalArraySelectedProofEdgePathStatus::RawShapeOnly:
      return "raw_shape_only";
    case LocalArraySelectedProofEdgePathStatus::TargetOrFinalHomeOnly:
      return "target_or_final_home_only";
  }
  return "unknown";
}

struct LocalArraySelectedProofEdgePathInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  std::string proof_function_name;
  std::string proof_block_label;
  Value proof_condition_value;
  bool proof_source_available = false;
  LocalArraySelectedProofEdgePredicate proof_predicate =
      LocalArraySelectedProofEdgePredicate::Unknown;
  TypeKind proof_compare_type = TypeKind::Void;
  Value proof_lhs;
  Value proof_rhs;
  std::optional<std::size_t> proof_instruction_index;
  LocalArraySelectedProofEdgeBoundContribution bound_contribution =
      LocalArraySelectedProofEdgeBoundContribution::None;
  std::optional<std::int64_t> normalized_bound;
  bool bound_inclusive = true;
  LocalArraySelectedProofEdgeOutcome selected_outcome =
      LocalArraySelectedProofEdgeOutcome::None;
  std::string selected_successor_label;
  std::string non_selected_successor_label;
  bool path_validity_known = false;
  bool selected_edge_reaches_lir_producer = false;
  bool selected_edge_covers_lir_producer = false;
  bool proof_dominates_lir_producer = false;
  bool proof_guards_lir_producer = false;
  bool same_block_candidate = false;
  bool same_block_ordering_known = false;
  bool prepared_bir_coordinate_confusion = false;
  bool raw_shape_only = false;
  bool target_or_final_home_only = false;
  bool unsupported_boundary = false;
};

struct LocalArraySelectedProofEdgePathRecord {
  LocalArraySelectedProofEdgePathStatus status =
      LocalArraySelectedProofEdgePathStatus::MissingLocalArrayPath;
  const LocalArrayElementPathRecord* element_path = nullptr;
  std::string path_result_name;
  std::string source_object_name;
  std::string derivation_result_name;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  LocalArrayLirProducerOperationRole lir_producer_operation_role =
      LocalArrayLirProducerOperationRole::None;
  std::string lir_producer_lookup_key;
  LocalArrayLirProducerCoordinateStatus lir_producer_coordinate_status =
      LocalArrayLirProducerCoordinateStatus::MissingLirProducerCoordinate;
  std::string proof_function_name;
  std::string proof_block_label;
  Value proof_condition_value;
  LocalArraySelectedProofEdgePredicate proof_predicate =
      LocalArraySelectedProofEdgePredicate::Unknown;
  TypeKind proof_compare_type = TypeKind::Void;
  Value proof_lhs;
  Value proof_rhs;
  std::optional<std::size_t> proof_instruction_index;
  LocalArraySelectedProofEdgeBoundContribution bound_contribution =
      LocalArraySelectedProofEdgeBoundContribution::None;
  std::optional<std::int64_t> normalized_bound;
  bool bound_inclusive = true;
  LocalArraySelectedProofEdgeOutcome selected_outcome =
      LocalArraySelectedProofEdgeOutcome::None;
  std::string selected_successor_label;
  std::string non_selected_successor_label;
  bool path_validity_known = false;
  bool selected_edge_reaches_lir_producer = false;
  bool selected_edge_covers_lir_producer = false;
  bool proof_dominates_lir_producer = false;
  bool proof_guards_lir_producer = false;
};

[[nodiscard]] inline const LocalArrayIndexRecord* single_dynamic_local_array_index(
    const LocalArrayElementPathRecord& path,
    bool* saw_multiple);

enum class LocalArrayEndpointBridgeStatus : unsigned char {
  Available,
  MissingProducerRow,
  DuplicateProducerRow,
  InvalidProducerCoordinate,
  CoordinateConfusion,
  MissingPreparedBirEndpointBridge,
  DuplicateEndpoint,
  MismatchedFunction,
  MismatchedResultValue,
  MismatchedSourceObject,
  MismatchedDerivationResult,
  MismatchedDynamicIndex,
  UnsupportedOperationRole,
  MissingEndpointOrder,
};

[[nodiscard]] constexpr std::string_view local_array_endpoint_bridge_status_name(
    LocalArrayEndpointBridgeStatus status) {
  switch (status) {
    case LocalArrayEndpointBridgeStatus::Available:
      return "available";
    case LocalArrayEndpointBridgeStatus::MissingProducerRow:
      return "missing_producer_row";
    case LocalArrayEndpointBridgeStatus::DuplicateProducerRow:
      return "duplicate_producer_row";
    case LocalArrayEndpointBridgeStatus::InvalidProducerCoordinate:
      return "invalid_producer_coordinate";
    case LocalArrayEndpointBridgeStatus::CoordinateConfusion:
      return "coordinate_confusion";
    case LocalArrayEndpointBridgeStatus::MissingPreparedBirEndpointBridge:
      return "missing_prepared_bir_endpoint_bridge";
    case LocalArrayEndpointBridgeStatus::DuplicateEndpoint:
      return "duplicate_endpoint";
    case LocalArrayEndpointBridgeStatus::MismatchedFunction:
      return "mismatched_function";
    case LocalArrayEndpointBridgeStatus::MismatchedResultValue:
      return "mismatched_result_value";
    case LocalArrayEndpointBridgeStatus::MismatchedSourceObject:
      return "mismatched_source_object";
    case LocalArrayEndpointBridgeStatus::MismatchedDerivationResult:
      return "mismatched_derivation_result";
    case LocalArrayEndpointBridgeStatus::MismatchedDynamicIndex:
      return "mismatched_dynamic_index";
    case LocalArrayEndpointBridgeStatus::UnsupportedOperationRole:
      return "unsupported_operation_role";
    case LocalArrayEndpointBridgeStatus::MissingEndpointOrder:
      return "missing_endpoint_order";
  }
  return "unknown";
}

struct LocalArrayEndpointBridgeInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  bool duplicate_producer_row = false;
  bool prepared_bir_coordinate_confusion = false;
  bool endpoint_available = false;
  bool duplicate_endpoint = false;
  bool mismatched_function = false;
  bool mismatched_result_value = false;
  bool mismatched_source_object = false;
  bool mismatched_derivation_result = false;
  bool mismatched_dynamic_index = false;
  bool missing_endpoint_order = false;
  std::string prepared_function_name;
  std::string prepared_block_label;
  std::optional<std::size_t> prepared_block_index;
  std::string bir_block_label;
  std::optional<std::size_t> endpoint_instruction_index;
  std::string address_materialization_kind;
  std::string result_value_name;
  std::string matched_source_object_name;
  std::string matched_derivation_result_name;
};

struct LocalArrayEndpointBridgeRecord {
  LocalArrayEndpointBridgeStatus status =
      LocalArrayEndpointBridgeStatus::MissingProducerRow;
  const LocalArrayElementPathRecord* element_path = nullptr;
  std::string path_result_name;
  std::string source_object_name;
  std::string derivation_result_name;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  LocalArrayLirProducerOperationRole lir_producer_operation_role =
      LocalArrayLirProducerOperationRole::None;
  std::string lir_producer_lookup_key;
  LocalArrayLirProducerCoordinateStatus lir_producer_coordinate_status =
      LocalArrayLirProducerCoordinateStatus::MissingLirProducerCoordinate;
  Value dynamic_index;
  std::string prepared_function_name;
  std::string prepared_block_label;
  std::optional<std::size_t> prepared_block_index;
  std::string bir_block_label;
  std::optional<std::size_t> endpoint_instruction_index;
  std::string address_materialization_kind;
  std::string result_value_name;
  std::string matched_source_object_name;
  std::string matched_derivation_result_name;
};

[[nodiscard]] inline LocalArrayEndpointBridgeRecord
evaluate_local_array_endpoint_bridge(
    const LocalArrayEndpointBridgeInputs& inputs) {
  LocalArrayEndpointBridgeRecord record{
      .element_path = inputs.element_path,
  };

  if (inputs.prepared_bir_coordinate_confusion) {
    record.status = LocalArrayEndpointBridgeStatus::CoordinateConfusion;
    return record;
  }
  if (inputs.element_path == nullptr) {
    record.status = LocalArrayEndpointBridgeStatus::MissingProducerRow;
    return record;
  }

  const auto& path = *inputs.element_path;
  record.path_result_name = path.result_name;
  record.source_object_name = path.source_object_name;
  record.derivation_result_name = path.derivation_result_name;
  record.lir_producer_function_name = path.lir_producer_function_name;
  record.lir_producer_block_label = path.lir_producer_block_label;
  record.lir_producer_instruction_index = path.lir_producer_instruction_index;
  record.lir_producer_operation_role = path.lir_producer_operation_role;
  record.lir_producer_lookup_key = path.lir_producer_lookup_key;
  record.lir_producer_coordinate_status = path.lir_producer_coordinate_status;
  record.prepared_function_name = inputs.prepared_function_name;
  record.prepared_block_label = inputs.prepared_block_label;
  record.prepared_block_index = inputs.prepared_block_index;
  record.bir_block_label = inputs.bir_block_label;
  record.endpoint_instruction_index = inputs.endpoint_instruction_index;
  record.address_materialization_kind = inputs.address_materialization_kind;
  record.result_value_name = inputs.result_value_name;
  record.matched_source_object_name = inputs.matched_source_object_name;
  record.matched_derivation_result_name = inputs.matched_derivation_result_name;

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(path, &saw_multiple_dynamic_indices);
  if (dynamic_index != nullptr) {
    record.dynamic_index = dynamic_index->value;
  }

  if (inputs.duplicate_producer_row) {
    record.status = LocalArrayEndpointBridgeStatus::DuplicateProducerRow;
    return record;
  }
  if (path.lir_producer_lookup_key.empty() ||
      path.lir_producer_coordinate_status !=
          LocalArrayLirProducerCoordinateStatus::Available) {
    record.status = LocalArrayEndpointBridgeStatus::InvalidProducerCoordinate;
    return record;
  }
  if (path.lir_producer_operation_role !=
      LocalArrayLirProducerOperationRole::AddressDerivation) {
    record.status = LocalArrayEndpointBridgeStatus::UnsupportedOperationRole;
    return record;
  }
  if (dynamic_index == nullptr || saw_multiple_dynamic_indices) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedDynamicIndex;
    return record;
  }
  if (inputs.mismatched_derivation_result) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedDerivationResult;
    return record;
  }
  if (inputs.mismatched_function) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedFunction;
    return record;
  }
  if (inputs.mismatched_result_value) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedResultValue;
    return record;
  }
  if (inputs.mismatched_source_object) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedSourceObject;
    return record;
  }
  if (inputs.mismatched_dynamic_index) {
    record.status = LocalArrayEndpointBridgeStatus::MismatchedDynamicIndex;
    return record;
  }
  if (inputs.duplicate_endpoint) {
    record.status = LocalArrayEndpointBridgeStatus::DuplicateEndpoint;
    return record;
  }
  if (!inputs.endpoint_available) {
    record.status =
        LocalArrayEndpointBridgeStatus::MissingPreparedBirEndpointBridge;
    return record;
  }
  if (inputs.missing_endpoint_order ||
      !inputs.prepared_block_index.has_value() ||
      !inputs.endpoint_instruction_index.has_value()) {
    record.status = LocalArrayEndpointBridgeStatus::MissingEndpointOrder;
    return record;
  }

  record.status = LocalArrayEndpointBridgeStatus::Available;
  return record;
}

enum class LocalArrayIntervalEffectStatus : unsigned char {
  Available,
  MissingLirProducerLookupKey,
  MissingLirProducerCoordinate,
  UnsupportedLirProducerRole,
  MissingDynamicIndex,
  DynamicIndexOperandMismatch,
  MissingSelectedEdgeOrOutcome,
  MissingPathValidity,
  PathNotCoveringLirProducer,
  MissingPreparedBirEndpointBridge,
  PreparedBirCoordinateConfusion,
  MissingSameBlockOrdering,
  SelectedPathOnlyInference,
  MissingOrderedEffectSourceStream,
  DuplicateOrderedEffectSourceStream,
  MissingEffectSourceCoordinate,
  UnorderedEffectSourceBoundary,
  IndexValueRedefined,
  IndexPhiOrAliasUnresolved,
  CallOrHelperEffectUnknown,
  CallOrHelperClobbersIndex,
  InlineAsmEffectUnknown,
  InlineAsmClobbersIndex,
  PublicationEffectUnknown,
  PublicationClobbersIndex,
  MoveBundleEffectUnknown,
  MoveBundleClobbersIndex,
  ParallelCopyEffectUnknown,
  ParallelCopyClobbersIndex,
  UnknownEffect,
  RawShapeOnly,
};

[[nodiscard]] constexpr std::string_view local_array_interval_effect_status_name(
    LocalArrayIntervalEffectStatus status) {
  switch (status) {
    case LocalArrayIntervalEffectStatus::Available:
      return "available";
    case LocalArrayIntervalEffectStatus::MissingLirProducerLookupKey:
      return "missing_lir_producer_lookup_key";
    case LocalArrayIntervalEffectStatus::MissingLirProducerCoordinate:
      return "missing_lir_producer_coordinate";
    case LocalArrayIntervalEffectStatus::UnsupportedLirProducerRole:
      return "unsupported_lir_producer_role";
    case LocalArrayIntervalEffectStatus::MissingDynamicIndex:
      return "missing_dynamic_index";
    case LocalArrayIntervalEffectStatus::DynamicIndexOperandMismatch:
      return "dynamic_index_operand_mismatch";
    case LocalArrayIntervalEffectStatus::MissingSelectedEdgeOrOutcome:
      return "missing_selected_edge_or_outcome";
    case LocalArrayIntervalEffectStatus::MissingPathValidity:
      return "missing_path_validity";
    case LocalArrayIntervalEffectStatus::PathNotCoveringLirProducer:
      return "path_not_covering_lir_producer";
    case LocalArrayIntervalEffectStatus::MissingPreparedBirEndpointBridge:
      return "missing_prepared_bir_endpoint_bridge";
    case LocalArrayIntervalEffectStatus::PreparedBirCoordinateConfusion:
      return "prepared_bir_coordinate_confusion";
    case LocalArrayIntervalEffectStatus::MissingSameBlockOrdering:
      return "missing_same_block_ordering";
    case LocalArrayIntervalEffectStatus::SelectedPathOnlyInference:
      return "selected_path_only_inference";
    case LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream:
      return "missing_ordered_effect_source_stream";
    case LocalArrayIntervalEffectStatus::DuplicateOrderedEffectSourceStream:
      return "duplicate_ordered_effect_source_stream";
    case LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate:
      return "missing_effect_source_coordinate";
    case LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary:
      return "unordered_effect_source_boundary";
    case LocalArrayIntervalEffectStatus::IndexValueRedefined:
      return "index_value_redefined";
    case LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved:
      return "index_phi_or_alias_unresolved";
    case LocalArrayIntervalEffectStatus::CallOrHelperEffectUnknown:
      return "call_or_helper_effect_unknown";
    case LocalArrayIntervalEffectStatus::CallOrHelperClobbersIndex:
      return "call_or_helper_clobbers_index";
    case LocalArrayIntervalEffectStatus::InlineAsmEffectUnknown:
      return "inline_asm_effect_unknown";
    case LocalArrayIntervalEffectStatus::InlineAsmClobbersIndex:
      return "inline_asm_clobbers_index";
    case LocalArrayIntervalEffectStatus::PublicationEffectUnknown:
      return "publication_effect_unknown";
    case LocalArrayIntervalEffectStatus::PublicationClobbersIndex:
      return "publication_clobbers_index";
    case LocalArrayIntervalEffectStatus::MoveBundleEffectUnknown:
      return "move_bundle_effect_unknown";
    case LocalArrayIntervalEffectStatus::MoveBundleClobbersIndex:
      return "move_bundle_clobbers_index";
    case LocalArrayIntervalEffectStatus::ParallelCopyEffectUnknown:
      return "parallel_copy_effect_unknown";
    case LocalArrayIntervalEffectStatus::ParallelCopyClobbersIndex:
      return "parallel_copy_clobbers_index";
    case LocalArrayIntervalEffectStatus::UnknownEffect:
      return "unknown_effect";
    case LocalArrayIntervalEffectStatus::RawShapeOnly:
      return "raw_shape_only";
  }
  return "unknown";
}

enum class LocalArrayEffectSourceFamily : unsigned char {
  IndexDefinition,
  PhiOrAliasTransfer,
  CallOrHelper,
  InlineAsm,
  Publication,
  MoveBundle,
  ParallelCopy,
  Unknown,
};

enum class LocalArrayEffectSourceStatus : unsigned char {
  PreservesIndex,
  RedefinesIndexValue,
  PhiOrAliasUnresolved,
  ClobbersIndex,
  UnknownModeledEffect,
  UnsupportedModeledEffect,
};

enum class LocalArrayOrderedEffectSourceStreamStatus : unsigned char {
  Available,
  MissingBuilder,
  MissingLowerBoundaryCoordinate,
  MissingEndpointCoordinate,
  MissingSourceCoordinate,
  UnorderedBoundaryCoordinate,
  UnsupportedModeledEffect,
};

struct LocalArrayEffectSourceCoordinate {
  std::optional<std::size_t> prepared_block_index;
  std::string bir_block_label;
  std::optional<std::size_t> instruction_index;
  std::size_t tie_break_index = 0;
};

struct LocalArrayIntervalBoundaryContract {
  LocalArrayEffectSourceCoordinate proof_source;
  LocalArrayEffectSourceCoordinate endpoint;
};

struct LocalArrayOrderedEffectSourceRecord {
  LocalArrayEffectSourceFamily family = LocalArrayEffectSourceFamily::Unknown;
  LocalArrayEffectSourceStatus status =
      LocalArrayEffectSourceStatus::UnknownModeledEffect;
  LocalArrayEffectSourceCoordinate coordinate;
  Value value;
};

struct LocalArrayOrderedEffectSourceStream {
  LocalArrayOrderedEffectSourceStreamStatus status =
      LocalArrayOrderedEffectSourceStreamStatus::MissingBuilder;
  LocalArrayIntervalBoundaryContract interval;
  const LocalArraySelectedProofEdgePathRecord* selected_path = nullptr;
  const LocalArrayEndpointBridgeRecord* endpoint_bridge = nullptr;
  std::vector<LocalArrayOrderedEffectSourceRecord> sources;
};

[[nodiscard]] inline bool local_array_effect_source_coordinate_available(
    const LocalArrayEffectSourceCoordinate& coordinate) {
  return coordinate.prepared_block_index.has_value() &&
         !coordinate.bir_block_label.empty() &&
         coordinate.instruction_index.has_value();
}

[[nodiscard]] inline int compare_local_array_effect_source_coordinates(
    const LocalArrayEffectSourceCoordinate& lhs,
    const LocalArrayEffectSourceCoordinate& rhs) {
  const auto lhs_block = lhs.prepared_block_index.value_or(0);
  const auto rhs_block = rhs.prepared_block_index.value_or(0);
  if (lhs_block != rhs_block) {
    return lhs_block < rhs_block ? -1 : 1;
  }
  const auto lhs_inst = lhs.instruction_index.value_or(0);
  const auto rhs_inst = rhs.instruction_index.value_or(0);
  if (lhs_inst != rhs_inst) {
    return lhs_inst < rhs_inst ? -1 : 1;
  }
  if (lhs.tie_break_index != rhs.tie_break_index) {
    return lhs.tie_break_index < rhs.tie_break_index ? -1 : 1;
  }
  if (lhs.bir_block_label != rhs.bir_block_label) {
    return lhs.bir_block_label < rhs.bir_block_label ? -1 : 1;
  }
  return 0;
}

// Dynamic local-array effect scans cover the half-open/closed interval
// (proof_source, endpoint]. The proof-source coordinate is excluded because it
// is the guard fact; the address-materialization endpoint is included because
// it can still redefine or clobber the index before the bounded access exists.
[[nodiscard]] inline bool local_array_effect_source_in_selected_interval(
    const LocalArrayIntervalBoundaryContract& interval,
    const LocalArrayEffectSourceCoordinate& coordinate) {
  return compare_local_array_effect_source_coordinates(
             interval.proof_source,
             coordinate) < 0 &&
         compare_local_array_effect_source_coordinates(coordinate,
                                                       interval.endpoint) <= 0;
}

[[nodiscard]] inline LocalArrayIntervalEffectStatus
local_array_interval_effect_status_for_source(
    const LocalArrayOrderedEffectSourceRecord& source) {
  switch (source.status) {
    case LocalArrayEffectSourceStatus::PreservesIndex:
      return LocalArrayIntervalEffectStatus::Available;
    case LocalArrayEffectSourceStatus::RedefinesIndexValue:
      return LocalArrayIntervalEffectStatus::IndexValueRedefined;
    case LocalArrayEffectSourceStatus::PhiOrAliasUnresolved:
      return LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved;
    case LocalArrayEffectSourceStatus::ClobbersIndex:
      switch (source.family) {
        case LocalArrayEffectSourceFamily::CallOrHelper:
          return LocalArrayIntervalEffectStatus::CallOrHelperClobbersIndex;
        case LocalArrayEffectSourceFamily::InlineAsm:
          return LocalArrayIntervalEffectStatus::InlineAsmClobbersIndex;
        case LocalArrayEffectSourceFamily::Publication:
          return LocalArrayIntervalEffectStatus::PublicationClobbersIndex;
        case LocalArrayEffectSourceFamily::MoveBundle:
          return LocalArrayIntervalEffectStatus::MoveBundleClobbersIndex;
        case LocalArrayEffectSourceFamily::ParallelCopy:
          return LocalArrayIntervalEffectStatus::ParallelCopyClobbersIndex;
        case LocalArrayEffectSourceFamily::IndexDefinition:
          return LocalArrayIntervalEffectStatus::IndexValueRedefined;
        case LocalArrayEffectSourceFamily::PhiOrAliasTransfer:
          return LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved;
        case LocalArrayEffectSourceFamily::Unknown:
          return LocalArrayIntervalEffectStatus::UnknownEffect;
      }
      break;
    case LocalArrayEffectSourceStatus::UnknownModeledEffect:
    case LocalArrayEffectSourceStatus::UnsupportedModeledEffect:
      switch (source.family) {
        case LocalArrayEffectSourceFamily::CallOrHelper:
          return LocalArrayIntervalEffectStatus::CallOrHelperEffectUnknown;
        case LocalArrayEffectSourceFamily::InlineAsm:
          return LocalArrayIntervalEffectStatus::InlineAsmEffectUnknown;
        case LocalArrayEffectSourceFamily::Publication:
          return LocalArrayIntervalEffectStatus::PublicationEffectUnknown;
        case LocalArrayEffectSourceFamily::MoveBundle:
          return LocalArrayIntervalEffectStatus::MoveBundleEffectUnknown;
        case LocalArrayEffectSourceFamily::ParallelCopy:
          return LocalArrayIntervalEffectStatus::ParallelCopyEffectUnknown;
        case LocalArrayEffectSourceFamily::IndexDefinition:
          return LocalArrayIntervalEffectStatus::IndexValueRedefined;
        case LocalArrayEffectSourceFamily::PhiOrAliasTransfer:
          return LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved;
        case LocalArrayEffectSourceFamily::Unknown:
          return LocalArrayIntervalEffectStatus::UnknownEffect;
      }
      break;
  }
  return LocalArrayIntervalEffectStatus::UnknownEffect;
}

[[nodiscard]] inline bool local_array_ordered_effect_source_stream_matches_path(
    const LocalArrayOrderedEffectSourceStream& stream,
    const LocalArraySelectedProofEdgePathRecord& selected_path) {
  if (stream.selected_path == nullptr) {
    return false;
  }
  if (stream.selected_path == &selected_path) {
    return true;
  }
  const auto& stream_path = *stream.selected_path;
  return !stream_path.path_result_name.empty() &&
         stream_path.path_result_name == selected_path.path_result_name &&
         stream_path.source_object_name == selected_path.source_object_name &&
         stream_path.derivation_result_name ==
             selected_path.derivation_result_name &&
         stream_path.lir_producer_lookup_key ==
             selected_path.lir_producer_lookup_key &&
         stream_path.lir_producer_function_name ==
             selected_path.lir_producer_function_name &&
         stream_path.lir_producer_block_label ==
             selected_path.lir_producer_block_label &&
         stream_path.lir_producer_instruction_index ==
             selected_path.lir_producer_instruction_index &&
         stream_path.proof_function_name == selected_path.proof_function_name &&
         stream_path.proof_block_label == selected_path.proof_block_label &&
         stream_path.proof_instruction_index ==
             selected_path.proof_instruction_index &&
         stream_path.selected_successor_label ==
             selected_path.selected_successor_label;
}

[[nodiscard]] inline bool
local_array_ordered_effect_source_stream_matches_endpoint_bridge(
    const LocalArrayOrderedEffectSourceStream& stream,
    const LocalArrayEndpointBridgeRecord& endpoint_bridge) {
  if (stream.endpoint_bridge == nullptr) {
    return false;
  }
  if (stream.endpoint_bridge == &endpoint_bridge) {
    return true;
  }
  const auto& stream_bridge = *stream.endpoint_bridge;
  return !stream_bridge.path_result_name.empty() &&
         stream_bridge.path_result_name == endpoint_bridge.path_result_name &&
         stream_bridge.source_object_name == endpoint_bridge.source_object_name &&
         stream_bridge.derivation_result_name ==
             endpoint_bridge.derivation_result_name &&
         stream_bridge.lir_producer_lookup_key ==
             endpoint_bridge.lir_producer_lookup_key &&
         stream_bridge.prepared_function_name ==
             endpoint_bridge.prepared_function_name &&
         stream_bridge.bir_block_label == endpoint_bridge.bir_block_label &&
         stream_bridge.prepared_block_index ==
             endpoint_bridge.prepared_block_index &&
         stream_bridge.endpoint_instruction_index ==
             endpoint_bridge.endpoint_instruction_index &&
         stream_bridge.result_value_name == endpoint_bridge.result_value_name;
}

struct LocalArrayIntervalEffectInputs {
  const LocalArraySelectedProofEdgePathRecord* selected_path = nullptr;
  const LocalArrayEndpointBridgeRecord* endpoint_bridge = nullptr;
  const LocalArrayOrderedEffectSourceStream* ordered_effect_sources = nullptr;
  bool duplicate_ordered_effect_source_stream = false;
  bool prepared_bir_coordinate_confusion = false;
  bool raw_shape_only = false;
};

struct LocalArrayIntervalEffectRecord {
  LocalArrayIntervalEffectStatus status =
      LocalArrayIntervalEffectStatus::MissingPathValidity;
  const LocalArraySelectedProofEdgePathRecord* selected_path = nullptr;
  std::string lir_producer_lookup_key;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  Value dynamic_index;
  std::string proof_function_name;
  std::string proof_block_label;
  std::string selected_successor_label;
};

[[nodiscard]] inline bool local_array_interval_effect_same_value(
    const Value& lhs,
    const Value& rhs) {
  return !lhs.name.empty() &&
         lhs.kind == rhs.kind &&
         lhs.type == rhs.type &&
         lhs.name == rhs.name;
}

[[nodiscard]] inline LocalArrayIntervalEffectRecord
evaluate_local_array_interval_effect(
    const LocalArrayIntervalEffectInputs& inputs) {
  LocalArrayIntervalEffectRecord record{
      .selected_path = inputs.selected_path,
  };

  if (inputs.raw_shape_only) {
    record.status = LocalArrayIntervalEffectStatus::RawShapeOnly;
    return record;
  }
  if (inputs.prepared_bir_coordinate_confusion) {
    record.status =
        LocalArrayIntervalEffectStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.selected_path == nullptr ||
      inputs.selected_path->status != LocalArraySelectedProofEdgePathStatus::Available) {
    record.status = LocalArrayIntervalEffectStatus::MissingPathValidity;
    return record;
  }

  const auto& selected_path = *inputs.selected_path;
  record.lir_producer_lookup_key = selected_path.lir_producer_lookup_key;
  record.lir_producer_function_name = selected_path.lir_producer_function_name;
  record.lir_producer_block_label = selected_path.lir_producer_block_label;
  record.lir_producer_instruction_index =
      selected_path.lir_producer_instruction_index;
  record.proof_function_name = selected_path.proof_function_name;
  record.proof_block_label = selected_path.proof_block_label;
  record.selected_successor_label = selected_path.selected_successor_label;

  if (selected_path.lir_producer_lookup_key.empty()) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingLirProducerLookupKey;
    return record;
  }
  if (selected_path.lir_producer_coordinate_status !=
      LocalArrayLirProducerCoordinateStatus::Available) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingLirProducerCoordinate;
    return record;
  }
  if (selected_path.lir_producer_operation_role !=
      LocalArrayLirProducerOperationRole::AddressDerivation) {
    record.status = LocalArrayIntervalEffectStatus::UnsupportedLirProducerRole;
    return record;
  }
  if (selected_path.selected_outcome == LocalArraySelectedProofEdgeOutcome::None ||
      selected_path.selected_successor_label.empty()) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingSelectedEdgeOrOutcome;
    return record;
  }
  if (!selected_path.path_validity_known) {
    record.status = LocalArrayIntervalEffectStatus::MissingPathValidity;
    return record;
  }
  if (!selected_path.selected_edge_reaches_lir_producer ||
      !selected_path.selected_edge_covers_lir_producer) {
    record.status =
        LocalArrayIntervalEffectStatus::PathNotCoveringLirProducer;
    return record;
  }

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      selected_path.element_path == nullptr
          ? nullptr
          : single_dynamic_local_array_index(*selected_path.element_path,
                                             &saw_multiple_dynamic_indices);
  if (dynamic_index == nullptr || saw_multiple_dynamic_indices) {
    record.status = LocalArrayIntervalEffectStatus::MissingDynamicIndex;
    return record;
  }
  record.dynamic_index = dynamic_index->value;
  if (!local_array_interval_effect_same_value(selected_path.proof_lhs,
                                              record.dynamic_index) &&
      !local_array_interval_effect_same_value(selected_path.proof_rhs,
                                              record.dynamic_index)) {
    record.status =
        LocalArrayIntervalEffectStatus::DynamicIndexOperandMismatch;
    return record;
  }

  if (selected_path.proof_block_label == selected_path.lir_producer_block_label &&
      (!selected_path.proof_instruction_index.has_value() ||
       !selected_path.lir_producer_instruction_index.has_value() ||
       *selected_path.proof_instruction_index >=
           *selected_path.lir_producer_instruction_index)) {
    record.status = LocalArrayIntervalEffectStatus::MissingSameBlockOrdering;
    return record;
  }
  if (inputs.endpoint_bridge == nullptr ||
      inputs.endpoint_bridge->status != LocalArrayEndpointBridgeStatus::Available) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingPreparedBirEndpointBridge;
    return record;
  }
  const auto& endpoint_bridge = *inputs.endpoint_bridge;
  if (!endpoint_bridge.prepared_block_index.has_value() ||
      !endpoint_bridge.endpoint_instruction_index.has_value() ||
      endpoint_bridge.bir_block_label.empty()) {
    record.status = LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate;
    return record;
  }
  if (inputs.duplicate_ordered_effect_source_stream) {
    record.status =
        LocalArrayIntervalEffectStatus::DuplicateOrderedEffectSourceStream;
    return record;
  }
  if (inputs.ordered_effect_sources == nullptr ||
      inputs.ordered_effect_sources->status ==
          LocalArrayOrderedEffectSourceStreamStatus::MissingBuilder) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream;
    return record;
  }
  const auto& stream = *inputs.ordered_effect_sources;
  if (stream.status == LocalArrayOrderedEffectSourceStreamStatus::MissingLowerBoundaryCoordinate ||
      stream.status == LocalArrayOrderedEffectSourceStreamStatus::MissingEndpointCoordinate ||
      stream.status == LocalArrayOrderedEffectSourceStreamStatus::MissingSourceCoordinate) {
    record.status = LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate;
    return record;
  }
  if (stream.status ==
      LocalArrayOrderedEffectSourceStreamStatus::UnorderedBoundaryCoordinate) {
    record.status = LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary;
    return record;
  }
  if (stream.status ==
      LocalArrayOrderedEffectSourceStreamStatus::UnsupportedModeledEffect) {
    record.status = LocalArrayIntervalEffectStatus::UnknownEffect;
    return record;
  }
  if (stream.status != LocalArrayOrderedEffectSourceStreamStatus::Available) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream;
    return record;
  }
  if (!local_array_ordered_effect_source_stream_matches_path(
          stream, selected_path) ||
      !local_array_ordered_effect_source_stream_matches_endpoint_bridge(
          stream, endpoint_bridge)) {
    record.status =
        LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream;
    return record;
  }
  if (!local_array_effect_source_coordinate_available(
          stream.interval.proof_source) ||
      !local_array_effect_source_coordinate_available(stream.interval.endpoint)) {
    record.status = LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate;
    return record;
  }
  const LocalArrayEffectSourceCoordinate endpoint_coordinate{
      .prepared_block_index = endpoint_bridge.prepared_block_index,
      .bir_block_label = endpoint_bridge.bir_block_label,
      .instruction_index = endpoint_bridge.endpoint_instruction_index,
  };
  if (compare_local_array_effect_source_coordinates(
          stream.interval.endpoint,
          endpoint_coordinate) != 0) {
    record.status = LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate;
    return record;
  }
  if (compare_local_array_effect_source_coordinates(
          stream.interval.proof_source, stream.interval.endpoint) >= 0) {
    record.status = LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary;
    return record;
  }
  for (const auto& source : stream.sources) {
    if (!local_array_effect_source_coordinate_available(source.coordinate)) {
      record.status = LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate;
      return record;
    }
    if (!local_array_effect_source_in_selected_interval(stream.interval,
                                                        source.coordinate)) {
      continue;
    }
    const auto source_status =
        local_array_interval_effect_status_for_source(source);
    if (source_status != LocalArrayIntervalEffectStatus::Available) {
      record.status = source_status;
      return record;
    }
  }

  record.status = LocalArrayIntervalEffectStatus::Available;
  return record;
}

[[nodiscard]] inline LocalArraySelectedProofEdgePathRecord
evaluate_local_array_selected_proof_edge_path(
    const LocalArraySelectedProofEdgePathInputs& inputs) {
  LocalArraySelectedProofEdgePathRecord record{
      .element_path = inputs.element_path,
      .proof_function_name = inputs.proof_function_name,
      .proof_block_label = inputs.proof_block_label,
      .proof_condition_value = inputs.proof_condition_value,
      .proof_predicate = inputs.proof_predicate,
      .proof_compare_type = inputs.proof_compare_type,
      .proof_lhs = inputs.proof_lhs,
      .proof_rhs = inputs.proof_rhs,
      .proof_instruction_index = inputs.proof_instruction_index,
      .bound_contribution = inputs.bound_contribution,
      .normalized_bound = inputs.normalized_bound,
      .bound_inclusive = inputs.bound_inclusive,
      .selected_outcome = inputs.selected_outcome,
      .selected_successor_label = inputs.selected_successor_label,
      .non_selected_successor_label = inputs.non_selected_successor_label,
      .path_validity_known = inputs.path_validity_known,
      .selected_edge_reaches_lir_producer =
          inputs.selected_edge_reaches_lir_producer,
      .selected_edge_covers_lir_producer =
          inputs.selected_edge_covers_lir_producer,
      .proof_dominates_lir_producer = inputs.proof_dominates_lir_producer,
      .proof_guards_lir_producer = inputs.proof_guards_lir_producer,
  };

  if (inputs.raw_shape_only) {
    record.status = LocalArraySelectedProofEdgePathStatus::RawShapeOnly;
    return record;
  }
  if (inputs.target_or_final_home_only) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::TargetOrFinalHomeOnly;
    return record;
  }
  if (inputs.unsupported_boundary) {
    record.status = LocalArraySelectedProofEdgePathStatus::UnsupportedBoundary;
    return record;
  }
  if (inputs.prepared_bir_coordinate_confusion) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.element_path == nullptr) {
    record.status = LocalArraySelectedProofEdgePathStatus::MissingLocalArrayPath;
    return record;
  }

  record.path_result_name = inputs.element_path->result_name;
  record.source_object_name = inputs.element_path->source_object_name;
  record.derivation_result_name = inputs.element_path->derivation_result_name;
  record.lir_producer_function_name =
      inputs.element_path->lir_producer_function_name;
  record.lir_producer_block_label =
      inputs.element_path->lir_producer_block_label;
  record.lir_producer_instruction_index =
      inputs.element_path->lir_producer_instruction_index;
  record.lir_producer_operation_role =
      inputs.element_path->lir_producer_operation_role;
  record.lir_producer_lookup_key = inputs.element_path->lir_producer_lookup_key;
  record.lir_producer_coordinate_status =
      inputs.element_path->lir_producer_coordinate_status;

  if (record.lir_producer_lookup_key.empty()) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::MissingLirProducerLookupKey;
    return record;
  }
  if (record.lir_producer_coordinate_status !=
      LocalArrayLirProducerCoordinateStatus::Available) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::MissingLirProducerCoordinate;
    return record;
  }
  if (record.lir_producer_operation_role !=
      LocalArrayLirProducerOperationRole::AddressDerivation) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::UnsupportedLirProducerRole;
    return record;
  }
  if (!inputs.proof_source_available) {
    record.status = LocalArraySelectedProofEdgePathStatus::MissingProofSource;
    return record;
  }
  if (record.proof_function_name != record.lir_producer_function_name) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::ProofFunctionMismatch;
    return record;
  }
  if (inputs.selected_outcome == LocalArraySelectedProofEdgeOutcome::None) {
    record.status = LocalArraySelectedProofEdgePathStatus::MissingSelectedOutcome;
    return record;
  }
  if (inputs.selected_successor_label.empty() ||
      inputs.non_selected_successor_label.empty()) {
    record.status = LocalArraySelectedProofEdgePathStatus::MissingSelectedEdge;
    return record;
  }
  if (inputs.same_block_candidate && !inputs.same_block_ordering_known) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::MissingSameBlockOrdering;
    return record;
  }
  if (!inputs.path_validity_known ||
      !inputs.selected_edge_reaches_lir_producer ||
      !inputs.selected_edge_covers_lir_producer) {
    record.status = LocalArraySelectedProofEdgePathStatus::NonCoveringPath;
    return record;
  }
  if (!inputs.proof_dominates_lir_producer &&
      !inputs.proof_guards_lir_producer) {
    record.status =
        LocalArraySelectedProofEdgePathStatus::NonDominatingOrGuardingProof;
    return record;
  }

  record.status = LocalArraySelectedProofEdgePathStatus::Available;
  return record;
}

enum class LocalArrayRangeProofStatus : unsigned char {
  Available,
  MissingLocalArrayPath,
  MissingProofFact,
  MissingRangeProofCertificate,
  MissingSelectedProofEdgePath,
  SelectedPathOnlyInference,
  MissingIntervalEffect,
  IntervalEffectOnlyInference,
  MissingDynamicIndex,
  MissingProofSource,
  UnsupportedProofSource,
  MissingLirProducerCoordinate,
  PreparedBirCoordinateConfusion,
  MissingLowerBound,
  MissingUpperBound,
  UnsupportedPredicate,
  UnsupportedIndexWidth,
  OperandRoleMismatch,
  BoundValueMismatch,
  ProofFunctionMismatch,
  ProofNotDominatingConsumer,
  PathNotCoveringConsumer,
  MissingPathValidity,
  MissingNoClobber,
  IndexValueClobbered,
  IndexValueRedefined,
  IndexPhiOrAliasUnresolved,
  CallOrHelperEffectUnknown,
  CallOrHelperClobbersIndex,
  InlineAsmEffectUnknown,
  InlineAsmClobbersIndex,
  PublicationOrMoveEffectUnknown,
  PublicationOrMoveClobbersIndex,
  RawShapeOnly,
  TargetOnlyOrFinalHomeOnly,
  UnsupportedBoundary,
};

[[nodiscard]] constexpr std::string_view local_array_range_proof_status_name(
    LocalArrayRangeProofStatus status) {
  switch (status) {
    case LocalArrayRangeProofStatus::Available:
      return "available";
    case LocalArrayRangeProofStatus::MissingLocalArrayPath:
      return "missing_local_array_path";
    case LocalArrayRangeProofStatus::MissingProofFact:
      return "missing_proof_fact";
    case LocalArrayRangeProofStatus::MissingRangeProofCertificate:
      return "missing_range_proof_certificate";
    case LocalArrayRangeProofStatus::MissingSelectedProofEdgePath:
      return "missing_selected_proof_edge_path";
    case LocalArrayRangeProofStatus::SelectedPathOnlyInference:
      return "selected_path_only_inference";
    case LocalArrayRangeProofStatus::MissingIntervalEffect:
      return "missing_interval_effect";
    case LocalArrayRangeProofStatus::IntervalEffectOnlyInference:
      return "interval_effect_only_inference";
    case LocalArrayRangeProofStatus::MissingDynamicIndex:
      return "missing_dynamic_index";
    case LocalArrayRangeProofStatus::MissingProofSource:
      return "missing_proof_source";
    case LocalArrayRangeProofStatus::UnsupportedProofSource:
      return "unsupported_proof_source";
    case LocalArrayRangeProofStatus::MissingLirProducerCoordinate:
      return "missing_lir_producer_coordinate";
    case LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion:
      return "prepared_bir_coordinate_confusion";
    case LocalArrayRangeProofStatus::MissingLowerBound:
      return "missing_lower_bound";
    case LocalArrayRangeProofStatus::MissingUpperBound:
      return "missing_upper_bound";
    case LocalArrayRangeProofStatus::UnsupportedPredicate:
      return "unsupported_predicate";
    case LocalArrayRangeProofStatus::UnsupportedIndexWidth:
      return "unsupported_index_width";
    case LocalArrayRangeProofStatus::OperandRoleMismatch:
      return "operand_role_mismatch";
    case LocalArrayRangeProofStatus::BoundValueMismatch:
      return "bound_value_mismatch";
    case LocalArrayRangeProofStatus::ProofFunctionMismatch:
      return "proof_function_mismatch";
    case LocalArrayRangeProofStatus::ProofNotDominatingConsumer:
      return "proof_not_dominating_consumer";
    case LocalArrayRangeProofStatus::PathNotCoveringConsumer:
      return "path_not_covering_consumer";
    case LocalArrayRangeProofStatus::MissingPathValidity:
      return "missing_path_validity";
    case LocalArrayRangeProofStatus::MissingNoClobber:
      return "missing_no_clobber";
    case LocalArrayRangeProofStatus::IndexValueClobbered:
      return "index_value_clobbered";
    case LocalArrayRangeProofStatus::IndexValueRedefined:
      return "index_value_redefined";
    case LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved:
      return "index_phi_or_alias_unresolved";
    case LocalArrayRangeProofStatus::CallOrHelperEffectUnknown:
      return "call_or_helper_effect_unknown";
    case LocalArrayRangeProofStatus::CallOrHelperClobbersIndex:
      return "call_or_helper_clobbers_index";
    case LocalArrayRangeProofStatus::InlineAsmEffectUnknown:
      return "inline_asm_effect_unknown";
    case LocalArrayRangeProofStatus::InlineAsmClobbersIndex:
      return "inline_asm_clobbers_index";
    case LocalArrayRangeProofStatus::PublicationOrMoveEffectUnknown:
      return "publication_or_move_effect_unknown";
    case LocalArrayRangeProofStatus::PublicationOrMoveClobbersIndex:
      return "publication_or_move_clobbers_index";
    case LocalArrayRangeProofStatus::RawShapeOnly:
      return "raw_shape_only";
    case LocalArrayRangeProofStatus::TargetOnlyOrFinalHomeOnly:
      return "target_only_or_final_home_only";
    case LocalArrayRangeProofStatus::UnsupportedBoundary:
      return "unsupported_boundary";
  }
  return "unknown";
}

enum class LocalArrayRangeProofSourceKind : unsigned char {
  None,
  BranchCondition,
  ExplicitCompare,
};

enum class LocalArrayRangeProofPredicate : unsigned char {
  Unknown,
  Slt,
  Sle,
  Sge,
  Uge,
  Ult,
  Ule,
};

struct LocalArrayIndexRangeProofInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  std::string consumer_function_name;
  std::string proof_function_name;
  std::string consumer_block_label;
  std::string proof_block_label;
  std::optional<std::size_t> consumer_instruction_index;
  std::optional<std::size_t> proof_instruction_index;
  LocalArrayRangeProofSourceKind proof_source_kind =
      LocalArrayRangeProofSourceKind::None;
  Value proof_lhs;
  Value proof_rhs;
  TypeKind compare_type = TypeKind::Void;
  LocalArrayRangeProofPredicate lower_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  LocalArrayRangeProofPredicate upper_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  bool lower_bound_available = false;
  std::int64_t lower_bound = 0;
  bool lower_bound_inclusive = true;
  bool upper_bound_available = false;
  std::size_t upper_bound = 0;
  bool upper_bound_exclusive = true;
  bool operand_roles_match_index = true;
  bool path_validity_known = false;
  bool proof_dominates_consumer = false;
  bool path_covers_consumer = false;
  bool no_clobber_known = false;
  bool index_value_clobbered = false;
  bool index_value_redefined = false;
  bool index_phi_or_alias_unresolved = false;
  bool call_or_helper_effect_unknown = false;
  bool call_or_helper_clobbers_index = false;
  bool inline_asm_effect_unknown = false;
  bool publication_or_move_effect_unknown = false;
  bool publication_or_move_clobbers_index = false;
  bool raw_shape_only = false;
  bool target_only_or_final_home_only = false;
  bool unsupported_boundary = false;
};

struct LocalArrayIndexRangeProofRecord {
  LocalArrayRangeProofStatus status =
      LocalArrayRangeProofStatus::MissingLocalArrayPath;
  const LocalArrayElementPathRecord* element_path = nullptr;
  Value dynamic_index;
  std::string consumer_function_name;
  std::string proof_function_name;
  std::string consumer_block_label;
  std::string proof_block_label;
  std::optional<std::size_t> consumer_instruction_index;
  std::optional<std::size_t> proof_instruction_index;
  LocalArrayRangeProofSourceKind proof_source_kind =
      LocalArrayRangeProofSourceKind::None;
  Value proof_lhs;
  Value proof_rhs;
  TypeKind compare_type = TypeKind::Void;
  LocalArrayRangeProofPredicate lower_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  LocalArrayRangeProofPredicate upper_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  std::int64_t normalized_lower_bound = 0;
  std::size_t normalized_upper_bound = 0;
  bool lower_bound_inclusive = true;
  bool upper_bound_exclusive = true;
  bool path_validity_known = false;
  bool proof_dominates_consumer = false;
  bool path_covers_consumer = false;
  bool no_clobber_known = false;
};

struct LocalArrayRangeProofCertificateInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArraySelectedProofEdgePathRecord* selected_path = nullptr;
  const LocalArrayIntervalEffectRecord* interval_effect = nullptr;
  bool interval_effect_only = false;
};

struct LocalArrayProofFactInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayIndexRangeProofRecord* range_proof = nullptr;
};

struct LocalArrayProofFactRecord {
  LocalArrayRangeProofStatus status =
      LocalArrayRangeProofStatus::MissingLocalArrayPath;
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayIndexRangeProofRecord* range_proof = nullptr;
  Value dynamic_index;
  std::string lir_producer_lookup_key;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  std::string proof_function_name;
  std::string proof_block_label;
  std::optional<std::size_t> proof_instruction_index;
  LocalArrayRangeProofSourceKind proof_source_kind =
      LocalArrayRangeProofSourceKind::None;
  Value proof_lhs;
  Value proof_rhs;
  TypeKind compare_type = TypeKind::Void;
  LocalArrayRangeProofPredicate lower_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  LocalArrayRangeProofPredicate upper_predicate =
      LocalArrayRangeProofPredicate::Unknown;
  std::int64_t normalized_lower_bound = 0;
  std::size_t normalized_upper_bound = 0;
  bool lower_bound_inclusive = true;
  bool upper_bound_exclusive = true;
  bool path_validity_known = false;
  bool proof_dominates_consumer = false;
  bool path_covers_consumer = false;
  bool no_clobber_known = false;
};

struct LocalArrayIndexRangeCheckerInputInputs {
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayProofFactRecord* proof_fact = nullptr;
};

struct LocalArrayIndexRangeCheckerInputRecord {
  LocalArrayRangeProofStatus status =
      LocalArrayRangeProofStatus::MissingLocalArrayPath;
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayProofFactRecord* proof_fact = nullptr;
  LocalArrayIndexRangeProofInputs inputs;
  LocalArrayIndexRangeProofRecord checker_record;
};

struct LocalArrayLocalAddressProvenanceInputs {
  const LocalArraySourceObjectRecord* source_object = nullptr;
  const LocalArrayAddressDerivationRecord* derivation = nullptr;
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayIndexRangeCheckerInputRecord* checker_input = nullptr;
};

struct LocalArrayLocalAddressProvenanceRecord {
  LocalArrayCarrierStatus status = LocalArrayCarrierStatus::MissingElementPath;
  LocalArrayRangeProofStatus checker_status =
      LocalArrayRangeProofStatus::MissingLocalArrayPath;
  const LocalArraySourceObjectRecord* source_object = nullptr;
  const LocalArrayAddressDerivationRecord* derivation = nullptr;
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayIndexRangeCheckerInputRecord* checker_input = nullptr;
  std::string source_object_name;
  std::string derived_pointer_name;
  std::string element_result_name;
  std::string lir_producer_lookup_key;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  LocalArrayDerivationKind derivation_kind = LocalArrayDerivationKind::Unknown;
  Value dynamic_index;
  TypeKind element_type = TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t source_total_size_bytes = 0;
  bool scalar_in_bounds = false;
};

struct LocalArraySemanticGepInputs {
  const LocalArrayLocalAddressProvenanceRecord* provenance = nullptr;
};

struct LocalArraySemanticGepRecord {
  LocalArrayCarrierStatus status = LocalArrayCarrierStatus::MissingElementPath;
  LocalArrayRangeProofStatus checker_status =
      LocalArrayRangeProofStatus::MissingLocalArrayPath;
  const LocalArrayLocalAddressProvenanceRecord* provenance = nullptr;
  const LocalArraySourceObjectRecord* source_object = nullptr;
  const LocalArrayAddressDerivationRecord* derivation = nullptr;
  const LocalArrayElementPathRecord* element_path = nullptr;
  const LocalArrayIndexRangeCheckerInputRecord* checker_input = nullptr;
  std::string source_object_name;
  std::string derived_pointer_name;
  std::string element_result_name;
  std::string lir_producer_lookup_key;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  LocalArrayDerivationKind derivation_kind = LocalArrayDerivationKind::Unknown;
  LocalArrayLirProducerOperationRole lir_producer_operation_role =
      LocalArrayLirProducerOperationRole::None;
  LocalArrayLirProducerCoordinateStatus lir_producer_coordinate_status =
      LocalArrayLirProducerCoordinateStatus::MissingLirProducerCoordinate;
  Value dynamic_index;
  TypeKind element_type = TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t source_total_size_bytes = 0;
  bool scalar_in_bounds = false;
};

[[nodiscard]] inline const LocalArrayIndexRecord* single_dynamic_local_array_index(
    const LocalArrayElementPathRecord& path,
    bool* saw_multiple = nullptr) {
  const LocalArrayIndexRecord* dynamic_index = nullptr;
  if (saw_multiple != nullptr) {
    *saw_multiple = false;
  }
  for (const auto& index : path.indices) {
    if (index.kind != LocalArrayIndexKind::Dynamic) {
      continue;
    }
    if (dynamic_index != nullptr) {
      if (saw_multiple != nullptr) {
        *saw_multiple = true;
      }
      return dynamic_index;
    }
    dynamic_index = &index;
  }
  return dynamic_index;
}

[[nodiscard]] inline bool local_array_source_object_matches_element_path(
    const LocalArraySourceObjectRecord& source_object,
    const LocalArrayElementPathRecord& element_path) {
  return !source_object.object_name.empty() &&
         source_object.object_name == element_path.source_object_name;
}

[[nodiscard]] inline bool local_array_derivation_matches_element_path(
    const LocalArrayAddressDerivationRecord& derivation,
    const LocalArrayElementPathRecord& element_path) {
  return !derivation.result_name.empty() &&
         derivation.result_name == element_path.derivation_result_name &&
         derivation.source_object_name == element_path.source_object_name;
}

[[nodiscard]] constexpr bool local_array_range_proof_index_type_supported(
    TypeKind type) {
  return type == TypeKind::I32 || type == TypeKind::I64;
}

[[nodiscard]] constexpr bool local_array_range_proof_lower_predicate_supported(
    LocalArrayRangeProofPredicate predicate) {
  return predicate == LocalArrayRangeProofPredicate::Sge ||
         predicate == LocalArrayRangeProofPredicate::Uge;
}

[[nodiscard]] constexpr bool local_array_range_proof_upper_predicate_supported(
    LocalArrayRangeProofPredicate predicate) {
  return predicate == LocalArrayRangeProofPredicate::Slt ||
         predicate == LocalArrayRangeProofPredicate::Ult;
}

[[nodiscard]] inline LocalArrayIndexRangeProofRecord
evaluate_local_array_index_range_proof(
    const LocalArrayIndexRangeProofInputs& inputs) {
  LocalArrayIndexRangeProofRecord record{
      .element_path = inputs.element_path,
      .consumer_function_name = inputs.consumer_function_name,
      .proof_function_name = inputs.proof_function_name,
      .consumer_block_label = inputs.consumer_block_label,
      .proof_block_label = inputs.proof_block_label,
      .consumer_instruction_index = inputs.consumer_instruction_index,
      .proof_instruction_index = inputs.proof_instruction_index,
      .proof_source_kind = inputs.proof_source_kind,
      .proof_lhs = inputs.proof_lhs,
      .proof_rhs = inputs.proof_rhs,
      .compare_type = inputs.compare_type,
      .lower_predicate = inputs.lower_predicate,
      .upper_predicate = inputs.upper_predicate,
      .normalized_lower_bound = inputs.lower_bound,
      .normalized_upper_bound = inputs.upper_bound,
      .lower_bound_inclusive = inputs.lower_bound_inclusive,
      .upper_bound_exclusive = inputs.upper_bound_exclusive,
      .path_validity_known = inputs.path_validity_known,
      .proof_dominates_consumer = inputs.proof_dominates_consumer,
      .path_covers_consumer = inputs.path_covers_consumer,
      .no_clobber_known = inputs.no_clobber_known,
  };

  if (inputs.raw_shape_only) {
    record.status = LocalArrayRangeProofStatus::RawShapeOnly;
    return record;
  }
  if (inputs.target_only_or_final_home_only) {
    record.status = LocalArrayRangeProofStatus::TargetOnlyOrFinalHomeOnly;
    return record;
  }
  if (inputs.unsupported_boundary) {
    record.status = LocalArrayRangeProofStatus::UnsupportedBoundary;
    return record;
  }
  if (inputs.element_path == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingLocalArrayPath;
    return record;
  }

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(*inputs.element_path, &saw_multiple_dynamic_indices);
  if (saw_multiple_dynamic_indices || inputs.index_phi_or_alias_unresolved) {
    record.status = LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved;
    return record;
  }
  if (dynamic_index == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingDynamicIndex;
    return record;
  }
  record.dynamic_index = dynamic_index->value;

  if (inputs.proof_source_kind == LocalArrayRangeProofSourceKind::None) {
    record.status = LocalArrayRangeProofStatus::MissingProofSource;
    return record;
  }
  if (inputs.proof_source_kind != LocalArrayRangeProofSourceKind::BranchCondition &&
      inputs.proof_source_kind != LocalArrayRangeProofSourceKind::ExplicitCompare) {
    record.status = LocalArrayRangeProofStatus::UnsupportedProofSource;
    return record;
  }
  if (!inputs.consumer_function_name.empty() &&
      !inputs.proof_function_name.empty() &&
      inputs.consumer_function_name != inputs.proof_function_name) {
    record.status = LocalArrayRangeProofStatus::ProofFunctionMismatch;
    return record;
  }
  if (!inputs.lower_bound_available) {
    record.status = LocalArrayRangeProofStatus::MissingLowerBound;
    return record;
  }
  if (!inputs.upper_bound_available) {
    record.status = LocalArrayRangeProofStatus::MissingUpperBound;
    return record;
  }
  if (!local_array_range_proof_lower_predicate_supported(inputs.lower_predicate) ||
      !local_array_range_proof_upper_predicate_supported(inputs.upper_predicate)) {
    record.status = LocalArrayRangeProofStatus::UnsupportedPredicate;
    return record;
  }
  if (!local_array_range_proof_index_type_supported(dynamic_index->value.type) ||
      inputs.compare_type != dynamic_index->value.type) {
    record.status = LocalArrayRangeProofStatus::UnsupportedIndexWidth;
    return record;
  }
  if (!inputs.operand_roles_match_index) {
    record.status = LocalArrayRangeProofStatus::OperandRoleMismatch;
    return record;
  }
  if (inputs.lower_bound != 0 ||
      !inputs.lower_bound_inclusive ||
      inputs.upper_bound != inputs.element_path->element_count ||
      !inputs.upper_bound_exclusive) {
    record.status = LocalArrayRangeProofStatus::BoundValueMismatch;
    return record;
  }
  if (!inputs.path_validity_known) {
    record.status = LocalArrayRangeProofStatus::MissingPathValidity;
    return record;
  }
  if (!inputs.proof_dominates_consumer) {
    record.status = LocalArrayRangeProofStatus::ProofNotDominatingConsumer;
    return record;
  }
  if (!inputs.path_covers_consumer) {
    record.status = LocalArrayRangeProofStatus::PathNotCoveringConsumer;
    return record;
  }
  if (!inputs.no_clobber_known) {
    record.status = LocalArrayRangeProofStatus::MissingNoClobber;
    return record;
  }
  if (inputs.index_value_clobbered) {
    record.status = LocalArrayRangeProofStatus::IndexValueClobbered;
    return record;
  }
  if (inputs.index_value_redefined) {
    record.status = LocalArrayRangeProofStatus::IndexValueRedefined;
    return record;
  }
  if (inputs.call_or_helper_effect_unknown) {
    record.status = LocalArrayRangeProofStatus::CallOrHelperEffectUnknown;
    return record;
  }
  if (inputs.call_or_helper_clobbers_index) {
    record.status = LocalArrayRangeProofStatus::CallOrHelperClobbersIndex;
    return record;
  }
  if (inputs.inline_asm_effect_unknown) {
    record.status = LocalArrayRangeProofStatus::InlineAsmEffectUnknown;
    return record;
  }
  if (inputs.publication_or_move_effect_unknown) {
    record.status = LocalArrayRangeProofStatus::PublicationOrMoveEffectUnknown;
    return record;
  }
  if (inputs.publication_or_move_clobbers_index) {
    record.status = LocalArrayRangeProofStatus::PublicationOrMoveClobbersIndex;
    return record;
  }

  record.status = LocalArrayRangeProofStatus::Available;
  return record;
}

[[nodiscard]] inline bool local_array_range_proof_same_value(
    const Value& lhs,
    const Value& rhs) {
  return !lhs.name.empty() &&
         lhs.kind == rhs.kind &&
         lhs.type == rhs.type &&
         lhs.name == rhs.name;
}

[[nodiscard]] inline bool local_array_selected_proof_edge_path_matches_element_path(
    const LocalArraySelectedProofEdgePathRecord& selected_path,
    const LocalArrayElementPathRecord& element_path) {
  if (selected_path.element_path == &element_path) {
    return true;
  }
  return !selected_path.path_result_name.empty() &&
         selected_path.path_result_name == element_path.result_name &&
         selected_path.source_object_name == element_path.source_object_name &&
         selected_path.derivation_result_name ==
             element_path.derivation_result_name &&
         selected_path.lir_producer_lookup_key ==
             element_path.lir_producer_lookup_key &&
         selected_path.lir_producer_function_name ==
             element_path.lir_producer_function_name &&
         selected_path.lir_producer_block_label ==
             element_path.lir_producer_block_label &&
         selected_path.lir_producer_instruction_index ==
             element_path.lir_producer_instruction_index;
}

[[nodiscard]] inline bool local_array_range_proof_matches_element_path(
    const LocalArrayIndexRangeProofRecord& range_proof,
    const LocalArrayElementPathRecord& element_path) {
  if (range_proof.element_path == &element_path) {
    return true;
  }
  if (range_proof.element_path == nullptr) {
    return false;
  }
  return !range_proof.element_path->result_name.empty() &&
         range_proof.element_path->result_name == element_path.result_name &&
         range_proof.element_path->source_object_name ==
             element_path.source_object_name &&
         range_proof.element_path->derivation_result_name ==
             element_path.derivation_result_name &&
         range_proof.element_path->lir_producer_lookup_key ==
             element_path.lir_producer_lookup_key &&
         range_proof.element_path->lir_producer_function_name ==
             element_path.lir_producer_function_name &&
         range_proof.element_path->lir_producer_block_label ==
             element_path.lir_producer_block_label &&
         range_proof.element_path->lir_producer_instruction_index ==
             element_path.lir_producer_instruction_index;
}

[[nodiscard]] inline bool local_array_interval_effect_matches_selected_path(
    const LocalArrayIntervalEffectRecord& interval_effect,
    const LocalArraySelectedProofEdgePathRecord& selected_path) {
  if (interval_effect.selected_path == &selected_path) {
    return true;
  }
  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      selected_path.element_path == nullptr
          ? nullptr
          : single_dynamic_local_array_index(*selected_path.element_path,
                                             &saw_multiple_dynamic_indices);
  return dynamic_index != nullptr &&
         !saw_multiple_dynamic_indices &&
         interval_effect.lir_producer_lookup_key ==
             selected_path.lir_producer_lookup_key &&
         interval_effect.lir_producer_function_name ==
             selected_path.lir_producer_function_name &&
         interval_effect.lir_producer_block_label ==
             selected_path.lir_producer_block_label &&
         interval_effect.lir_producer_instruction_index ==
             selected_path.lir_producer_instruction_index &&
         interval_effect.proof_function_name ==
             selected_path.proof_function_name &&
         interval_effect.proof_block_label == selected_path.proof_block_label &&
         interval_effect.selected_successor_label ==
             selected_path.selected_successor_label &&
         local_array_range_proof_same_value(interval_effect.dynamic_index,
                                            dynamic_index->value);
}

[[nodiscard]] constexpr LocalArrayRangeProofStatus
local_array_range_proof_status_from_selected_path(
    LocalArraySelectedProofEdgePathStatus status) {
  switch (status) {
    case LocalArraySelectedProofEdgePathStatus::Available:
      return LocalArrayRangeProofStatus::MissingIntervalEffect;
    case LocalArraySelectedProofEdgePathStatus::MissingLocalArrayPath:
      return LocalArrayRangeProofStatus::MissingLocalArrayPath;
    case LocalArraySelectedProofEdgePathStatus::MissingLirProducerLookupKey:
    case LocalArraySelectedProofEdgePathStatus::MissingLirProducerCoordinate:
      return LocalArrayRangeProofStatus::MissingLirProducerCoordinate;
    case LocalArraySelectedProofEdgePathStatus::UnsupportedLirProducerRole:
      return LocalArrayRangeProofStatus::UnsupportedProofSource;
    case LocalArraySelectedProofEdgePathStatus::MissingProofSource:
      return LocalArrayRangeProofStatus::MissingProofSource;
    case LocalArraySelectedProofEdgePathStatus::ProofFunctionMismatch:
      return LocalArrayRangeProofStatus::ProofFunctionMismatch;
    case LocalArraySelectedProofEdgePathStatus::MissingSelectedEdge:
    case LocalArraySelectedProofEdgePathStatus::MissingSelectedOutcome:
      return LocalArrayRangeProofStatus::MissingPathValidity;
    case LocalArraySelectedProofEdgePathStatus::NonCoveringPath:
      return LocalArrayRangeProofStatus::PathNotCoveringConsumer;
    case LocalArraySelectedProofEdgePathStatus::NonDominatingOrGuardingProof:
      return LocalArrayRangeProofStatus::ProofNotDominatingConsumer;
    case LocalArraySelectedProofEdgePathStatus::UnsupportedBoundary:
    case LocalArraySelectedProofEdgePathStatus::MissingSameBlockOrdering:
      return LocalArrayRangeProofStatus::UnsupportedBoundary;
    case LocalArraySelectedProofEdgePathStatus::PreparedBirCoordinateConfusion:
      return LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    case LocalArraySelectedProofEdgePathStatus::RawShapeOnly:
      return LocalArrayRangeProofStatus::RawShapeOnly;
    case LocalArraySelectedProofEdgePathStatus::TargetOrFinalHomeOnly:
      return LocalArrayRangeProofStatus::TargetOnlyOrFinalHomeOnly;
  }
  return LocalArrayRangeProofStatus::MissingSelectedProofEdgePath;
}

[[nodiscard]] constexpr LocalArrayRangeProofStatus
local_array_range_proof_status_from_interval_effect(
    LocalArrayIntervalEffectStatus status) {
  switch (status) {
    case LocalArrayIntervalEffectStatus::Available:
      return LocalArrayRangeProofStatus::Available;
    case LocalArrayIntervalEffectStatus::MissingLirProducerLookupKey:
    case LocalArrayIntervalEffectStatus::MissingLirProducerCoordinate:
    case LocalArrayIntervalEffectStatus::MissingEffectSourceCoordinate:
      return LocalArrayRangeProofStatus::MissingLirProducerCoordinate;
    case LocalArrayIntervalEffectStatus::UnsupportedLirProducerRole:
      return LocalArrayRangeProofStatus::UnsupportedProofSource;
    case LocalArrayIntervalEffectStatus::MissingDynamicIndex:
      return LocalArrayRangeProofStatus::MissingDynamicIndex;
    case LocalArrayIntervalEffectStatus::DynamicIndexOperandMismatch:
      return LocalArrayRangeProofStatus::OperandRoleMismatch;
    case LocalArrayIntervalEffectStatus::MissingSelectedEdgeOrOutcome:
    case LocalArrayIntervalEffectStatus::MissingPathValidity:
      return LocalArrayRangeProofStatus::MissingPathValidity;
    case LocalArrayIntervalEffectStatus::PathNotCoveringLirProducer:
      return LocalArrayRangeProofStatus::PathNotCoveringConsumer;
    case LocalArrayIntervalEffectStatus::MissingPreparedBirEndpointBridge:
    case LocalArrayIntervalEffectStatus::MissingSameBlockOrdering:
    case LocalArrayIntervalEffectStatus::UnorderedEffectSourceBoundary:
      return LocalArrayRangeProofStatus::UnsupportedBoundary;
    case LocalArrayIntervalEffectStatus::PreparedBirCoordinateConfusion:
      return LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    case LocalArrayIntervalEffectStatus::SelectedPathOnlyInference:
      return LocalArrayRangeProofStatus::SelectedPathOnlyInference;
    case LocalArrayIntervalEffectStatus::MissingOrderedEffectSourceStream:
    case LocalArrayIntervalEffectStatus::DuplicateOrderedEffectSourceStream:
      return LocalArrayRangeProofStatus::MissingIntervalEffect;
    case LocalArrayIntervalEffectStatus::IndexValueRedefined:
      return LocalArrayRangeProofStatus::IndexValueRedefined;
    case LocalArrayIntervalEffectStatus::IndexPhiOrAliasUnresolved:
      return LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved;
    case LocalArrayIntervalEffectStatus::CallOrHelperEffectUnknown:
      return LocalArrayRangeProofStatus::CallOrHelperEffectUnknown;
    case LocalArrayIntervalEffectStatus::CallOrHelperClobbersIndex:
      return LocalArrayRangeProofStatus::CallOrHelperClobbersIndex;
    case LocalArrayIntervalEffectStatus::InlineAsmEffectUnknown:
      return LocalArrayRangeProofStatus::InlineAsmEffectUnknown;
    case LocalArrayIntervalEffectStatus::InlineAsmClobbersIndex:
      return LocalArrayRangeProofStatus::InlineAsmClobbersIndex;
    case LocalArrayIntervalEffectStatus::PublicationEffectUnknown:
    case LocalArrayIntervalEffectStatus::MoveBundleEffectUnknown:
    case LocalArrayIntervalEffectStatus::ParallelCopyEffectUnknown:
    case LocalArrayIntervalEffectStatus::UnknownEffect:
      return LocalArrayRangeProofStatus::PublicationOrMoveEffectUnknown;
    case LocalArrayIntervalEffectStatus::PublicationClobbersIndex:
    case LocalArrayIntervalEffectStatus::MoveBundleClobbersIndex:
    case LocalArrayIntervalEffectStatus::ParallelCopyClobbersIndex:
      return LocalArrayRangeProofStatus::PublicationOrMoveClobbersIndex;
    case LocalArrayIntervalEffectStatus::RawShapeOnly:
      return LocalArrayRangeProofStatus::RawShapeOnly;
  }
  return LocalArrayRangeProofStatus::MissingIntervalEffect;
}

[[nodiscard]] inline std::optional<LocalArrayRangeProofPredicate>
local_array_range_upper_predicate_from_selected_path(
    LocalArraySelectedProofEdgePredicate predicate) {
  switch (predicate) {
    case LocalArraySelectedProofEdgePredicate::Ult:
    case LocalArraySelectedProofEdgePredicate::Ule:
      return LocalArrayRangeProofPredicate::Ult;
    case LocalArraySelectedProofEdgePredicate::Slt:
    case LocalArraySelectedProofEdgePredicate::Sle:
      return LocalArrayRangeProofPredicate::Slt;
    case LocalArraySelectedProofEdgePredicate::Unknown:
    case LocalArraySelectedProofEdgePredicate::Sgt:
    case LocalArraySelectedProofEdgePredicate::Sge:
    case LocalArraySelectedProofEdgePredicate::Ugt:
    case LocalArraySelectedProofEdgePredicate::Uge:
    case LocalArraySelectedProofEdgePredicate::Eq:
    case LocalArraySelectedProofEdgePredicate::Ne:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] inline LocalArrayIndexRangeProofRecord
evaluate_local_array_index_range_proof_certificate(
    const LocalArrayRangeProofCertificateInputs& inputs) {
  LocalArrayIndexRangeProofRecord record{.element_path = inputs.element_path};
  if (inputs.element_path == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingLocalArrayPath;
    return record;
  }
  if (inputs.interval_effect_only) {
    record.status = LocalArrayRangeProofStatus::IntervalEffectOnlyInference;
    return record;
  }
  if (inputs.selected_path == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingSelectedProofEdgePath;
    return record;
  }
  if (!local_array_selected_proof_edge_path_matches_element_path(
          *inputs.selected_path, *inputs.element_path)) {
    record.status = LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.selected_path->status !=
      LocalArraySelectedProofEdgePathStatus::Available) {
    record.status = local_array_range_proof_status_from_selected_path(
        inputs.selected_path->status);
    return record;
  }
  if (inputs.interval_effect == nullptr) {
    record.status = LocalArrayRangeProofStatus::SelectedPathOnlyInference;
    return record;
  }
  if (!local_array_interval_effect_matches_selected_path(*inputs.interval_effect,
                                                         *inputs.selected_path)) {
    record.status = LocalArrayRangeProofStatus::MissingIntervalEffect;
    return record;
  }
  if (inputs.interval_effect->status != LocalArrayIntervalEffectStatus::Available) {
    record.status = local_array_range_proof_status_from_interval_effect(
        inputs.interval_effect->status);
    return record;
  }

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(*inputs.element_path,
                                       &saw_multiple_dynamic_indices);
  if (dynamic_index == nullptr || saw_multiple_dynamic_indices) {
    record.status = saw_multiple_dynamic_indices
                        ? LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved
                        : LocalArrayRangeProofStatus::MissingDynamicIndex;
    return record;
  }
  if (!inputs.selected_path->normalized_bound.has_value() ||
      inputs.selected_path->bound_contribution !=
          LocalArraySelectedProofEdgeBoundContribution::Upper) {
    record.status = LocalArrayRangeProofStatus::MissingUpperBound;
    return record;
  }
  const auto upper_predicate =
      local_array_range_upper_predicate_from_selected_path(
          inputs.selected_path->proof_predicate);
  if (!upper_predicate.has_value()) {
    record.status = LocalArrayRangeProofStatus::UnsupportedPredicate;
    return record;
  }
  std::int64_t upper_bound = *inputs.selected_path->normalized_bound;
  if (inputs.selected_path->bound_inclusive) {
    if (upper_bound == std::numeric_limits<std::int64_t>::max()) {
      record.status = LocalArrayRangeProofStatus::UnsupportedBoundary;
      return record;
    }
    ++upper_bound;
  }
  if (upper_bound < 0) {
    record.status = LocalArrayRangeProofStatus::UnsupportedBoundary;
    return record;
  }

  return evaluate_local_array_index_range_proof(
      LocalArrayIndexRangeProofInputs{
          .element_path = inputs.element_path,
          .consumer_function_name =
              inputs.selected_path->lir_producer_function_name,
          .proof_function_name = inputs.selected_path->proof_function_name,
          .consumer_block_label = inputs.selected_path->lir_producer_block_label,
          .proof_block_label = inputs.selected_path->proof_block_label,
          .consumer_instruction_index =
              inputs.selected_path->lir_producer_instruction_index,
          .proof_instruction_index = inputs.selected_path->proof_instruction_index,
          .proof_source_kind = LocalArrayRangeProofSourceKind::BranchCondition,
          .proof_lhs = inputs.selected_path->proof_lhs,
          .proof_rhs = inputs.selected_path->proof_rhs,
          .compare_type = inputs.selected_path->proof_compare_type,
          .lower_predicate = inputs.selected_path->proof_predicate ==
                                     LocalArraySelectedProofEdgePredicate::Ult ||
                                 inputs.selected_path->proof_predicate ==
                                     LocalArraySelectedProofEdgePredicate::Ule
                             ? LocalArrayRangeProofPredicate::Uge
                             : LocalArrayRangeProofPredicate::Sge,
          .upper_predicate = *upper_predicate,
          .lower_bound_available = true,
          .lower_bound = 0,
          .lower_bound_inclusive = true,
          .upper_bound_available = true,
          .upper_bound = static_cast<std::size_t>(upper_bound),
          .upper_bound_exclusive = true,
          .operand_roles_match_index =
              local_array_range_proof_same_value(
                  inputs.selected_path->proof_lhs, dynamic_index->value) ||
              local_array_range_proof_same_value(
                  inputs.selected_path->proof_rhs, dynamic_index->value),
          .path_validity_known = inputs.selected_path->path_validity_known,
          .proof_dominates_consumer =
              inputs.selected_path->proof_dominates_lir_producer ||
              inputs.selected_path->proof_guards_lir_producer,
          .path_covers_consumer =
              inputs.selected_path->selected_edge_covers_lir_producer,
          .no_clobber_known = true,
      });
}

[[nodiscard]] inline LocalArrayProofFactRecord
evaluate_local_array_proof_fact(const LocalArrayProofFactInputs& inputs) {
  LocalArrayProofFactRecord fact{
      .element_path = inputs.element_path,
      .range_proof = inputs.range_proof,
  };
  if (inputs.element_path == nullptr) {
    fact.status = LocalArrayRangeProofStatus::MissingLocalArrayPath;
    return fact;
  }
  fact.lir_producer_lookup_key = inputs.element_path->lir_producer_lookup_key;
  fact.lir_producer_function_name =
      inputs.element_path->lir_producer_function_name;
  fact.lir_producer_block_label = inputs.element_path->lir_producer_block_label;
  fact.lir_producer_instruction_index =
      inputs.element_path->lir_producer_instruction_index;

  if (inputs.range_proof == nullptr) {
    fact.status = LocalArrayRangeProofStatus::MissingRangeProofCertificate;
    return fact;
  }
  if (!local_array_range_proof_matches_element_path(*inputs.range_proof,
                                                    *inputs.element_path)) {
    fact.status = LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    return fact;
  }
  fact.dynamic_index = inputs.range_proof->dynamic_index;
  fact.proof_function_name = inputs.range_proof->proof_function_name;
  fact.proof_block_label = inputs.range_proof->proof_block_label;
  fact.proof_instruction_index = inputs.range_proof->proof_instruction_index;
  fact.proof_source_kind = inputs.range_proof->proof_source_kind;
  fact.proof_lhs = inputs.range_proof->proof_lhs;
  fact.proof_rhs = inputs.range_proof->proof_rhs;
  fact.compare_type = inputs.range_proof->compare_type;
  fact.lower_predicate = inputs.range_proof->lower_predicate;
  fact.upper_predicate = inputs.range_proof->upper_predicate;
  fact.normalized_lower_bound = inputs.range_proof->normalized_lower_bound;
  fact.normalized_upper_bound = inputs.range_proof->normalized_upper_bound;
  fact.lower_bound_inclusive = inputs.range_proof->lower_bound_inclusive;
  fact.upper_bound_exclusive = inputs.range_proof->upper_bound_exclusive;
  fact.path_validity_known = inputs.range_proof->path_validity_known;
  fact.proof_dominates_consumer =
      inputs.range_proof->proof_dominates_consumer;
  fact.path_covers_consumer = inputs.range_proof->path_covers_consumer;
  fact.no_clobber_known = inputs.range_proof->no_clobber_known;

  if (inputs.range_proof->status != LocalArrayRangeProofStatus::Available) {
    fact.status = inputs.range_proof->status;
    return fact;
  }

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(*inputs.element_path,
                                       &saw_multiple_dynamic_indices);
  if (dynamic_index == nullptr || saw_multiple_dynamic_indices) {
    fact.status = saw_multiple_dynamic_indices
                      ? LocalArrayRangeProofStatus::IndexPhiOrAliasUnresolved
                      : LocalArrayRangeProofStatus::MissingDynamicIndex;
    return fact;
  }
  if (!local_array_range_proof_same_value(inputs.range_proof->dynamic_index,
                                          dynamic_index->value)) {
    fact.status = LocalArrayRangeProofStatus::OperandRoleMismatch;
    return fact;
  }
  if (inputs.element_path->lir_producer_coordinate_status !=
          LocalArrayLirProducerCoordinateStatus::Available ||
      inputs.element_path->lir_producer_lookup_key.empty()) {
    fact.status = LocalArrayRangeProofStatus::MissingLirProducerCoordinate;
    return fact;
  }
  if (inputs.range_proof->consumer_function_name !=
          inputs.element_path->lir_producer_function_name ||
      inputs.range_proof->consumer_block_label !=
          inputs.element_path->lir_producer_block_label ||
      inputs.range_proof->consumer_instruction_index !=
          inputs.element_path->lir_producer_instruction_index) {
    fact.status = LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    return fact;
  }
  if (inputs.range_proof->normalized_lower_bound != 0 ||
      !inputs.range_proof->lower_bound_inclusive ||
      inputs.range_proof->normalized_upper_bound !=
          inputs.element_path->element_count ||
      !inputs.range_proof->upper_bound_exclusive) {
    fact.status = LocalArrayRangeProofStatus::BoundValueMismatch;
    return fact;
  }
  if (!inputs.range_proof->path_validity_known) {
    fact.status = LocalArrayRangeProofStatus::MissingPathValidity;
    return fact;
  }
  if (!inputs.range_proof->proof_dominates_consumer) {
    fact.status = LocalArrayRangeProofStatus::ProofNotDominatingConsumer;
    return fact;
  }
  if (!inputs.range_proof->path_covers_consumer) {
    fact.status = LocalArrayRangeProofStatus::PathNotCoveringConsumer;
    return fact;
  }
  if (!inputs.range_proof->no_clobber_known) {
    fact.status = LocalArrayRangeProofStatus::MissingNoClobber;
    return fact;
  }

  fact.status = LocalArrayRangeProofStatus::Available;
  return fact;
}

[[nodiscard]] inline bool local_array_proof_fact_matches_element_path(
    const LocalArrayProofFactRecord& proof_fact,
    const LocalArrayElementPathRecord& element_path) {
  if (proof_fact.element_path == &element_path) {
    return true;
  }
  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(element_path, &saw_multiple_dynamic_indices);
  return dynamic_index != nullptr &&
         !saw_multiple_dynamic_indices &&
         proof_fact.lir_producer_lookup_key ==
             element_path.lir_producer_lookup_key &&
         proof_fact.lir_producer_function_name ==
             element_path.lir_producer_function_name &&
         proof_fact.lir_producer_block_label ==
             element_path.lir_producer_block_label &&
         proof_fact.lir_producer_instruction_index ==
             element_path.lir_producer_instruction_index &&
         local_array_range_proof_same_value(proof_fact.dynamic_index,
                                            dynamic_index->value);
}

[[nodiscard]] inline LocalArrayIndexRangeCheckerInputRecord
evaluate_local_array_index_range_checker_input(
    const LocalArrayIndexRangeCheckerInputInputs& inputs) {
  LocalArrayIndexRangeCheckerInputRecord record{
      .element_path = inputs.element_path,
      .proof_fact = inputs.proof_fact,
  };
  if (inputs.element_path == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingLocalArrayPath;
    record.checker_record.status = record.status;
    return record;
  }
  if (inputs.proof_fact == nullptr) {
    record.status = LocalArrayRangeProofStatus::MissingProofFact;
    record.inputs.element_path = inputs.element_path;
    record.checker_record = LocalArrayIndexRangeProofRecord{
        .status = record.status,
        .element_path = inputs.element_path,
    };
    return record;
  }
  if (!local_array_proof_fact_matches_element_path(*inputs.proof_fact,
                                                   *inputs.element_path)) {
    record.status = LocalArrayRangeProofStatus::PreparedBirCoordinateConfusion;
    record.inputs.element_path = inputs.element_path;
    record.checker_record = LocalArrayIndexRangeProofRecord{
        .status = record.status,
        .element_path = inputs.element_path,
    };
    return record;
  }
  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(*inputs.element_path,
                                       &saw_multiple_dynamic_indices);
  const bool operand_roles_match_index =
      dynamic_index != nullptr &&
      !saw_multiple_dynamic_indices &&
      (local_array_range_proof_same_value(inputs.proof_fact->proof_lhs,
                                          dynamic_index->value) ||
       local_array_range_proof_same_value(inputs.proof_fact->proof_rhs,
                                          dynamic_index->value));

  record.inputs = LocalArrayIndexRangeProofInputs{
      .element_path = inputs.element_path,
      .consumer_function_name = inputs.proof_fact->lir_producer_function_name,
      .proof_function_name = inputs.proof_fact->proof_function_name,
      .consumer_block_label = inputs.proof_fact->lir_producer_block_label,
      .proof_block_label = inputs.proof_fact->proof_block_label,
      .consumer_instruction_index =
          inputs.proof_fact->lir_producer_instruction_index,
      .proof_instruction_index = inputs.proof_fact->proof_instruction_index,
      .proof_source_kind = inputs.proof_fact->proof_source_kind,
      .proof_lhs = inputs.proof_fact->proof_lhs,
      .proof_rhs = inputs.proof_fact->proof_rhs,
      .compare_type = inputs.proof_fact->compare_type,
      .lower_predicate = inputs.proof_fact->lower_predicate,
      .upper_predicate = inputs.proof_fact->upper_predicate,
      .lower_bound_available = true,
      .lower_bound = inputs.proof_fact->normalized_lower_bound,
      .lower_bound_inclusive = inputs.proof_fact->lower_bound_inclusive,
      .upper_bound_available = true,
      .upper_bound = inputs.proof_fact->normalized_upper_bound,
      .upper_bound_exclusive = inputs.proof_fact->upper_bound_exclusive,
      .operand_roles_match_index = operand_roles_match_index,
      .path_validity_known = inputs.proof_fact->path_validity_known,
      .proof_dominates_consumer =
          inputs.proof_fact->proof_dominates_consumer,
      .path_covers_consumer = inputs.proof_fact->path_covers_consumer,
      .no_clobber_known = inputs.proof_fact->no_clobber_known,
  };
  if (inputs.proof_fact->status != LocalArrayRangeProofStatus::Available) {
    record.status = inputs.proof_fact->status;
    record.checker_record = LocalArrayIndexRangeProofRecord{
        .status = record.status,
        .element_path = inputs.element_path,
    };
    return record;
  }
  record.checker_record = evaluate_local_array_index_range_proof(record.inputs);
  record.status = record.checker_record.status;
  return record;
}

[[nodiscard]] inline bool local_array_checker_input_matches_element_path(
    const LocalArrayIndexRangeCheckerInputRecord& checker_input,
    const LocalArrayElementPathRecord& element_path) {
  if (checker_input.element_path == &element_path) {
    return true;
  }
  if (checker_input.element_path == nullptr) {
    return false;
  }
  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(element_path, &saw_multiple_dynamic_indices);
  return dynamic_index != nullptr &&
         !saw_multiple_dynamic_indices &&
         checker_input.element_path->lir_producer_lookup_key ==
             element_path.lir_producer_lookup_key &&
         checker_input.element_path->lir_producer_function_name ==
             element_path.lir_producer_function_name &&
         checker_input.element_path->lir_producer_block_label ==
             element_path.lir_producer_block_label &&
         checker_input.element_path->lir_producer_instruction_index ==
             element_path.lir_producer_instruction_index &&
         local_array_range_proof_same_value(
             checker_input.checker_record.dynamic_index, dynamic_index->value);
}

[[nodiscard]] inline LocalArrayLocalAddressProvenanceRecord
evaluate_local_array_local_address_provenance(
    const LocalArrayLocalAddressProvenanceInputs& inputs) {
  LocalArrayLocalAddressProvenanceRecord record{
      .source_object = inputs.source_object,
      .derivation = inputs.derivation,
      .element_path = inputs.element_path,
      .checker_input = inputs.checker_input,
  };

  if (inputs.element_path == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingElementPath;
    return record;
  }

  const auto& path = *inputs.element_path;
  record.source_object_name = path.source_object_name;
  record.derived_pointer_name = path.derivation_result_name;
  record.element_result_name = path.result_name;
  record.lir_producer_lookup_key = path.lir_producer_lookup_key;
  record.lir_producer_function_name = path.lir_producer_function_name;
  record.lir_producer_block_label = path.lir_producer_block_label;
  record.lir_producer_instruction_index = path.lir_producer_instruction_index;
  record.element_type = path.element_type;
  record.element_size_bytes = path.element_size_bytes;
  record.byte_offset = path.byte_offset;
  record.element_count = path.element_count;
  record.scalar_in_bounds = path.scalar_in_bounds;

  if (path.status != LocalArrayCarrierStatus::Available &&
      path.status != LocalArrayCarrierStatus::MissingIndexRangeProof) {
    record.status = path.status;
    return record;
  }
  if (path.lir_producer_coordinate_status !=
      LocalArrayLirProducerCoordinateStatus::Available) {
    record.status = LocalArrayCarrierStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (path.lir_producer_operation_role !=
      LocalArrayLirProducerOperationRole::AddressDerivation) {
    record.status = LocalArrayCarrierStatus::UnknownProvenance;
    return record;
  }
  if (inputs.source_object == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingSourceObject;
    return record;
  }
  record.source_total_size_bytes = inputs.source_object->total_size_bytes;
  if (inputs.source_object->status != LocalArrayCarrierStatus::Available) {
    record.status = inputs.source_object->status;
    return record;
  }
  if (!local_array_source_object_matches_element_path(*inputs.source_object,
                                                      path)) {
    record.status = LocalArrayCarrierStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.derivation == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingDerivation;
    return record;
  }
  record.derivation_kind = inputs.derivation->kind;
  if (inputs.derivation->status != LocalArrayCarrierStatus::Available) {
    record.status = inputs.derivation->status;
    return record;
  }
  if (!local_array_derivation_matches_element_path(*inputs.derivation, path)) {
    record.status = LocalArrayCarrierStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.derivation->kind == LocalArrayDerivationKind::Unknown) {
    record.status = LocalArrayCarrierStatus::MissingDerivation;
    return record;
  }

  bool saw_multiple_dynamic_indices = false;
  const auto* dynamic_index =
      single_dynamic_local_array_index(path, &saw_multiple_dynamic_indices);
  if (dynamic_index == nullptr || saw_multiple_dynamic_indices) {
    record.status = LocalArrayCarrierStatus::MissingIndexIdentity;
    return record;
  }
  record.dynamic_index = dynamic_index->value;

  if (inputs.checker_input == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingIndexRangeProof;
    record.checker_status = LocalArrayRangeProofStatus::MissingProofFact;
    return record;
  }
  record.checker_status = inputs.checker_input->status;
  if (!local_array_checker_input_matches_element_path(*inputs.checker_input,
                                                      path)) {
    record.status = LocalArrayCarrierStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (inputs.checker_input->status != LocalArrayRangeProofStatus::Available ||
      inputs.checker_input->checker_record.status !=
          LocalArrayRangeProofStatus::Available) {
    record.status = LocalArrayCarrierStatus::MissingIndexRangeProof;
    return record;
  }
  if (path.element_type == TypeKind::Void || path.element_size_bytes == 0) {
    record.status =
        LocalArrayCarrierStatus::F128ComplexVectorOrVolatileAtomicBoundary;
    return record;
  }
  if (!path.scalar_in_bounds) {
    record.status = LocalArrayCarrierStatus::ElementOutOfBounds;
    return record;
  }
  if (inputs.source_object->total_size_bytes != 0 &&
      path.byte_offset + path.element_size_bytes >
          inputs.source_object->total_size_bytes) {
    record.status = LocalArrayCarrierStatus::ElementOutOfBounds;
    return record;
  }

  record.status = LocalArrayCarrierStatus::Available;
  return record;
}

[[nodiscard]] inline LocalArraySemanticGepRecord
evaluate_local_array_semantic_gep(
    const LocalArraySemanticGepInputs& inputs) {
  LocalArraySemanticGepRecord record{
      .provenance = inputs.provenance,
  };
  if (inputs.provenance == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingElementPath;
    return record;
  }

  const auto& provenance = *inputs.provenance;
  record.checker_status = provenance.checker_status;
  record.source_object = provenance.source_object;
  record.derivation = provenance.derivation;
  record.element_path = provenance.element_path;
  record.checker_input = provenance.checker_input;
  record.source_object_name = provenance.source_object_name;
  record.derived_pointer_name = provenance.derived_pointer_name;
  record.element_result_name = provenance.element_result_name;
  record.lir_producer_lookup_key = provenance.lir_producer_lookup_key;
  record.lir_producer_function_name = provenance.lir_producer_function_name;
  record.lir_producer_block_label = provenance.lir_producer_block_label;
  record.lir_producer_instruction_index =
      provenance.lir_producer_instruction_index;
  record.derivation_kind = provenance.derivation_kind;
  record.dynamic_index = provenance.dynamic_index;
  record.element_type = provenance.element_type;
  record.element_size_bytes = provenance.element_size_bytes;
  record.byte_offset = provenance.byte_offset;
  record.element_count = provenance.element_count;
  record.source_total_size_bytes = provenance.source_total_size_bytes;
  record.scalar_in_bounds = provenance.scalar_in_bounds;
  if (provenance.element_path != nullptr) {
    record.lir_producer_operation_role =
        provenance.element_path->lir_producer_operation_role;
    record.lir_producer_coordinate_status =
        provenance.element_path->lir_producer_coordinate_status;
  }

  if (provenance.status != LocalArrayCarrierStatus::Available) {
    record.status = provenance.status;
    return record;
  }
  if (provenance.source_object == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingSourceObject;
    return record;
  }
  if (provenance.source_object->status != LocalArrayCarrierStatus::Available) {
    record.status = provenance.source_object->status;
    return record;
  }
  if (provenance.derivation == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingDerivation;
    return record;
  }
  if (provenance.derivation->status != LocalArrayCarrierStatus::Available) {
    record.status = provenance.derivation->status;
    return record;
  }
  if (provenance.derivation_kind == LocalArrayDerivationKind::Unknown ||
      provenance.derivation->kind == LocalArrayDerivationKind::Unknown) {
    record.status = LocalArrayCarrierStatus::MissingDerivation;
    return record;
  }
  if (provenance.element_path == nullptr) {
    record.status = LocalArrayCarrierStatus::MissingElementPath;
    return record;
  }
  if (provenance.element_path->lir_producer_coordinate_status !=
      LocalArrayLirProducerCoordinateStatus::Available) {
    record.status = LocalArrayCarrierStatus::PreparedBirCoordinateConfusion;
    return record;
  }
  if (provenance.element_path->lir_producer_operation_role !=
      LocalArrayLirProducerOperationRole::AddressDerivation) {
    record.status = LocalArrayCarrierStatus::UnknownProvenance;
    return record;
  }
  if (provenance.checker_input == nullptr ||
      provenance.checker_status != LocalArrayRangeProofStatus::Available ||
      provenance.checker_input->status != LocalArrayRangeProofStatus::Available ||
      provenance.checker_input->checker_record.status !=
          LocalArrayRangeProofStatus::Available) {
    record.status = LocalArrayCarrierStatus::MissingIndexRangeProof;
    return record;
  }
  if (provenance.element_type == TypeKind::Void ||
      provenance.element_size_bytes == 0) {
    record.status =
        LocalArrayCarrierStatus::F128ComplexVectorOrVolatileAtomicBoundary;
    return record;
  }
  if (!provenance.scalar_in_bounds) {
    record.status = LocalArrayCarrierStatus::ElementOutOfBounds;
    return record;
  }
  if (provenance.source_total_size_bytes != 0 &&
      provenance.byte_offset + provenance.element_size_bytes >
          provenance.source_total_size_bytes) {
    record.status = LocalArrayCarrierStatus::ElementOutOfBounds;
    return record;
  }

  record.status = LocalArrayCarrierStatus::Available;
  return record;
}
enum class GlobalStaticGepAuthorityStatus : unsigned char {
  Available,
  MissingGlobalSourceObject,
  MissingGlobalIdentity,
  MissingGlobalLayout,
  MissingDerivedPointerIdentity,
  MissingLayoutPath,
  MissingElementByteRange,
  MissingDynamicIndexIdentity,
  MissingDynamicRangeAuthority,
  ElementOutOfBounds,
  PointerOrFormalProvenanceBoundary,
  StringOrGlobalPointerProvenanceBoundary,
  RuntimeOrStringIntrinsicBoundary,
  AggregateOrMemberBoundary,
  RawShapeOnly,
  TargetOnlyOrFinalHomeOnly,
  PreparedBirCoordinateConfusion,
};

[[nodiscard]] constexpr std::string_view global_static_gep_authority_status_name(
    GlobalStaticGepAuthorityStatus status) {
  switch (status) {
    case GlobalStaticGepAuthorityStatus::Available:
      return "available";
    case GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject:
      return "missing_global_source_object";
    case GlobalStaticGepAuthorityStatus::MissingGlobalIdentity:
      return "missing_global_identity";
    case GlobalStaticGepAuthorityStatus::MissingGlobalLayout:
      return "missing_global_layout";
    case GlobalStaticGepAuthorityStatus::MissingDerivedPointerIdentity:
      return "missing_derived_pointer_identity";
    case GlobalStaticGepAuthorityStatus::MissingLayoutPath:
      return "missing_layout_path";
    case GlobalStaticGepAuthorityStatus::MissingElementByteRange:
      return "missing_element_byte_range";
    case GlobalStaticGepAuthorityStatus::MissingDynamicIndexIdentity:
      return "missing_dynamic_index_identity";
    case GlobalStaticGepAuthorityStatus::MissingDynamicRangeAuthority:
      return "missing_dynamic_range_authority";
    case GlobalStaticGepAuthorityStatus::ElementOutOfBounds:
      return "element_out_of_bounds";
    case GlobalStaticGepAuthorityStatus::PointerOrFormalProvenanceBoundary:
      return "pointer_or_formal_provenance_boundary";
    case GlobalStaticGepAuthorityStatus::StringOrGlobalPointerProvenanceBoundary:
      return "string_or_global_pointer_provenance_boundary";
    case GlobalStaticGepAuthorityStatus::RuntimeOrStringIntrinsicBoundary:
      return "runtime_or_string_intrinsic_boundary";
    case GlobalStaticGepAuthorityStatus::AggregateOrMemberBoundary:
      return "aggregate_or_member_boundary";
    case GlobalStaticGepAuthorityStatus::RawShapeOnly:
      return "raw_shape_only";
    case GlobalStaticGepAuthorityStatus::TargetOnlyOrFinalHomeOnly:
      return "target_only_or_final_home_only";
    case GlobalStaticGepAuthorityStatus::PreparedBirCoordinateConfusion:
      return "prepared_bir_coordinate_confusion";
  }
  return "unknown";
}

enum class GlobalStaticGepDerivationKind : unsigned char {
  Unknown,
  DirectGlobal,
  RelativeGlobalPointer,
  DynamicGlobalPointerArray,
  DynamicGlobalAggregateArray,
  DynamicGlobalScalarArray,
};

[[nodiscard]] constexpr std::string_view global_static_gep_derivation_kind_name(
    GlobalStaticGepDerivationKind kind) {
  switch (kind) {
    case GlobalStaticGepDerivationKind::Unknown:
      return "unknown";
    case GlobalStaticGepDerivationKind::DirectGlobal:
      return "direct_global";
    case GlobalStaticGepDerivationKind::RelativeGlobalPointer:
      return "relative_global_pointer";
    case GlobalStaticGepDerivationKind::DynamicGlobalPointerArray:
      return "dynamic_global_pointer_array";
    case GlobalStaticGepDerivationKind::DynamicGlobalAggregateArray:
      return "dynamic_global_aggregate_array";
    case GlobalStaticGepDerivationKind::DynamicGlobalScalarArray:
      return "dynamic_global_scalar_array";
  }
  return "unknown";
}

enum class GlobalStaticGepCoordinateStatus : unsigned char {
  Available,
  MissingLirProducerCoordinate,
  MissingLirProducerBlock,
  MissingLirInstructionIndex,
  MissingLirProducerLookupKey,
};

[[nodiscard]] constexpr std::string_view global_static_gep_coordinate_status_name(
    GlobalStaticGepCoordinateStatus status) {
  switch (status) {
    case GlobalStaticGepCoordinateStatus::Available:
      return "available";
    case GlobalStaticGepCoordinateStatus::MissingLirProducerCoordinate:
      return "missing_lir_producer_coordinate";
    case GlobalStaticGepCoordinateStatus::MissingLirProducerBlock:
      return "missing_lir_producer_block";
    case GlobalStaticGepCoordinateStatus::MissingLirInstructionIndex:
      return "missing_lir_instruction_index";
    case GlobalStaticGepCoordinateStatus::MissingLirProducerLookupKey:
      return "missing_lir_producer_lookup_key";
  }
  return "unknown";
}

struct GlobalStaticGepAuthorityRecord {
  GlobalStaticGepAuthorityStatus status =
      GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject;
  GlobalStaticGepDerivationKind derivation_kind =
      GlobalStaticGepDerivationKind::Unknown;
  std::string global_name;
  LinkNameId global_link_name_id = kInvalidLinkName;
  std::string result_name;
  std::string base_pointer_name;
  std::string source_type_text;
  std::string layout_path_type_text;
  std::size_t source_size_bytes = 0;
  std::size_t byte_offset = 0;
  TypeKind element_type = TypeKind::Void;
  std::string element_type_text;
  std::size_t element_size_bytes = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bool has_constant_range = false;
  bool has_dynamic_range = false;
  Value dynamic_index;
  MemoryLayoutAuthorityKind layout_authority = MemoryLayoutAuthorityKind::Unknown;
  MemoryRangeVerdict range_verdict = MemoryRangeVerdict::UnknownCompatible;
  GlobalStaticGepCoordinateStatus coordinate_status =
      GlobalStaticGepCoordinateStatus::Available;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  std::string lir_producer_lookup_key;
};

struct GlobalStaticSemanticGepInputs {
  const GlobalStaticGepAuthorityRecord* authority = nullptr;
};

struct GlobalStaticSemanticGepRecord {
  GlobalStaticGepAuthorityStatus status =
      GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject;
  const GlobalStaticGepAuthorityRecord* authority = nullptr;
  GlobalStaticGepDerivationKind derivation_kind =
      GlobalStaticGepDerivationKind::Unknown;
  std::string global_name;
  LinkNameId global_link_name_id = kInvalidLinkName;
  std::string result_name;
  std::string base_pointer_name;
  std::string source_type_text;
  std::string layout_path_type_text;
  std::size_t source_size_bytes = 0;
  std::size_t byte_offset = 0;
  TypeKind element_type = TypeKind::Void;
  std::string element_type_text;
  std::size_t element_size_bytes = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bool has_constant_range = false;
  bool has_dynamic_range = false;
  Value dynamic_index;
  MemoryLayoutAuthorityKind layout_authority = MemoryLayoutAuthorityKind::Unknown;
  MemoryRangeVerdict range_verdict = MemoryRangeVerdict::UnknownCompatible;
  GlobalStaticGepCoordinateStatus coordinate_status =
      GlobalStaticGepCoordinateStatus::MissingLirProducerCoordinate;
  std::string lir_producer_function_name;
  std::string lir_producer_block_label;
  std::optional<std::size_t> lir_producer_instruction_index;
  std::string lir_producer_lookup_key;
};

[[nodiscard]] inline GlobalStaticSemanticGepRecord
evaluate_global_static_semantic_gep(
    const GlobalStaticSemanticGepInputs& inputs) {
  GlobalStaticSemanticGepRecord record{
      .authority = inputs.authority,
  };
  if (inputs.authority == nullptr) {
    record.status = GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject;
    return record;
  }

  const auto& authority = *inputs.authority;
  record.status = authority.status;
  record.derivation_kind = authority.derivation_kind;
  record.global_name = authority.global_name;
  record.global_link_name_id = authority.global_link_name_id;
  record.result_name = authority.result_name;
  record.base_pointer_name = authority.base_pointer_name;
  record.source_type_text = authority.source_type_text;
  record.layout_path_type_text = authority.layout_path_type_text;
  record.source_size_bytes = authority.source_size_bytes;
  record.byte_offset = authority.byte_offset;
  record.element_type = authority.element_type;
  record.element_type_text = authority.element_type_text;
  record.element_size_bytes = authority.element_size_bytes;
  record.element_count = authority.element_count;
  record.element_stride_bytes = authority.element_stride_bytes;
  record.has_constant_range = authority.has_constant_range;
  record.has_dynamic_range = authority.has_dynamic_range;
  record.dynamic_index = authority.dynamic_index;
  record.layout_authority = authority.layout_authority;
  record.range_verdict = authority.range_verdict;
  record.coordinate_status = authority.coordinate_status;
  record.lir_producer_function_name = authority.lir_producer_function_name;
  record.lir_producer_block_label = authority.lir_producer_block_label;
  record.lir_producer_instruction_index =
      authority.lir_producer_instruction_index;
  record.lir_producer_lookup_key = authority.lir_producer_lookup_key;

  if (authority.status != GlobalStaticGepAuthorityStatus::Available) {
    return record;
  }
  if (authority.global_name.empty()) {
    record.status =
        GlobalStaticGepAuthorityStatus::MissingGlobalSourceObject;
    return record;
  }
  if (authority.global_link_name_id == kInvalidLinkName) {
    record.status = GlobalStaticGepAuthorityStatus::MissingGlobalIdentity;
    return record;
  }
  if (authority.source_size_bytes == 0) {
    record.status = GlobalStaticGepAuthorityStatus::MissingGlobalLayout;
    return record;
  }
  if (authority.result_name.empty()) {
    record.status =
        GlobalStaticGepAuthorityStatus::MissingDerivedPointerIdentity;
    return record;
  }
  if (authority.layout_path_type_text.empty()) {
    record.status = GlobalStaticGepAuthorityStatus::MissingLayoutPath;
    return record;
  }
  if (authority.element_size_bytes == 0) {
    record.status = GlobalStaticGepAuthorityStatus::MissingElementByteRange;
    return record;
  }
  if (!authority.has_constant_range && !authority.has_dynamic_range) {
    record.status =
        GlobalStaticGepAuthorityStatus::MissingDynamicRangeAuthority;
    return record;
  }
  if (authority.range_verdict != MemoryRangeVerdict::ProvenInBounds) {
    record.status = authority.range_verdict == MemoryRangeVerdict::ProvenOutOfBounds
                        ? GlobalStaticGepAuthorityStatus::ElementOutOfBounds
                        : GlobalStaticGepAuthorityStatus::
                              MissingDynamicRangeAuthority;
    return record;
  }
  if (authority.element_type == TypeKind::Ptr) {
    record.status =
        GlobalStaticGepAuthorityStatus::StringOrGlobalPointerProvenanceBoundary;
    return record;
  }
  if (authority.coordinate_status != GlobalStaticGepCoordinateStatus::Available) {
    record.status =
        GlobalStaticGepAuthorityStatus::PreparedBirCoordinateConfusion;
    return record;
  }

  record.status = GlobalStaticGepAuthorityStatus::Available;
  return record;
}

}  // namespace c4c::backend::bir
