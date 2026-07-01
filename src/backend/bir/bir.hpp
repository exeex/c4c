#pragma once

#include <cstdint>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {

struct Module;
struct Function;
struct Block;

struct NameTables {
  NameTables() { reattach(); }

  NameTables(const NameTables& other)
      : texts(other.texts),
        link_names(other.link_names),
        block_labels(other.block_labels),
        slot_names(other.slot_names) {
    reattach();
  }

  NameTables(NameTables&& other) noexcept
      : texts(std::move(other.texts)),
        link_names(std::move(other.link_names)),
        block_labels(std::move(other.block_labels)),
        slot_names(std::move(other.slot_names)) {
    reattach();
  }

  NameTables& operator=(const NameTables& other) {
    if (this == &other) {
      return *this;
    }
    texts = other.texts;
    link_names = other.link_names;
    block_labels = other.block_labels;
    slot_names = other.slot_names;
    reattach();
    return *this;
  }

  NameTables& operator=(NameTables&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    texts = std::move(other.texts);
    link_names = std::move(other.link_names);
    block_labels = std::move(other.block_labels);
    slot_names = std::move(other.slot_names);
    reattach();
    return *this;
  }

  void import_link_names(const TextTable& source_texts,
                         const LinkNameTable& source_link_names) {
    texts = source_texts;
    link_names = source_link_names;
    reattach();
  }

  TextTable texts;
  LinkNameTable link_names{&texts};
  BlockLabelTable block_labels{&texts};
  SlotNameTable slot_names{&texts};

 private:
  void reattach() {
    link_names.attach_text_table(&texts);
    block_labels.attach_text_table(&texts);
    slot_names.attach_text_table(&texts);
  }
};

enum class TypeKind : unsigned char {
  Void,
  I1,
  I8,
  I16,
  I32,
  I64,
  I128,
  Ptr,
  F32,
  F64,
  F128,
  Vrm1,
  Vrm2,
  Vrm4,
  Vrm8,
};

[[nodiscard]] constexpr bool is_vrm_register_type(TypeKind type) {
  switch (type) {
    case TypeKind::Vrm1:
    case TypeKind::Vrm2:
    case TypeKind::Vrm4:
    case TypeKind::Vrm8:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] constexpr std::size_t vrm_register_group_width(TypeKind type) {
  switch (type) {
    case TypeKind::Vrm1:
      return 1;
    case TypeKind::Vrm2:
      return 2;
    case TypeKind::Vrm4:
      return 4;
    case TypeKind::Vrm8:
      return 8;
    default:
      return 0;
  }
}

enum class AddressSpace : unsigned char {
  Default,
  Fs,
  Gs,
  Tls,
};

enum class AtomicOperationKind : unsigned char {
  None,
  Load,
  Store,
  Fence,
  Rmw,
  CompareExchange,
};

[[nodiscard]] constexpr std::string_view atomic_operation_kind_name(
    AtomicOperationKind kind) {
  switch (kind) {
    case AtomicOperationKind::None:
      return "none";
    case AtomicOperationKind::Load:
      return "load";
    case AtomicOperationKind::Store:
      return "store";
    case AtomicOperationKind::Fence:
      return "fence";
    case AtomicOperationKind::Rmw:
      return "rmw";
    case AtomicOperationKind::CompareExchange:
      return "compare_exchange";
  }
  return "unknown";
}

enum class AtomicOrdering : unsigned char {
  None,
  Relaxed,
  Acquire,
  Release,
  AcqRel,
  SeqCst,
};

[[nodiscard]] constexpr std::string_view atomic_ordering_name(AtomicOrdering ordering) {
  switch (ordering) {
    case AtomicOrdering::None:
      return "none";
    case AtomicOrdering::Relaxed:
      return "relaxed";
    case AtomicOrdering::Acquire:
      return "acquire";
    case AtomicOrdering::Release:
      return "release";
    case AtomicOrdering::AcqRel:
      return "acq_rel";
    case AtomicOrdering::SeqCst:
      return "seq_cst";
  }
  return "unknown";
}

enum class AtomicRmwOpcode : unsigned char {
  None,
  Exchange,
  Add,
  Sub,
  And,
  Or,
  Xor,
};

[[nodiscard]] constexpr std::string_view atomic_rmw_opcode_name(AtomicRmwOpcode opcode) {
  switch (opcode) {
    case AtomicRmwOpcode::None:
      return "none";
    case AtomicRmwOpcode::Exchange:
      return "exchange";
    case AtomicRmwOpcode::Add:
      return "add";
    case AtomicRmwOpcode::Sub:
      return "sub";
    case AtomicRmwOpcode::And:
      return "and";
    case AtomicRmwOpcode::Or:
      return "or";
    case AtomicRmwOpcode::Xor:
      return "xor";
  }
  return "unknown";
}

enum class AtomicResultMode : unsigned char {
  None,
  LoadedValue,
  OldValue,
  BooleanSuccess,
};

[[nodiscard]] constexpr std::string_view atomic_result_mode_name(AtomicResultMode mode) {
  switch (mode) {
    case AtomicResultMode::None:
      return "none";
    case AtomicResultMode::LoadedValue:
      return "loaded_value";
    case AtomicResultMode::OldValue:
      return "old_value";
    case AtomicResultMode::BooleanSuccess:
      return "boolean_success";
  }
  return "unknown";
}

enum class IntrinsicFamilyKind : unsigned char {
  None,
  ScalarFpUnary,
  Crc,
  VectorMemory,
  VectorOperation,
  Barrier,
  CacheMaintenance,
  PauseHint,
};

[[nodiscard]] constexpr std::string_view intrinsic_family_kind_name(
    IntrinsicFamilyKind kind) {
  switch (kind) {
    case IntrinsicFamilyKind::None:
      return "none";
    case IntrinsicFamilyKind::ScalarFpUnary:
      return "scalar_fp_unary";
    case IntrinsicFamilyKind::Crc:
      return "crc";
    case IntrinsicFamilyKind::VectorMemory:
      return "vector_memory";
    case IntrinsicFamilyKind::VectorOperation:
      return "vector_operation";
    case IntrinsicFamilyKind::Barrier:
      return "barrier";
    case IntrinsicFamilyKind::CacheMaintenance:
      return "cache_maintenance";
    case IntrinsicFamilyKind::PauseHint:
      return "pause_hint";
  }
  return "unknown";
}

enum class IntrinsicOperationKind : unsigned char {
  None,
  FAbs,
  Crc32W,
  VectorLoad,
  VectorAdd,
  BarrierDmb,
  CacheDcCvau,
  HintYield,
};

[[nodiscard]] constexpr std::string_view intrinsic_operation_kind_name(
    IntrinsicOperationKind kind) {
  switch (kind) {
    case IntrinsicOperationKind::None:
      return "none";
    case IntrinsicOperationKind::FAbs:
      return "fabs";
    case IntrinsicOperationKind::Crc32W:
      return "crc32w";
    case IntrinsicOperationKind::VectorLoad:
      return "vector_load";
    case IntrinsicOperationKind::VectorAdd:
      return "vector_add";
    case IntrinsicOperationKind::BarrierDmb:
      return "barrier_dmb";
    case IntrinsicOperationKind::CacheDcCvau:
      return "cache_dc_cvau";
    case IntrinsicOperationKind::HintYield:
      return "hint_yield";
  }
  return "unknown";
}

enum class IntrinsicFeatureKind : unsigned char {
  None,
  AArch64Crc,
  AArch64Neon,
};

[[nodiscard]] constexpr std::string_view intrinsic_feature_kind_name(
    IntrinsicFeatureKind kind) {
  switch (kind) {
    case IntrinsicFeatureKind::None:
      return "none";
    case IntrinsicFeatureKind::AArch64Crc:
      return "aarch64_crc";
    case IntrinsicFeatureKind::AArch64Neon:
      return "aarch64_neon";
  }
  return "unknown";
}

enum class IntrinsicOperandRole : unsigned char {
  None,
  Accumulator,
  Data,
  Pointer,
  VectorLhs,
  VectorRhs,
  BarrierDomain,
  CacheAddress,
  HintImmediate,
};

[[nodiscard]] constexpr std::string_view intrinsic_operand_role_name(
    IntrinsicOperandRole role) {
  switch (role) {
    case IntrinsicOperandRole::None:
      return "none";
    case IntrinsicOperandRole::Accumulator:
      return "accumulator";
    case IntrinsicOperandRole::Data:
      return "data";
    case IntrinsicOperandRole::Pointer:
      return "pointer";
    case IntrinsicOperandRole::VectorLhs:
      return "vector_lhs";
    case IntrinsicOperandRole::VectorRhs:
      return "vector_rhs";
    case IntrinsicOperandRole::BarrierDomain:
      return "barrier_domain";
    case IntrinsicOperandRole::CacheAddress:
      return "cache_address";
    case IntrinsicOperandRole::HintImmediate:
      return "hint_immediate";
  }
  return "unknown";
}

enum class IntrinsicBarrierDomainKind : unsigned char {
  None,
  Sy,
};

[[nodiscard]] constexpr std::string_view intrinsic_barrier_domain_kind_name(
    IntrinsicBarrierDomainKind domain) {
  switch (domain) {
    case IntrinsicBarrierDomainKind::None:
      return "none";
    case IntrinsicBarrierDomainKind::Sy:
      return "sy";
  }
  return "unknown";
}

enum class IntrinsicSignedness : unsigned char {
  None,
  Unsigned,
  Signed,
};

[[nodiscard]] constexpr std::string_view intrinsic_signedness_name(
    IntrinsicSignedness signedness) {
  switch (signedness) {
    case IntrinsicSignedness::None:
      return "none";
    case IntrinsicSignedness::Unsigned:
      return "unsigned";
    case IntrinsicSignedness::Signed:
      return "signed";
  }
  return "unknown";
}

enum class IntrinsicMemoryAccessKind : unsigned char {
  None,
  Read,
  Write,
  ReadWrite,
};

[[nodiscard]] constexpr std::string_view intrinsic_memory_access_kind_name(
    IntrinsicMemoryAccessKind access) {
  switch (access) {
    case IntrinsicMemoryAccessKind::None:
      return "none";
    case IntrinsicMemoryAccessKind::Read:
      return "read";
    case IntrinsicMemoryAccessKind::Write:
      return "write";
    case IntrinsicMemoryAccessKind::ReadWrite:
      return "read_write";
  }
  return "unknown";
}

enum class CallingConv : unsigned char {
  C,
  SysV,
  Win64,
  Fast,
  Cold,
};

enum class AbiValueClass : unsigned char {
  None,
  Integer,
  Sse,
  X87,
  Memory,
};

struct Value {
  enum class Kind : unsigned char {
    Immediate,
    Named,
  };

  struct F128Payload {
    std::uint64_t low_bits = 0;
    std::uint64_t high_bits = 0;
  };

  Kind kind = Kind::Immediate;
  TypeKind type = TypeKind::Void;
  std::int64_t immediate = 0;
  std::uint64_t immediate_bits = 0;
  std::optional<F128Payload> f128_payload;
  // Display/final spelling for SSA-like values; BIR does not use this as
  // cross-object semantic identity.
  std::string name;
  // Semantic identity for named pointer values that denote link-visible
  // globals or functions. Route-local pointers, locals, slots, nulls, and
  // unresolved compatibility values keep this invalid.
  LinkNameId pointer_symbol_link_name_id = kInvalidLinkName;

  static Value immediate_i1(bool value);
  static Value immediate_i8(std::int8_t value);
  static Value immediate_i16(std::int16_t value);
  static Value immediate_i32(std::int32_t value);
  static Value immediate_i64(std::int64_t value);
  static Value immediate_f32_bits(std::uint32_t bits);
  static Value immediate_f64_bits(std::uint64_t bits);
  static Value immediate_f128_bits(std::uint64_t low_bits, std::uint64_t high_bits);
  static Value named(TypeKind type, std::string name);
  static Value named_symbol_pointer(std::string name, LinkNameId link_name_id);
};

inline bool operator==(const Value::F128Payload& lhs, const Value::F128Payload& rhs) {
  return lhs.low_bits == rhs.low_bits && lhs.high_bits == rhs.high_bits;
}

inline bool operator==(const Value& lhs, const Value& rhs) {
  return lhs.kind == rhs.kind && lhs.type == rhs.type && lhs.immediate == rhs.immediate &&
         lhs.immediate_bits == rhs.immediate_bits && lhs.f128_payload == rhs.f128_payload &&
         lhs.name == rhs.name &&
         lhs.pointer_symbol_link_name_id == rhs.pointer_symbol_link_name_id;
}

inline bool operator!=(const Value& lhs, const Value& rhs) {
  return !(lhs == rhs);
}

enum class Route1ProducerKind : unsigned char {
  Unknown,
  Immediate,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  SelectMaterialization,
};

[[nodiscard]] constexpr std::string_view route1_producer_kind_name(
    Route1ProducerKind kind) {
  switch (kind) {
    case Route1ProducerKind::Unknown:
      return "unknown";
    case Route1ProducerKind::Immediate:
      return "immediate";
    case Route1ProducerKind::LoadLocal:
      return "load_local";
    case Route1ProducerKind::LoadGlobal:
      return "load_global";
    case Route1ProducerKind::Cast:
      return "cast";
    case Route1ProducerKind::Binary:
      return "binary";
    case Route1ProducerKind::SelectMaterialization:
      return "select_materialization";
  }
  return "unknown";
}

[[nodiscard]] constexpr bool route1_producer_kind_has_materialization(
    Route1ProducerKind kind) {
  switch (kind) {
    case Route1ProducerKind::LoadLocal:
    case Route1ProducerKind::LoadGlobal:
    case Route1ProducerKind::Cast:
    case Route1ProducerKind::Binary:
    case Route1ProducerKind::SelectMaterialization:
      return true;
    case Route1ProducerKind::Unknown:
    case Route1ProducerKind::Immediate:
      return false;
  }
  return false;
}

struct Route1SourceValueIdentity {
  const Value* value = nullptr;
  Value::Kind value_kind = Value::Kind::Immediate;
  TypeKind type = TypeKind::Void;
  std::string_view name;
  ValueNameId name_id = kInvalidValueName;
  std::optional<std::int64_t> integer_constant;
  LinkNameId pointer_symbol_link_name_id = kInvalidLinkName;

  [[nodiscard]] explicit operator bool() const {
    return value != nullptr ||
           !name.empty() ||
           name_id != kInvalidValueName ||
           integer_constant.has_value();
  }
};

struct Route1ImmediateIntegerConstant {
  bool available = false;
  std::int64_t value = 0;
  TypeKind type = TypeKind::Void;
  unsigned depth = 0;

  [[nodiscard]] explicit operator bool() const { return available; }
};

[[nodiscard]] Route1SourceValueIdentity route1_source_value_identity(
    const Value& value,
    ValueNameId name_id = kInvalidValueName);

[[nodiscard]] Route1ImmediateIntegerConstant route1_immediate_integer_constant(
    const Value& value,
    unsigned depth = 0);

struct PhiIncoming {
  // Compatibility label spelling for dumps and raw-only BIR; label_id is the
  // semantic block reference when present.
  std::string label;
  Value value;
  BlockLabelId label_id = kInvalidBlockLabel;
};

struct PhiObservation {
  Value result;
  std::vector<PhiIncoming> incomings;
};

struct CallArgAbiInfo {
  TypeKind type = TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  AbiValueClass primary_class = AbiValueClass::None;
  AbiValueClass secondary_class = AbiValueClass::None;
  bool passed_in_register = false;
  bool passed_on_stack = false;
  bool byval_copy = false;
  bool sret_pointer = false;
  std::size_t aarch64_hfa_lane_count = 0;
  std::size_t aarch64_hfa_lane_index = 0;
};

struct CallResultAbiInfo {
  TypeKind type = TypeKind::Void;
  AbiValueClass primary_class = AbiValueClass::None;
  AbiValueClass secondary_class = AbiValueClass::None;
  bool returned_in_memory = false;
  std::size_t register_count = 1;
};

struct Param {
  TypeKind type = TypeKind::Void;
  // Display/final spelling for the lowered function parameter.
  std::string name;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<CallArgAbiInfo> abi;
  bool is_varargs = false;
  bool is_sret = false;
  bool is_byval = false;
};

enum class LocalSlotStorageKind : unsigned char {
  None,
  LoweringScratch,
};

struct LocalSlot {
  // Display/compatibility spelling for a route-local slot. slot_id is the
  // semantic route-local slot reference when present.
  std::string name;
  SlotNameId slot_id = kInvalidSlotName;
  TypeKind type = TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  LocalSlotStorageKind storage_kind = LocalSlotStorageKind::None;
  bool is_address_taken = false;
  bool is_byval_copy = false;
  std::optional<PhiObservation> phi_observation;
};

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
  MissingDynamicIndex,
  MissingProofSource,
  UnsupportedProofSource,
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
    case LocalArrayRangeProofStatus::MissingDynamicIndex:
      return "missing_dynamic_index";
    case LocalArrayRangeProofStatus::MissingProofSource:
      return "missing_proof_source";
    case LocalArrayRangeProofStatus::UnsupportedProofSource:
      return "unsupported_proof_source";
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
  std::int64_t normalized_lower_bound = 0;
  std::size_t normalized_upper_bound = 0;
  bool lower_bound_inclusive = true;
  bool upper_bound_exclusive = true;
  bool path_validity_known = false;
  bool proof_dominates_consumer = false;
  bool path_covers_consumer = false;
  bool no_clobber_known = false;
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

enum class GlobalAddressMaterializationPolicy {
  Unspecified,
  Direct,
  GotRequired,
};

[[nodiscard]] constexpr std::string_view global_address_materialization_policy_name(
    GlobalAddressMaterializationPolicy policy) {
  switch (policy) {
    case GlobalAddressMaterializationPolicy::Unspecified:
      return "unspecified";
    case GlobalAddressMaterializationPolicy::Direct:
      return "direct";
    case GlobalAddressMaterializationPolicy::GotRequired:
      return "got_required";
  }
  return "unknown";
}

struct Global {
  std::string name;
  LinkNameId link_name_id = kInvalidLinkName;
  TypeKind type = TypeKind::Void;
  bool is_extern = false;
  bool is_thread_local = false;
  bool is_constant = false;
  bool has_scalar_layout_authority = false;
  bool has_integer_array_layout_authority = false;
  std::size_t integer_array_element_size_bytes = 0;
  std::size_t integer_array_element_count = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<Value> initializer;
  std::optional<std::string> initializer_symbol_name;
  // Retained compatibility/display spelling for pointer initializers parsed
  // from legacy LIR text. Known global/function targets also carry
  // initializer_symbol_name_id after module lowering resolves the spelling
  // through structured symbol tables; unresolved compatibility-only targets
  // keep the id invalid.
  LinkNameId initializer_symbol_name_id = kInvalidLinkName;
  std::vector<Value> initializer_elements;
  GlobalAddressMaterializationPolicy address_materialization_policy =
      GlobalAddressMaterializationPolicy::Unspecified;
};

struct StringConstant {
  // Compatibility/display spelling for raw-only BIR and output. Metadata-rich
  // lowering must use name_id as the text-pool identity and may only consume
  // name when no structured text identity exists.
  std::string name;
  TextId name_id = kInvalidText;
  std::string bytes;
  std::size_t align_bytes = 1;
};

struct StructuredTypeFieldSpelling {
  // Dump/final type spelling retained for structured type rendering.
  std::string type_name;
};

struct StructuredTypeDeclSpelling {
  // Dump/final type spelling; structured layout authority lives in the
  // structured type tables built during lowering.
  std::string name;
  std::vector<StructuredTypeFieldSpelling> fields;
  bool is_packed = false;
  bool is_opaque = false;
};

struct StructuredTypeSpellingContext {
  std::vector<StructuredTypeDeclSpelling> declarations;

  [[nodiscard]] const StructuredTypeDeclSpelling* find_struct_decl(
      std::string_view name) const;
};

enum class MemoryProvenanceBaseIdentityKind : unsigned char {
  Unknown,
  UnknownRuntimeBase,
  LocalSlot,
  GlobalSymbol,
  PointerValue,
  FormalParameter,
  ByvalParameter,
  SretParameter,
  StringConstant,
};

enum class MemoryObjectExtentCompleteness : unsigned char {
  Unknown,
  Complete,
  Partial,
};

enum class MemoryLayoutAuthorityKind : unsigned char {
  Unknown,
  StructuredLayout,
  ScalarLayout,
  ByteStorageAggregate,
  RenderedTypeFallback,
  OpaqueCompatibility,
};

enum class MemoryRangeVerdict : unsigned char {
  UnknownCompatible,
  ProvenInBounds,
  ProvenOutOfBounds,
};

enum class MemoryDynamicArrayRangeVerdict : unsigned char {
  Unknown,
  BoundedByElementCount,
  Unbounded,
};

struct MemoryProvenanceBaseIdentity {
  MemoryProvenanceBaseIdentityKind kind = MemoryProvenanceBaseIdentityKind::Unknown;
  std::string spelling;
  Value value;
  LinkNameId link_name_id = kInvalidLinkName;
  SlotNameId slot_name_id = kInvalidSlotName;
};

struct MemoryObjectExtent {
  MemoryObjectExtentCompleteness completeness = MemoryObjectExtentCompleteness::Unknown;
  std::size_t size_bytes = 0;
  bool size_known = false;
};

struct MemoryByteRange {
  bool available = false;
  std::int64_t begin = 0;
  std::size_t size_bytes = 0;
  std::int64_t end = 0;
  bool end_available = false;
  bool overflowed = false;
};

[[nodiscard]] inline MemoryByteRange make_memory_byte_range(std::int64_t byte_offset,
                                                           std::size_t size_bytes) {
  MemoryByteRange range{
      .available = true,
      .begin = byte_offset,
      .size_bytes = size_bytes,
  };
  if (size_bytes >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    range.overflowed = true;
    return range;
  }

  const auto delta = static_cast<std::int64_t>(size_bytes);
  if (byte_offset > std::numeric_limits<std::int64_t>::max() - delta) {
    range.overflowed = true;
    return range;
  }

  range.end = byte_offset + delta;
  range.end_available = true;
  return range;
}

struct MemoryDynamicArrayFacts {
  bool available = false;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  std::size_t base_byte_offset = 0;
  Value index;
  MemoryDynamicArrayRangeVerdict verdict = MemoryDynamicArrayRangeVerdict::Unknown;
};

struct MemoryAccessProvenance {
  MemoryProvenanceBaseIdentity base_identity;
  MemoryObjectExtent object_extent;
  MemoryByteRange requested_range;
  MemoryLayoutAuthorityKind layout_authority = MemoryLayoutAuthorityKind::Unknown;
  MemoryDynamicArrayFacts dynamic_array;
  MemoryRangeVerdict range_verdict = MemoryRangeVerdict::UnknownCompatible;
};

inline void prove_memory_dynamic_array_range(MemoryAccessProvenance& provenance) {
  provenance.dynamic_array.verdict = MemoryDynamicArrayRangeVerdict::Unknown;
  const auto& requested = provenance.requested_range;
  const auto& dynamic_array = provenance.dynamic_array;
  if (!dynamic_array.available || dynamic_array.element_count == 0 ||
      dynamic_array.element_stride_bytes == 0 || !requested.available ||
      requested.overflowed || !requested.end_available || requested.begin < 0 ||
      requested.end < requested.begin) {
    return;
  }

  if (dynamic_array.element_count >
      std::numeric_limits<std::size_t>::max() / dynamic_array.element_stride_bytes) {
    return;
  }
  const auto array_size_bytes =
      dynamic_array.element_count * dynamic_array.element_stride_bytes;
  if (dynamic_array.base_byte_offset >
      std::numeric_limits<std::size_t>::max() - array_size_bytes) {
    return;
  }

  const auto begin = static_cast<std::size_t>(requested.begin);
  const auto end = static_cast<std::size_t>(requested.end);
  const auto array_begin = dynamic_array.base_byte_offset;
  const auto array_end = dynamic_array.base_byte_offset + array_size_bytes;
  provenance.dynamic_array.verdict =
      begin >= array_begin && end <= array_end
          ? MemoryDynamicArrayRangeVerdict::BoundedByElementCount
          : MemoryDynamicArrayRangeVerdict::Unbounded;
}

inline void prove_memory_access_requested_range(MemoryAccessProvenance& provenance) {
  provenance.range_verdict = MemoryRangeVerdict::UnknownCompatible;
  prove_memory_dynamic_array_range(provenance);
  if (!provenance.requested_range.available ||
      !provenance.object_extent.size_known ||
      provenance.object_extent.completeness != MemoryObjectExtentCompleteness::Complete) {
    return;
  }
  if (provenance.requested_range.overflowed ||
      !provenance.requested_range.end_available ||
      provenance.requested_range.begin < 0 ||
      provenance.requested_range.end < provenance.requested_range.begin) {
    provenance.range_verdict = MemoryRangeVerdict::ProvenOutOfBounds;
    return;
  }

  const auto begin = static_cast<std::size_t>(provenance.requested_range.begin);
  const auto end = static_cast<std::size_t>(provenance.requested_range.end);
  provenance.range_verdict =
      begin <= provenance.object_extent.size_bytes &&
              end <= provenance.object_extent.size_bytes
          ? MemoryRangeVerdict::ProvenInBounds
          : MemoryRangeVerdict::ProvenOutOfBounds;
}

struct MemoryAddress {
  enum class BaseKind : unsigned char {
    None,
    LocalSlot,
    GlobalSymbol,
    PointerValue,
    Label,
    StringConstant,
  };

  BaseKind base_kind = BaseKind::None;
  // Display/compatibility spelling for address dumps. Global, label, and local
  // slot bases carry ids when semantic identity is known.
  std::string base_name;
  Value base_value;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  AddressSpace address_space = AddressSpace::Default;
  bool is_volatile = false;
  LinkNameId base_link_name_id = kInvalidLinkName;
  BlockLabelId base_label_id = kInvalidBlockLabel;
  SlotNameId base_slot_id = kInvalidSlotName;
  MemoryAccessProvenance provenance;
};

enum class BinaryOpcode : unsigned char {
  Add,
  Sub,
  Mul,
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  SDiv,
  UDiv,
  SRem,
  URem,
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};

struct BinaryInst {
  BinaryOpcode opcode = BinaryOpcode::Add;
  Value result;
  TypeKind operand_type = TypeKind::Void;
  Value lhs;
  Value rhs;
};

struct SelectInst {
  BinaryOpcode predicate = BinaryOpcode::Eq;
  Value result;
  TypeKind compare_type = TypeKind::Void;
  Value lhs;
  Value rhs;
  Value true_value;
  Value false_value;
};

enum class CastOpcode : unsigned char {
  SExt,
  ZExt,
  Trunc,
  FPTrunc,
  FPExt,
  FPToSI,
  FPToUI,
  SIToFP,
  UIToFP,
  PtrToInt,
  IntToPtr,
  Bitcast,
};

struct CastInst {
  CastOpcode opcode = CastOpcode::SExt;
  Value result;
  Value operand;
};

struct PhiInst {
  Value result;
  std::vector<PhiIncoming> incomings;
};

enum class InlineAsmOperandKind : unsigned char {
  Unsupported,
  RegisterInput,
  RegisterOutput,
  TiedInput,
  IntegerImmediateInput,
  MemoryInput,
  AddressInput,
  Clobber,
};

enum class InlineAsmRegisterClass : unsigned char {
  None,
  General,
  Vector,
};

[[nodiscard]] constexpr std::string_view inline_asm_register_class_name(
    InlineAsmRegisterClass register_class) {
  switch (register_class) {
    case InlineAsmRegisterClass::None:
      return "none";
    case InlineAsmRegisterClass::General:
      return "general";
    case InlineAsmRegisterClass::Vector:
      return "vector";
  }
  return "unknown";
}

[[nodiscard]] constexpr std::string_view inline_asm_operand_kind_name(
    InlineAsmOperandKind kind) {
  switch (kind) {
    case InlineAsmOperandKind::Unsupported:
      return "unsupported";
    case InlineAsmOperandKind::RegisterInput:
      return "register_input";
    case InlineAsmOperandKind::RegisterOutput:
      return "register_output";
    case InlineAsmOperandKind::TiedInput:
      return "tied_input";
    case InlineAsmOperandKind::IntegerImmediateInput:
      return "integer_immediate_input";
    case InlineAsmOperandKind::MemoryInput:
      return "memory_input";
    case InlineAsmOperandKind::AddressInput:
      return "address_input";
    case InlineAsmOperandKind::Clobber:
      return "clobber";
  }
  return "unknown";
}

struct InlineAsmOperandMetadata {
  InlineAsmOperandKind kind = InlineAsmOperandKind::Unsupported;
  std::size_t constraint_index = 0;
  std::string constraint;
  std::optional<std::size_t> arg_index;
  std::optional<std::size_t> output_index;
  std::optional<std::size_t> tied_output_index;
  InlineAsmRegisterClass register_class = InlineAsmRegisterClass::None;
  std::size_t register_group_width = 1;
  std::optional<std::string> name;
  std::optional<MemoryAddress> memory_address;
  std::optional<MemoryAddress> address;
};

struct InlineAsmInsnRMetadata {
  std::uint32_t opcode = 0;
  std::uint32_t funct3 = 0;
  std::uint32_t funct7 = 0;
  std::array<std::size_t, 3> operand_indices{};
};

struct InlineAsmMetadata {
  // Inline assembly payload text is final spelling passed through for dumps and
  // target emission diagnostics; it is not BIR lookup authority.
  std::string asm_text;
  std::string constraints;
  std::string args_text;
  bool side_effects = false;
  std::vector<InlineAsmOperandMetadata> operands;
  std::vector<std::string> clobbers;
  std::vector<std::string> unsupported_facts;
  bool has_named_operand_references = false;
  bool has_template_modifiers = false;
  std::optional<InlineAsmInsnRMetadata> insn_r;
};

struct IntrinsicOperation {
  IntrinsicFamilyKind family = IntrinsicFamilyKind::None;
  IntrinsicOperationKind operation = IntrinsicOperationKind::None;
  IntrinsicFeatureKind required_feature = IntrinsicFeatureKind::None;
  TypeKind operand_type = TypeKind::Void;
  TypeKind result_type = TypeKind::Void;
  std::vector<IntrinsicOperandRole> operand_roles;
  TypeKind vector_element_type = TypeKind::Void;
  std::size_t vector_element_width_bytes = 0;
  std::size_t vector_lane_count = 0;
  std::size_t vector_total_width_bytes = 0;
  IntrinsicSignedness signedness = IntrinsicSignedness::None;
  std::optional<MemoryAddress> memory_operand;
  IntrinsicMemoryAccessKind memory_access = IntrinsicMemoryAccessKind::None;
  IntrinsicBarrierDomainKind barrier_domain = IntrinsicBarrierDomainKind::None;
  bool has_immediate_operand = false;
  bool requires_immediate_operand = false;
  std::optional<std::int64_t> immediate_value;
  bool has_side_effects = false;
};

enum class CallArgumentSourceEncodingKind : unsigned char {
  None,
  Register,
  FrameSlot,
  Immediate,
  ComputedAddress,
  SymbolAddress,
};

[[nodiscard]] constexpr std::string_view call_argument_source_encoding_kind_name(
    CallArgumentSourceEncodingKind kind) {
  switch (kind) {
    case CallArgumentSourceEncodingKind::None:
      return "none";
    case CallArgumentSourceEncodingKind::Register:
      return "register";
    case CallArgumentSourceEncodingKind::FrameSlot:
      return "frame_slot";
    case CallArgumentSourceEncodingKind::Immediate:
      return "immediate";
    case CallArgumentSourceEncodingKind::ComputedAddress:
      return "computed_address";
    case CallArgumentSourceEncodingKind::SymbolAddress:
      return "symbol_address";
  }
  return "unknown";
}

enum class CallArgumentSourceSelectionKind : unsigned char {
  None,
  PriorPreservation,
  LocalFrameAddressMaterialization,
  FrameSlotAddress,
  FrameSlotValue,
  ByvalRegisterLane,
};

[[nodiscard]] constexpr std::string_view call_argument_source_selection_kind_name(
    CallArgumentSourceSelectionKind kind) {
  switch (kind) {
    case CallArgumentSourceSelectionKind::None:
      return "none";
    case CallArgumentSourceSelectionKind::PriorPreservation:
      return "prior_preservation";
    case CallArgumentSourceSelectionKind::LocalFrameAddressMaterialization:
      return "local_frame_address_materialization";
    case CallArgumentSourceSelectionKind::FrameSlotAddress:
      return "frame_slot_address";
    case CallArgumentSourceSelectionKind::FrameSlotValue:
      return "frame_slot_value";
    case CallArgumentSourceSelectionKind::ByvalRegisterLane:
      return "byval_register_lane";
  }
  return "unknown";
}

struct CallArgumentSourceSelection {
  CallArgumentSourceSelectionKind kind = CallArgumentSourceSelectionKind::None;
  std::optional<std::size_t> source_value_id;
  std::optional<std::string> source_value_name;
  std::optional<std::size_t> source_base_value_id;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<SlotNameId> source_slot_id;
  // Prepared stack layout facts, not BIR source-identity authority. Production
  // BIR lowering must leave these empty; they exist only so legacy fixture
  // comparisons can quarantine prepared-only policy before consumers switch.
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_size_bytes;
  std::optional<std::size_t> source_align_bytes;
  // Prepared address-materialization placement facts, not BIR source-identity
  // authority. Production BIR lowering may identify the source value/slot, but
  // not the instruction index, frame slot, or byte offset used by ABI layout.
  std::optional<BlockLabelId> address_materialization_block_label;
  std::optional<std::size_t> address_materialization_inst_index;
  std::optional<SlotNameId> address_materialization_frame_slot_id;
  std::optional<std::int64_t> address_materialization_byte_offset;
};

[[nodiscard]] constexpr bool call_argument_source_selection_available(
    const CallArgumentSourceSelection& selection) {
  return selection.kind != CallArgumentSourceSelectionKind::None;
}

struct CallArgumentDirectGlobalSelectChainDependency {
  bool available = false;
  std::string source_value_name;
  bool contains_direct_global_load = false;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
};

[[nodiscard]] constexpr bool call_argument_direct_global_select_chain_dependency_available(
    const CallArgumentDirectGlobalSelectChainDependency& dependency) {
  return dependency.available &&
         dependency.contains_direct_global_load &&
         dependency.root_instruction_index.has_value();
}

enum class CallArgumentSourceProducerKind : unsigned char {
  Unknown,
  LoadLocal,
  Binary,
};

[[nodiscard]] constexpr std::string_view call_argument_source_producer_kind_name(
    CallArgumentSourceProducerKind kind) {
  switch (kind) {
    case CallArgumentSourceProducerKind::Unknown:
      return "unknown";
    case CallArgumentSourceProducerKind::LoadLocal:
      return "load_local";
    case CallArgumentSourceProducerKind::Binary:
      return "binary";
  }
  return "unknown";
}

[[nodiscard]] bool call_argument_binary_source_producer_opcode_is_materializable(
    BinaryOpcode opcode);

struct CallArgumentSourceRelationship {
  std::size_t arg_index = 0;
  CallArgumentSourceEncodingKind source_encoding =
      CallArgumentSourceEncodingKind::None;
  std::optional<std::size_t> source_value_id;
  std::optional<std::string> source_value_name;
  std::optional<std::size_t> source_base_value_id;
  std::optional<std::string> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<CallArgumentSourceSelection> source_selection;
  std::optional<CallArgumentDirectGlobalSelectChainDependency>
      direct_global_select_chain_dependency;
};

struct CallArgumentPublicationSourceRouting {
  bool available = false;
  std::size_t arg_index = 0;
  CallArgumentSourceEncodingKind source_encoding =
      CallArgumentSourceEncodingKind::None;
  std::optional<std::size_t> source_value_id;
  std::optional<std::string> source_value_name;
  std::optional<std::size_t> source_base_value_id;
  std::optional<std::string> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  const CallArgumentSourceSelection* source_selection = nullptr;
  const CallArgumentDirectGlobalSelectChainDependency*
      direct_global_select_chain_dependency = nullptr;
};

struct CallInst {
  std::optional<Value> result;
  std::vector<Value> result_lanes;
  std::string callee;
  // Direct user/extern calls carry LinkNameId when LIR provides one.
  // Runtime/intrinsic placeholder calls synthesized by BIR lowering keep this
  // invalid and use callee only as an explicit compatibility/display token.
  LinkNameId callee_link_name_id = kInvalidLinkName;
  std::optional<Value> callee_value;
  std::vector<Value> args;
  std::vector<TypeKind> arg_types;
  std::vector<CallArgumentSourceRelationship> arg_sources;
  std::vector<CallArgAbiInfo> arg_abi;
  // Final type spelling for aggregate call results; return_type remains the
  // scalar ABI class and structured_return_type_name is display/dump text.
  std::optional<std::string> structured_return_type_name;
  std::string return_type_name;
  TypeKind return_type = TypeKind::Void;
  std::optional<CallResultAbiInfo> result_abi;
  // Runtime va_arg placeholders publish the semantic payload ABI here so
  // prealloc can plan AAPCS64 helper access without recomputing ABI from the
  // display callee or lowered result type.
  std::optional<CallArgAbiInfo> va_arg_payload_abi;
  std::size_t va_arg_hfa_lane_count = 0;
  std::size_t va_arg_hfa_lane_size_bytes = 0;
  CallingConv calling_convention = CallingConv::C;
  bool is_indirect = false;
  bool is_variadic = false;
  bool is_noreturn = false;
  std::optional<InlineAsmMetadata> inline_asm;
  std::optional<IntrinsicOperation> intrinsic;
  // Route-local sret storage spelling used to relate generated local slots.
  std::optional<std::string> sret_storage_name;
  SlotNameId sret_storage_name_id = kInvalidSlotName;
};

struct LoadLocalInst {
  Value result;
  // Compatibility slot spelling for dumps and raw-only BIR; slot_id is the
  // semantic local-slot reference when present.
  std::string slot_name;
  SlotNameId slot_id = kInvalidSlotName;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct LoadGlobalInst {
  Value result;
  std::string global_name;
  // Known global references carry LinkNameId, including dynamic scalar/array
  // materialization once the global_types table has a declaration. Invalid ids
  // are reserved for unresolved compatibility-only paths that remain
  // display-name validated.
  LinkNameId global_name_id = kInvalidLinkName;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct StoreGlobalInst {
  std::string global_name;
  // Known global references carry LinkNameId, including dynamic scalar/array
  // materialization once the global_types table has a declaration. Invalid ids
  // are reserved for unresolved compatibility-only paths that remain
  // display-name validated.
  LinkNameId global_name_id = kInvalidLinkName;
  Value value;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct StoreLocalInst {
  // Compatibility slot spelling for dumps and raw-only BIR; slot_id is the
  // semantic local-slot reference when present.
  std::string slot_name;
  SlotNameId slot_id = kInvalidSlotName;
  Value value;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

using Inst = std::variant<BinaryInst,
                          SelectInst,
                          CastInst,
                          PhiInst,
                          CallInst,
                          LoadLocalInst,
                          LoadGlobalInst,
                          StoreGlobalInst,
                          StoreLocalInst>;

struct Route1ProducerInstructionIdentity {
  const Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  Route1ProducerKind kind = Route1ProducerKind::Unknown;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;

  [[nodiscard]] explicit operator bool() const { return instruction != nullptr; }
};

struct Route1MaterializationAvailability {
  bool available = false;
  bool scalar_materialization_available = false;
  Route1ProducerKind producer_kind = Route1ProducerKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route1ProducerRecord {
  bool available = false;
  Route1ProducerKind kind = Route1ProducerKind::Unknown;
  Route1SourceValueIdentity source_value;
  Route1ProducerInstructionIdentity producer_instruction;
  Route1ImmediateIntegerConstant integer_constant;
  Route1MaterializationAvailability materialization;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route1ProducerIndex {
  const Block* block = nullptr;
  std::vector<Route1ProducerRecord> records;

  [[nodiscard]] explicit operator bool() const { return block != nullptr; }
};

struct Route1SameBlockProducerQuery {
  const Route1ProducerIndex* index = nullptr;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return index != nullptr && *index;
  }
};

struct Route1SameBlockScalarProducer {
  const Route1ProducerRecord* record = nullptr;
  const Inst* instruction = nullptr;
  const Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  Route1MaterializationAvailability materialization;

  [[nodiscard]] explicit operator bool() const {
    return record != nullptr && instruction != nullptr && produced_value != nullptr;
  }
};

[[nodiscard]] Route1ProducerKind route1_producer_kind(const Inst& inst);

[[nodiscard]] const Value* route1_produced_value(const Inst& inst);

[[nodiscard]] Route1ProducerInstructionIdentity route1_producer_instruction_identity(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route1ProducerRecord route1_producer_record(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route1ProducerIndex route1_build_producer_index(
    const Block& block);

[[nodiscard]] std::optional<Route1SameBlockScalarProducer>
route1_find_same_block_scalar_producer(
    Route1SameBlockProducerQuery query,
    const Value& value);

[[nodiscard]] Route1MaterializationAvailability
route1_find_materialization_availability(
    Route1SameBlockProducerQuery query,
    const Value& value);

[[nodiscard]] std::optional<Route1ImmediateIntegerConstant>
route1_evaluate_same_block_integer_constant(
    Route1SameBlockProducerQuery query,
    const Value& value,
    unsigned depth);

[[nodiscard]] std::optional<Route1ImmediateIntegerConstant>
route1_evaluate_same_block_integer_constant(
    Route1SameBlockProducerQuery query,
    const Value& value);

enum class Route2SelectChainProducerKind : unsigned char {
  Unknown,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  Select,
};

[[nodiscard]] constexpr std::string_view route2_select_chain_producer_kind_name(
    Route2SelectChainProducerKind kind) {
  switch (kind) {
    case Route2SelectChainProducerKind::Unknown:
      return "unknown";
    case Route2SelectChainProducerKind::LoadLocal:
      return "load_local";
    case Route2SelectChainProducerKind::LoadGlobal:
      return "load_global";
    case Route2SelectChainProducerKind::Cast:
      return "cast";
    case Route2SelectChainProducerKind::Binary:
      return "binary";
    case Route2SelectChainProducerKind::Select:
      return "select";
  }
  return "unknown";
}

struct Route2SelectChainProducerRecord {
  bool available = false;
  Route2SelectChainProducerKind kind = Route2SelectChainProducerKind::Unknown;
  const Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  const Value* produced_value = nullptr;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::string_view global_name;
  LinkNameId global_name_id = kInvalidLinkName;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route2SelectChainDirectGlobalDependencyRecord {
  bool available = false;
  bool contains_direct_global_load = false;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
  const LoadGlobalInst* load_global = nullptr;
  std::size_t direct_load_instruction_index = 0;
  std::string_view global_name;
  LinkNameId global_name_id = kInvalidLinkName;

  [[nodiscard]] explicit operator bool() const {
    return available && contains_direct_global_load && load_global != nullptr;
  }
};

struct Route2SelectChainValueRecord {
  bool available = false;
  Route1SourceValueIdentity root_value;
  std::string_view root_value_name;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
  bool scalar_materialization_available = false;
  Route2SelectChainProducerRecord root_producer;
  Route2SelectChainDirectGlobalDependencyRecord direct_global_dependency;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route2SelectChainValueIndex {
  const Block* block = nullptr;
  std::vector<Route2SelectChainValueRecord> records;

  [[nodiscard]] explicit operator bool() const { return block != nullptr; }
};

struct Route2SelectChainValueQuery {
  const Route2SelectChainValueIndex* index = nullptr;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return index != nullptr && *index;
  }
};

[[nodiscard]] Route2SelectChainProducerKind route2_select_chain_producer_kind(
    const Inst& inst);

[[nodiscard]] Route2SelectChainProducerRecord
route2_select_chain_producer_record(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route2SelectChainValueRecord route2_select_chain_value_record(
    Route1SameBlockProducerQuery query,
    const Value& value);

[[nodiscard]] Route2SelectChainValueIndex
route2_build_select_chain_value_index(const Block& block);

[[nodiscard]] const Route2SelectChainValueRecord*
route2_find_select_chain_value_record(
    Route2SelectChainValueQuery query,
    const Value& value);

enum class Route3MemoryAccessNodeKind : unsigned char {
  Unknown,
  LoadLocal,
  LoadGlobal,
  StoreLocal,
  StoreGlobal,
};

[[nodiscard]] constexpr std::string_view route3_memory_access_node_kind_name(
    Route3MemoryAccessNodeKind kind) {
  switch (kind) {
    case Route3MemoryAccessNodeKind::Unknown:
      return "unknown";
    case Route3MemoryAccessNodeKind::LoadLocal:
      return "load_local";
    case Route3MemoryAccessNodeKind::LoadGlobal:
      return "load_global";
    case Route3MemoryAccessNodeKind::StoreLocal:
      return "store_local";
    case Route3MemoryAccessNodeKind::StoreGlobal:
      return "store_global";
  }
  return "unknown";
}

enum class Route3MemoryAccessBaseKind : unsigned char {
  None,
  LocalSlot,
  GlobalSymbol,
  PointerValue,
  StringConstant,
};

[[nodiscard]] constexpr std::string_view route3_memory_access_base_kind_name(
    Route3MemoryAccessBaseKind kind) {
  switch (kind) {
    case Route3MemoryAccessBaseKind::None:
      return "none";
    case Route3MemoryAccessBaseKind::LocalSlot:
      return "local_slot";
    case Route3MemoryAccessBaseKind::GlobalSymbol:
      return "global_symbol";
    case Route3MemoryAccessBaseKind::PointerValue:
      return "pointer_value";
    case Route3MemoryAccessBaseKind::StringConstant:
      return "string_constant";
  }
  return "none";
}

enum class Route3MemoryAccessValueRole : unsigned char {
  None,
  Result,
  Stored,
};

struct Route3MemoryAccessRecord {
  bool available = false;
  const Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  Route3MemoryAccessNodeKind node_kind = Route3MemoryAccessNodeKind::Unknown;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  Route1SourceValueIdentity result_value;
  Route1SourceValueIdentity stored_value;
  AddressSpace address_space = AddressSpace::Default;
  bool is_volatile = false;
  Route3MemoryAccessBaseKind base_kind = Route3MemoryAccessBaseKind::None;
  std::string_view local_slot_name;
  SlotNameId local_slot_id = kInvalidSlotName;
  std::string_view global_name;
  LinkNameId global_name_id = kInvalidLinkName;
  Route1SourceValueIdentity pointer_value;
  std::string_view string_constant_name;
  LinkNameId string_constant_name_id = kInvalidLinkName;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  MemoryAccessProvenance provenance;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route3MemoryAccessValueRecord {
  bool available = false;
  Route3MemoryAccessValueRole role = Route3MemoryAccessValueRole::None;
  Route1SourceValueIdentity value;
  std::size_t access_instruction_index = 0;
  Route3MemoryAccessNodeKind node_kind = Route3MemoryAccessNodeKind::Unknown;
  Route3MemoryAccessRecord access;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route3MemoryAccessIndex {
  const Block* block = nullptr;
  std::vector<Route3MemoryAccessRecord> records;

  [[nodiscard]] explicit operator bool() const { return block != nullptr; }
};

struct Route3MemoryAccessQuery {
  const Route3MemoryAccessIndex* index = nullptr;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return index != nullptr && *index;
  }
};

struct Route3SameBlockGlobalLoadAccessRecord {
  bool available = false;
  bool access_available = false;
  Route1SourceValueIdentity root_value;
  std::optional<std::size_t> load_instruction_index;
  Route3MemoryAccessRecord load_access;

  [[nodiscard]] explicit operator bool() const {
    return available && access_available && load_access;
  }
};

struct Route3SameBlockLoadLocalSourceRecord {
  bool available = false;
  bool source_available = false;
  Route1SourceValueIdentity root_value;
  std::optional<std::size_t> load_instruction_index;
  Route3MemoryAccessRecord load_access;
  std::optional<std::size_t> invalidating_store_instruction_index;
  Route3MemoryAccessRecord invalidating_store_access;

  [[nodiscard]] explicit operator bool() const {
    return available && source_available && load_access;
  }
};

struct Route3SameBlockLoadLocalStoredValueSourceRecord {
  bool available = false;
  bool source_available = false;
  Route1SourceValueIdentity root_value;
  Route1SourceValueIdentity stored_value;
  std::optional<std::size_t> load_instruction_index;
  Route3MemoryAccessRecord load_access;
  std::optional<std::size_t> store_instruction_index;
  Route3MemoryAccessRecord store_access;

  [[nodiscard]] explicit operator bool() const {
    return available && source_available && load_access && store_access &&
           stored_value;
  }
};

[[nodiscard]] Route3MemoryAccessNodeKind route3_memory_access_node_kind(
    const Inst& inst);

[[nodiscard]] Route3MemoryAccessBaseKind route3_memory_access_base_kind(
    const MemoryAddress& address);

[[nodiscard]] Route3MemoryAccessRecord route3_memory_access_record(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route3MemoryAccessValueRecord
route3_memory_access_result_value_record(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route3MemoryAccessValueRecord
route3_memory_access_stored_value_record(
    const Block& block,
    std::size_t instruction_index);

[[nodiscard]] Route3MemoryAccessIndex route3_build_memory_access_index(
    const Block& block);

[[nodiscard]] const Route3MemoryAccessRecord*
route3_find_memory_access_record(
    const Route3MemoryAccessIndex& index,
    std::size_t instruction_index,
    Route3MemoryAccessNodeKind node_kind);

[[nodiscard]] Route3SameBlockGlobalLoadAccessRecord
route3_same_block_global_load_access_record(
    Route1SameBlockProducerQuery query,
    const Value& value);

[[nodiscard]] Route3SameBlockLoadLocalSourceRecord
route3_same_block_load_local_source_record(
    Route1SameBlockProducerQuery query,
    const Value& value);

[[nodiscard]] Route3SameBlockGlobalLoadAccessRecord
route3_find_same_block_global_load_access(
    Route3MemoryAccessQuery query,
    const Value& value);

[[nodiscard]] Route3SameBlockLoadLocalSourceRecord
route3_find_same_block_load_local_source(
    Route3MemoryAccessQuery query,
    const Value& value);

[[nodiscard]] Route3SameBlockLoadLocalStoredValueSourceRecord
route3_find_same_block_load_local_stored_value_source(
    Route3MemoryAccessQuery query,
    const Value& value);

enum class Route4PublicationAvailabilityStatus : unsigned char {
  Unavailable,
  Available,
  MissingBlock,
  MissingValue,
  MissingPublication,
  AlternateSource,
  NoMatch,
};

[[nodiscard]] constexpr std::string_view
route4_publication_availability_status_name(
    Route4PublicationAvailabilityStatus status) {
  switch (status) {
    case Route4PublicationAvailabilityStatus::Unavailable:
      return "unavailable";
    case Route4PublicationAvailabilityStatus::Available:
      return "available";
    case Route4PublicationAvailabilityStatus::MissingBlock:
      return "missing_block";
    case Route4PublicationAvailabilityStatus::MissingValue:
      return "missing_value";
    case Route4PublicationAvailabilityStatus::MissingPublication:
      return "missing_publication";
    case Route4PublicationAvailabilityStatus::AlternateSource:
      return "alternate_source";
    case Route4PublicationAvailabilityStatus::NoMatch:
      return "no_match";
  }
  return "unavailable";
}

enum class Route4PublicationScope : unsigned char {
  None,
  CurrentBlock,
  BlockEntry,
};

enum class Route4PublicationSourceKind : unsigned char {
  Unknown,
  Immediate,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  SelectMaterialization,
};

enum class Route4PublicationValueRole : unsigned char {
  None,
  Produced,
  Consumed,
  Source,
};

struct Route4CurrentBlockPublicationRecord {
  bool available = false;
  Route4PublicationAvailabilityStatus status =
      Route4PublicationAvailabilityStatus::Unavailable;
  const Block* block = nullptr;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  Route1SourceValueIdentity value;
  std::string_view value_name;
  ValueNameId value_name_id = kInvalidValueName;
  TypeKind value_type = TypeKind::Void;
  std::size_t before_instruction_index = 0;
  Route4PublicationSourceKind source_producer_kind =
      Route4PublicationSourceKind::Unknown;
  const Inst* source_producer_instruction = nullptr;
  std::size_t source_producer_instruction_index = 0;
  BlockLabelId source_producer_block_label_id = kInvalidBlockLabel;
  Route1SourceValueIdentity produced_value;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route4BlockEntryPublicationRecord {
  bool available = false;
  Route4PublicationAvailabilityStatus status =
      Route4PublicationAvailabilityStatus::Unavailable;
  const Block* successor_block = nullptr;
  std::string_view successor_label;
  BlockLabelId successor_label_id = kInvalidBlockLabel;
  const Inst* destination_instruction = nullptr;
  const PhiInst* phi = nullptr;
  std::size_t destination_instruction_index = 0;
  Route1SourceValueIdentity destination_value;
  std::string_view destination_value_name;
  ValueNameId destination_value_name_id = kInvalidValueName;
  TypeKind destination_value_type = TypeKind::Void;
  Route1SourceValueIdentity source_value;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route4PublicationValueRecord {
  bool available = false;
  Route4PublicationScope scope = Route4PublicationScope::None;
  Route4PublicationAvailabilityStatus status =
      Route4PublicationAvailabilityStatus::Unavailable;
  Route4PublicationValueRole value_role = Route4PublicationValueRole::None;
  Route1SourceValueIdentity value;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  Route4CurrentBlockPublicationRecord current_block;
  Route4BlockEntryPublicationRecord block_entry;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route4PublicationAvailabilityIndex {
  const Function* function = nullptr;
  std::vector<Route4CurrentBlockPublicationRecord> current_block_records;
  std::vector<Route4BlockEntryPublicationRecord> block_entry_records;
  std::vector<Route4PublicationValueRecord> value_records;

  [[nodiscard]] explicit operator bool() const { return function != nullptr; }
};

[[nodiscard]] Route4PublicationSourceKind
route4_publication_source_kind(Route1ProducerKind kind);

[[nodiscard]] Route4CurrentBlockPublicationRecord
route4_current_block_publication_record(
    Route1SameBlockProducerQuery query,
    const Value& value,
    ValueNameId value_name_id = kInvalidValueName);

[[nodiscard]] Route4BlockEntryPublicationRecord
route4_block_entry_publication_record(
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id = kInvalidValueName);

[[nodiscard]] Route4PublicationValueRecord
route4_current_block_publication_value_record(
    Route1SameBlockProducerQuery query,
    const Value& value,
    ValueNameId value_name_id = kInvalidValueName);

[[nodiscard]] Route4PublicationValueRecord
route4_block_entry_publication_value_record(
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id = kInvalidValueName);

[[nodiscard]] Route4PublicationAvailabilityIndex
route4_build_publication_availability_index(const Function& function);

[[nodiscard]] Route4CurrentBlockPublicationRecord
route4_find_current_block_publication(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);

enum class Route5PublicationStatus : unsigned char {
  Unavailable,
  Available,
  NoSource,
  MemorySource,
  MissingPredecessor,
  MissingSuccessor,
  MissingDestination,
  MissingPublication,
  MissingSourceValue,
  MissingSourceProducer,
  MissingSourceMemoryAccess,
  IncompleteSourceMemoryAccess,
  NoMatch,
};

[[nodiscard]] constexpr std::string_view route5_publication_status_name(
    Route5PublicationStatus status) {
  switch (status) {
    case Route5PublicationStatus::Unavailable:
      return "unavailable";
    case Route5PublicationStatus::Available:
      return "available";
    case Route5PublicationStatus::NoSource:
      return "no_source";
    case Route5PublicationStatus::MemorySource:
      return "memory_source";
    case Route5PublicationStatus::MissingPredecessor:
      return "missing_predecessor";
    case Route5PublicationStatus::MissingSuccessor:
      return "missing_successor";
    case Route5PublicationStatus::MissingDestination:
      return "missing_destination";
    case Route5PublicationStatus::MissingPublication:
      return "missing_publication";
    case Route5PublicationStatus::MissingSourceValue:
      return "missing_source_value";
    case Route5PublicationStatus::MissingSourceProducer:
      return "missing_source_producer";
    case Route5PublicationStatus::MissingSourceMemoryAccess:
      return "missing_source_memory_access";
    case Route5PublicationStatus::IncompleteSourceMemoryAccess:
      return "incomplete_source_memory_access";
    case Route5PublicationStatus::NoMatch:
      return "no_match";
  }
  return "unavailable";
}

enum class Route5PublicationScope : unsigned char {
  None,
  CfgEdge,
  CurrentBlockJoin,
};

enum class Route5PublicationSourceKind : unsigned char {
  Unknown,
  Immediate,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  SelectMaterialization,
};

enum class Route5PublicationValueRole : unsigned char {
  None,
  Destination,
  Source,
  IncomingExpression,
};

struct Route5CfgEdgePublicationRecord {
  bool available = false;
  Route5PublicationStatus status = Route5PublicationStatus::Unavailable;
  const Block* predecessor_block = nullptr;
  std::string_view predecessor_label;
  BlockLabelId predecessor_label_id = kInvalidBlockLabel;
  const Block* successor_block = nullptr;
  std::string_view successor_label;
  BlockLabelId successor_label_id = kInvalidBlockLabel;
  const Inst* destination_instruction = nullptr;
  const PhiInst* destination_phi = nullptr;
  std::size_t destination_instruction_index = 0;
  Route1SourceValueIdentity destination_value;
  std::string_view destination_value_name;
  ValueNameId destination_value_name_id = kInvalidValueName;
  TypeKind destination_value_type = TypeKind::Void;
  Route1SourceValueIdentity source_value;
  std::string_view source_value_name;
  ValueNameId source_value_name_id = kInvalidValueName;
  Value::Kind source_value_kind = Value::Kind::Immediate;
  TypeKind source_value_type = TypeKind::Void;
  bool explicit_no_source = false;
  Route5PublicationSourceKind source_producer_kind =
      Route5PublicationSourceKind::Unknown;
  const Inst* source_producer_instruction = nullptr;
  BlockLabelId source_producer_block_label_id = kInvalidBlockLabel;
  std::optional<std::size_t> source_producer_instruction_index;
  bool source_memory_identity_available = false;
  Route3MemoryAccessRecord source_memory_access;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route5CurrentBlockJoinSourceRecord {
  bool available = false;
  Route5PublicationStatus status = Route5PublicationStatus::Unavailable;
  const Block* successor_block = nullptr;
  std::string_view successor_label;
  BlockLabelId successor_label_id = kInvalidBlockLabel;
  std::string_view predecessor_label;
  BlockLabelId predecessor_label_id = kInvalidBlockLabel;
  const Inst* destination_instruction = nullptr;
  const PhiInst* destination_phi = nullptr;
  std::size_t destination_instruction_index = 0;
  Route1SourceValueIdentity destination_value;
  std::string_view destination_value_name;
  TypeKind destination_value_type = TypeKind::Void;
  Route1SourceValueIdentity source_value;
  std::string_view source_value_name;
  Value::Kind source_value_kind = Value::Kind::Immediate;
  TypeKind source_value_type = TypeKind::Void;
  Route5PublicationSourceKind source_producer_kind =
      Route5PublicationSourceKind::Unknown;
  const Inst* source_producer_instruction = nullptr;
  std::optional<std::size_t> source_producer_instruction_index;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route5PublicationValueRecord {
  bool available = false;
  Route5PublicationScope scope = Route5PublicationScope::None;
  Route5PublicationStatus status = Route5PublicationStatus::Unavailable;
  Route5PublicationValueRole value_role = Route5PublicationValueRole::None;
  Route1SourceValueIdentity value;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::string_view predecessor_label;
  BlockLabelId predecessor_label_id = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  Route5CfgEdgePublicationRecord edge;
  Route5CurrentBlockJoinSourceRecord join;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route5EdgeJoinSourceIndex {
  const Function* function = nullptr;
  std::vector<Route5CfgEdgePublicationRecord> edge_records;
  std::vector<Route5CurrentBlockJoinSourceRecord> join_records;
  std::vector<Route5PublicationValueRecord> value_records;

  [[nodiscard]] explicit operator bool() const { return function != nullptr; }
};

[[nodiscard]] Route5PublicationSourceKind
route5_publication_source_kind(Route1ProducerKind kind);

[[nodiscard]] Route5CfgEdgePublicationRecord
route5_cfg_edge_publication_record(
    const Block* predecessor_block,
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id = kInvalidValueName,
    ValueNameId source_value_name_id = kInvalidValueName);

[[nodiscard]] std::vector<Route5CurrentBlockJoinSourceRecord>
route5_current_block_join_source_records(const Block* successor_block);

[[nodiscard]] Route5PublicationValueRecord
route5_edge_destination_value_record(
    const Route5CfgEdgePublicationRecord& edge);

[[nodiscard]] Route5PublicationValueRecord
route5_edge_source_value_record(const Route5CfgEdgePublicationRecord& edge);

[[nodiscard]] Route5PublicationValueRecord
route5_join_destination_value_record(
    const Route5CurrentBlockJoinSourceRecord& join);

[[nodiscard]] Route5PublicationValueRecord
route5_join_source_value_record(
    const Route5CurrentBlockJoinSourceRecord& join);

[[nodiscard]] Route5EdgeJoinSourceIndex
route5_build_edge_join_source_index(const Function& function);

[[nodiscard]] Route5CfgEdgePublicationRecord
route5_find_cfg_edge_publication(
    const Route5EdgeJoinSourceIndex& index,
    const Block& predecessor_block,
    const Block& successor_block,
    const Value& destination_value);

[[nodiscard]] Route5CurrentBlockJoinSourceRecord
route5_find_current_block_join_source(
    const Route5EdgeJoinSourceIndex& index,
    const Block& successor_block,
    const Value& destination_value,
    const Value& source_value);

struct CallArgumentSourceProducerMaterialization {
  bool available = false;
  std::size_t arg_index = 0;
  CallArgumentSourceProducerKind producer_kind =
      CallArgumentSourceProducerKind::Unknown;
  const Inst* producer_instruction = nullptr;
  std::size_t producer_instruction_index = 0;
  const Value* produced_value = nullptr;
  bool materializable = false;
};

enum class Route6CallUseStatus : unsigned char {
  Unavailable,
  Available,
  MissingCall,
  WrongCall,
  MissingArgument,
  MissingResult,
  MissingSourceRelationship,
  MissingSourceValue,
  MissingSourceProducer,
  MissingDirectGlobal,
  MissingMemorySource,
  MissingPublicationSource,
  AbiBoundExcluded,
  DuplicateRelationship,
  DuplicateResultLane,
  NoMatch,
};

[[nodiscard]] constexpr std::string_view route6_call_use_status_name(
    Route6CallUseStatus status) {
  switch (status) {
    case Route6CallUseStatus::Unavailable:
      return "unavailable";
    case Route6CallUseStatus::Available:
      return "available";
    case Route6CallUseStatus::MissingCall:
      return "missing_call";
    case Route6CallUseStatus::WrongCall:
      return "wrong_call";
    case Route6CallUseStatus::MissingArgument:
      return "missing_argument";
    case Route6CallUseStatus::MissingResult:
      return "missing_result";
    case Route6CallUseStatus::MissingSourceRelationship:
      return "missing_source_relationship";
    case Route6CallUseStatus::MissingSourceValue:
      return "missing_source_value";
    case Route6CallUseStatus::MissingSourceProducer:
      return "missing_source_producer";
    case Route6CallUseStatus::MissingDirectGlobal:
      return "missing_direct_global";
    case Route6CallUseStatus::MissingMemorySource:
      return "missing_memory_source";
    case Route6CallUseStatus::MissingPublicationSource:
      return "missing_publication_source";
    case Route6CallUseStatus::AbiBoundExcluded:
      return "abi_bound_excluded";
    case Route6CallUseStatus::DuplicateRelationship:
      return "duplicate_relationship";
    case Route6CallUseStatus::DuplicateResultLane:
      return "duplicate_result_lane";
    case Route6CallUseStatus::NoMatch:
      return "no_match";
  }
  return "unknown";
}

enum class Route6CallUseSourceKind : unsigned char {
  Unknown,
  Immediate,
  ArgumentValue,
  BaseValue,
  LoadLocal,
  LoadGlobal,
  Binary,
  DirectGlobalSelectChain,
  MemorySource,
  PublicationSource,
  AbiBoundExcluded,
};

enum class Route6CallUseValueRole : unsigned char {
  None,
  ArgumentSource,
  ArgumentBase,
  Result,
  ResultLane,
  PublicationSource,
};

struct Route6CallArgumentSourceRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  const CallInst* call = nullptr;
  std::size_t call_instruction_index = 0;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::string_view callee;
  LinkNameId callee_link_name_id = kInvalidLinkName;
  std::size_t arg_index = 0;
  const Value* argument_value = nullptr;
  Route1SourceValueIdentity source_value;
  std::optional<std::size_t> source_value_id;
  std::optional<std::string_view> source_value_name;
  std::optional<std::size_t> source_base_value_id;
  std::optional<std::string_view> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  CallArgumentSourceEncodingKind source_encoding =
      CallArgumentSourceEncodingKind::None;
  Route6CallUseSourceKind source_kind = Route6CallUseSourceKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallArgumentSourceProducerRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  Route6CallArgumentSourceRecord argument_source;
  Route1ProducerRecord producer;
  Route1MaterializationAvailability materialization;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallArgumentDirectGlobalDependencyRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  Route6CallArgumentSourceRecord argument_source;
  Route2SelectChainDirectGlobalDependencyRecord direct_global_dependency;
  std::string_view source_value_name;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallArgumentPublicationSourceRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  Route6CallArgumentSourceRecord argument_source;
  Route6CallUseSourceKind source_kind = Route6CallUseSourceKind::Unknown;
  std::optional<std::size_t> source_value_id;
  std::optional<std::size_t> source_base_value_id;
  std::optional<std::string_view> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  Route3MemoryAccessRecord memory_source;
  Route4CurrentBlockPublicationRecord current_block_publication_source;
  Route5CfgEdgePublicationRecord edge_publication_source;
  bool abi_bound_excluded = false;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallResultSourceRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  const CallInst* call = nullptr;
  std::size_t call_instruction_index = 0;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::string_view callee;
  LinkNameId callee_link_name_id = kInvalidLinkName;
  const Value* result_value = nullptr;
  Route1SourceValueIdentity result_identity;
  Route6CallUseValueRole value_role = Route6CallUseValueRole::Result;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallResultLaneSourceRecord {
  bool available = false;
  Route6CallUseStatus status = Route6CallUseStatus::Unavailable;
  Route6CallResultSourceRecord result_source;
  std::size_t lane_index = 0;
  const Value* lane_value = nullptr;
  Route1SourceValueIdentity lane_identity;
  bool aliases_primary_result = false;
  Route6CallUseValueRole value_role = Route6CallUseValueRole::ResultLane;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route6CallUseSourceIndex {
  const Function* function = nullptr;
  std::vector<Route6CallArgumentSourceRecord> argument_source_records;
  std::vector<Route6CallArgumentSourceProducerRecord> argument_producer_records;
  std::vector<Route6CallArgumentDirectGlobalDependencyRecord>
      direct_global_records;
  std::vector<Route6CallArgumentPublicationSourceRecord>
      publication_source_records;
  std::vector<Route6CallResultSourceRecord> result_records;
  std::vector<Route6CallResultLaneSourceRecord> result_lane_records;

  [[nodiscard]] explicit operator bool() const { return function != nullptr; }
};

enum class ComparisonProducerKind : unsigned char {
  Unknown,
  Immediate,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  Select,
};

[[nodiscard]] constexpr std::string_view comparison_producer_kind_name(
    ComparisonProducerKind kind) {
  switch (kind) {
    case ComparisonProducerKind::Unknown:
      return "unknown";
    case ComparisonProducerKind::Immediate:
      return "immediate";
    case ComparisonProducerKind::LoadLocal:
      return "load_local";
    case ComparisonProducerKind::LoadGlobal:
      return "load_global";
    case ComparisonProducerKind::Cast:
      return "cast";
    case ComparisonProducerKind::Binary:
      return "binary";
    case ComparisonProducerKind::Select:
      return "select";
  }
  return "unknown";
}

struct ComparisonOperandProducer {
  bool available = false;
  ComparisonProducerKind producer_kind = ComparisonProducerKind::Unknown;
  const Inst* producer_instruction = nullptr;
  std::size_t producer_instruction_index = 0;
  const Value* produced_value = nullptr;
  std::optional<std::int64_t> integer_constant;
};

struct FusedCompareOperandProducerFacts {
  bool available = false;
  std::optional<ComparisonOperandProducer> lhs;
  std::optional<ComparisonOperandProducer> rhs;
};

struct MaterializedConditionProducerIdentity {
  bool available = false;
  const BinaryInst* binary = nullptr;
  std::size_t instruction_index = 0;
  std::string condition_value_name;
  std::optional<ComparisonOperandProducer> lhs;
  std::optional<ComparisonOperandProducer> rhs;
};

enum class Route7ComparisonStatus : unsigned char {
  Unavailable,
  Available,
  MissingBlock,
  MissingInstruction,
  WrongInstruction,
  NonComparison,
  MissingConditionValue,
  MissingOperandProducer,
  DuplicateProducer,
  AbsentProvenance,
  NoMatch,
};

[[nodiscard]] constexpr std::string_view route7_comparison_status_name(
    Route7ComparisonStatus status) {
  switch (status) {
    case Route7ComparisonStatus::Unavailable:
      return "unavailable";
    case Route7ComparisonStatus::Available:
      return "available";
    case Route7ComparisonStatus::MissingBlock:
      return "missing_block";
    case Route7ComparisonStatus::MissingInstruction:
      return "missing_instruction";
    case Route7ComparisonStatus::WrongInstruction:
      return "wrong_instruction";
    case Route7ComparisonStatus::NonComparison:
      return "non_comparison";
    case Route7ComparisonStatus::MissingConditionValue:
      return "missing_condition_value";
    case Route7ComparisonStatus::MissingOperandProducer:
      return "missing_operand_producer";
    case Route7ComparisonStatus::DuplicateProducer:
      return "duplicate_producer";
    case Route7ComparisonStatus::AbsentProvenance:
      return "absent_provenance";
    case Route7ComparisonStatus::NoMatch:
      return "no_match";
  }
  return "unavailable";
}

enum class Route7ComparisonOperandRole : unsigned char {
  None,
  Lhs,
  Rhs,
  ConditionValue,
};

enum class Route7BranchConditionKind : unsigned char {
  Unknown,
  FusedCompare,
  MaterializedCondition,
};

struct Route7ComparisonOperandRecord {
  bool available = false;
  Route7ComparisonStatus status = Route7ComparisonStatus::Unavailable;
  Route7ComparisonOperandRole role = Route7ComparisonOperandRole::None;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t before_instruction_index = 0;
  Route1SourceValueIdentity value;
  ComparisonProducerKind producer_kind = ComparisonProducerKind::Unknown;
  const Inst* producer_instruction = nullptr;
  std::size_t producer_instruction_index = 0;
  Route1SourceValueIdentity produced_value;
  std::optional<std::int64_t> integer_constant;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route7ComparisonInstructionRecord {
  bool available = false;
  Route7ComparisonStatus status = Route7ComparisonStatus::Unavailable;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  const Inst* instruction = nullptr;
  const BinaryInst* binary = nullptr;
  std::size_t instruction_index = 0;
  Route1SourceValueIdentity condition_value;
  BinaryOpcode predicate = BinaryOpcode::Eq;
  TypeKind compare_type = TypeKind::Void;
  Route1SourceValueIdentity lhs_value;
  Route1SourceValueIdentity rhs_value;
  Route7ComparisonOperandRecord lhs;
  Route7ComparisonOperandRecord rhs;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route7BranchConditionRecord {
  bool available = false;
  Route7ComparisonStatus status = Route7ComparisonStatus::Unavailable;
  Route7BranchConditionKind kind = Route7BranchConditionKind::Unknown;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  Route1SourceValueIdentity condition_value;
  std::string_view true_label;
  BlockLabelId true_label_id = kInvalidBlockLabel;
  std::string_view false_label;
  BlockLabelId false_label_id = kInvalidBlockLabel;
  Route7ComparisonInstructionRecord comparison;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route7ComparisonConditionIndex {
  const Function* function = nullptr;
  const Block* block = nullptr;
  std::vector<Route7ComparisonInstructionRecord> comparison_records;
  std::vector<Route7ComparisonOperandRecord> operand_records;
  std::vector<Route7BranchConditionRecord> branch_condition_records;

  [[nodiscard]] explicit operator bool() const {
    return function != nullptr || block != nullptr;
  }
};

enum class Route8ReturnChainStatus : unsigned char {
  Unavailable,
  Available,
  MissingBlock,
  MissingInstruction,
  MissingChainValue,
  MissingTerminalValue,
  MissingNextOperandValue,
  DuplicateRecord,
  NoMatch,
};

[[nodiscard]] constexpr std::string_view route8_return_chain_status_name(
    Route8ReturnChainStatus status) {
  switch (status) {
    case Route8ReturnChainStatus::Unavailable:
      return "unavailable";
    case Route8ReturnChainStatus::Available:
      return "available";
    case Route8ReturnChainStatus::MissingBlock:
      return "missing_block";
    case Route8ReturnChainStatus::MissingInstruction:
      return "missing_instruction";
    case Route8ReturnChainStatus::MissingChainValue:
      return "missing_chain_value";
    case Route8ReturnChainStatus::MissingTerminalValue:
      return "missing_terminal_value";
    case Route8ReturnChainStatus::MissingNextOperandValue:
      return "missing_next_operand_value";
    case Route8ReturnChainStatus::DuplicateRecord:
      return "duplicate_record";
    case Route8ReturnChainStatus::NoMatch:
      return "no_match";
  }
  return "unavailable";
}

struct Route8ReturnChainValueKey {
  const Function* function = nullptr;
  const Block* block = nullptr;
  std::string_view function_name;
  LinkNameId function_link_name_id = kInvalidLinkName;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  Route1SourceValueIdentity chain_value;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr || !block_label.empty() ||
           block_label_id != kInvalidBlockLabel || bool(chain_value);
  }
};

struct Route8ReturnChainRecord {
  bool available = false;
  Route8ReturnChainStatus status = Route8ReturnChainStatus::Unavailable;
  Route8ReturnChainValueKey key;
  Route1SourceValueIdentity terminal_return_value;
  Route1SourceValueIdentity next_operand_value;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct Route8ReturnChainIndex {
  const Function* function = nullptr;
  const Block* block = nullptr;
  std::vector<Route8ReturnChainRecord> records;

  [[nodiscard]] explicit operator bool() const {
    return function != nullptr || block != nullptr;
  }
};

enum class RouteIndexRoute : unsigned char {
  Unknown,
  Route4PublicationAvailability,
  Route7ComparisonCondition,
};

enum class RouteIndexOwnerScope : unsigned char {
  None,
  Function,
  Block,
};

enum class RouteIndexRecordCategory : unsigned char {
  Unknown,
  Route4CurrentBlockPublication,
  Route4BlockEntryPublication,
  Route7ComparisonInstruction,
  Route7ComparisonOperand,
  Route7BranchCondition,
};

enum class RouteIndexRelationshipKind : unsigned char {
  None,
  Route4CurrentBlockPublication,
  Route4BlockEntryPublication,
  Route7Instruction,
  Route7Operand,
  Route7MaterializedCondition,
  Route7BranchCondition,
};

enum class RouteIndexValidationStatus : unsigned char {
  Unavailable,
  Valid,
  MissingRecord,
  StaleOwner,
  WrongRecordCategory,
  WrongRelationship,
  WrongKey,
  DuplicateReference,
  AbsentProvenance,
  NoMatch,
  Diverged,
};

struct RouteIndexRecordReference {
  RouteIndexRoute route = RouteIndexRoute::Unknown;
  RouteIndexOwnerScope owner_scope = RouteIndexOwnerScope::None;
  RouteIndexRecordCategory record_category =
      RouteIndexRecordCategory::Unknown;
  RouteIndexRelationshipKind relationship =
      RouteIndexRelationshipKind::None;
  const Function* function = nullptr;
  const Block* block = nullptr;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  std::size_t before_instruction_index = 0;
  Route1SourceValueIdentity value;
  Route7ComparisonOperandRole operand_role =
      Route7ComparisonOperandRole::None;
  std::size_t record_index = 0;
};

struct RouteIndexReferenceFacade {
  const Route4PublicationAvailabilityIndex* route4_publications = nullptr;
  const Route7ComparisonConditionIndex* route7_comparisons = nullptr;

  [[nodiscard]] explicit operator bool() const {
    return route4_publications != nullptr || route7_comparisons != nullptr;
  }
};

struct Route4IndexReferenceValidation {
  bool valid = false;
  RouteIndexValidationStatus status =
      RouteIndexValidationStatus::Unavailable;
  Route4PublicationAvailabilityStatus route_status =
      Route4PublicationAvailabilityStatus::Unavailable;
  RouteIndexRecordReference reference;
  const Route4CurrentBlockPublicationRecord* current_block_record = nullptr;
  const Route4BlockEntryPublicationRecord* block_entry_record = nullptr;

  [[nodiscard]] explicit operator bool() const { return valid; }
};

struct Route7IndexReferenceValidation {
  bool valid = false;
  RouteIndexValidationStatus status =
      RouteIndexValidationStatus::Unavailable;
  Route7ComparisonStatus route_status =
      Route7ComparisonStatus::Unavailable;
  RouteIndexRecordReference reference;
  const Route7ComparisonInstructionRecord* comparison_record = nullptr;
  const Route7ComparisonOperandRecord* operand_record = nullptr;
  const Route7BranchConditionRecord* branch_condition_record = nullptr;

  [[nodiscard]] explicit operator bool() const { return valid; }
};

struct CallResultSourceIdentity {
  bool available = false;
  std::size_t call_instruction_index = 0;
  const Value* result_value = nullptr;
};

struct CallResultLaneSourceIdentity {
  bool available = false;
  std::size_t call_instruction_index = 0;
  std::size_t lane_index = 0;
  const Value* lane_value = nullptr;
  bool aliases_primary_result = false;
};

struct ReturnTerminator {
  std::optional<Value> value;
  std::vector<Value> return_lanes;
};

struct BranchTerminator {
  // Compatibility label spelling for dumps and raw-only BIR; target_label_id is
  // the semantic block reference when present.
  std::string target_label;
  BlockLabelId target_label_id = kInvalidBlockLabel;
};

struct CondBranchTerminator {
  Value condition;
  // Compatibility label spellings for dumps and raw-only BIR; label ids are the
  // semantic block references when present.
  std::string true_label;
  std::string false_label;
  BlockLabelId true_label_id = kInvalidBlockLabel;
  BlockLabelId false_label_id = kInvalidBlockLabel;
};

enum class TerminatorKind : unsigned char {
  Return,
  Branch,
  CondBranch,
};

struct Terminator {
  TerminatorKind kind = TerminatorKind::Return;
  std::optional<Value> value;
  std::vector<Value> return_lanes;
  Value condition;
  // Compatibility label spellings for dumps/raw-only BIR. Structured
  // BlockLabelId fields are authoritative when valid.
  std::string target_label;
  BlockLabelId target_label_id = kInvalidBlockLabel;
  std::string true_label;
  BlockLabelId true_label_id = kInvalidBlockLabel;
  std::string false_label;
  BlockLabelId false_label_id = kInvalidBlockLabel;

  Terminator() = default;
  Terminator(const ReturnTerminator& ret)
      : kind(TerminatorKind::Return), value(ret.value), return_lanes(ret.return_lanes) {}
  Terminator(const BranchTerminator& br)
      : kind(TerminatorKind::Branch),
        target_label(br.target_label),
        target_label_id(br.target_label_id) {}
  Terminator(const CondBranchTerminator& br)
      : kind(TerminatorKind::CondBranch),
        condition(br.condition),
        true_label(br.true_label),
        true_label_id(br.true_label_id),
        false_label(br.false_label),
        false_label_id(br.false_label_id) {}
};

struct Block {
  // Compatibility/display label spelling. label_id is the semantic block
  // identity when present.
  std::string label;
  std::vector<Inst> insts;
  Terminator terminator;
  BlockLabelId label_id = kInvalidBlockLabel;
};

struct AtomicOperation {
  AtomicOperationKind kind = AtomicOperationKind::None;
  // Compatibility/display label spelling. block_label_id is the semantic block
  // identity when present.
  std::string block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t inst_index = 0;
  TypeKind value_type = TypeKind::Void;
  std::size_t width_bytes = 0;
  std::optional<Value> result;
  std::optional<Value> pointer;
  std::optional<Value> value;
  std::optional<Value> expected;
  std::optional<Value> desired;
  AtomicOrdering ordering = AtomicOrdering::None;
  AtomicOrdering failure_ordering = AtomicOrdering::None;
  AtomicRmwOpcode rmw_opcode = AtomicRmwOpcode::None;
  AtomicResultMode result_mode = AtomicResultMode::None;
  AddressSpace address_space = AddressSpace::Default;
};

enum class FormalPointerAuthorityKind : unsigned char {
  Unknown,
  InternalOnly,
  NoExternalCaller,
};

[[nodiscard]] constexpr std::string_view formal_pointer_authority_kind_name(
    FormalPointerAuthorityKind kind) {
  switch (kind) {
    case FormalPointerAuthorityKind::Unknown:
      return "unknown";
    case FormalPointerAuthorityKind::InternalOnly:
      return "internal_only";
    case FormalPointerAuthorityKind::NoExternalCaller:
      return "no_external_caller";
  }
  return "unknown";
}

[[nodiscard]] constexpr bool formal_pointer_authority_allows_same_module_call_sources(
    FormalPointerAuthorityKind kind) {
  return kind == FormalPointerAuthorityKind::InternalOnly ||
         kind == FormalPointerAuthorityKind::NoExternalCaller;
}

struct Function {
  // Final/display function spelling. link_name_id is semantic identity when
  // present.
  std::string name;
  LinkNameId link_name_id = kInvalidLinkName;
  TypeKind return_type = TypeKind::Void;
  std::size_t return_size_bytes = 0;
  std::size_t return_align_bytes = 0;
  std::optional<CallResultAbiInfo> return_abi;
  CallingConv calling_convention = CallingConv::C;
  bool is_variadic = false;
  std::vector<Param> params;
  std::vector<LocalSlot> local_slots;
  std::vector<LocalArraySourceObjectRecord> local_array_source_objects;
  std::vector<LocalArrayAddressDerivationRecord> local_array_derivations;
  std::vector<LocalArrayElementPathRecord> local_array_element_paths;
  std::vector<LocalArraySelectedProofEdgePathRecord>
      local_array_selected_proof_edge_paths;
  std::vector<LocalArrayEndpointBridgeRecord> local_array_endpoint_bridges;
  std::vector<LocalArrayOrderedEffectSourceStream>
      local_array_ordered_effect_source_streams;
  std::vector<Block> blocks;
  std::vector<AtomicOperation> atomic_operations;
  bool is_declaration = false;
  FormalPointerAuthorityKind formal_pointer_authority =
      FormalPointerAuthorityKind::Unknown;
  GlobalAddressMaterializationPolicy address_materialization_policy =
      GlobalAddressMaterializationPolicy::Unspecified;
};

struct Module {
  std::string target_triple;
  std::string data_layout;
  StructuredTypeSpellingContext structured_types;
  std::vector<Global> globals;
  std::vector<StringConstant> string_constants;
  std::vector<Function> functions;
  NameTables names;
};

struct LocalArrayOrderedEffectSourceStreamLookup {
  const LocalArrayOrderedEffectSourceStream* stream = nullptr;
  bool duplicate = false;
};

[[nodiscard]] inline LocalArrayOrderedEffectSourceStreamLookup
find_local_array_ordered_effect_source_stream(
    const Function& function,
    const LocalArraySelectedProofEdgePathRecord& selected_path,
    const LocalArrayEndpointBridgeRecord& endpoint_bridge) {
  LocalArrayOrderedEffectSourceStreamLookup lookup;
  const LocalArrayOrderedEffectSourceStream* found = nullptr;
  for (const auto& stream :
       function.local_array_ordered_effect_source_streams) {
    if (!local_array_ordered_effect_source_stream_matches_path(
            stream, selected_path) ||
        !local_array_ordered_effect_source_stream_matches_endpoint_bridge(
            stream, endpoint_bridge)) {
      continue;
    }
    if (found != nullptr) {
      lookup.duplicate = true;
      return lookup;
    }
    found = &stream;
  }
  lookup.stream = found;
  return lookup;
}

[[nodiscard]] inline LocalArrayIntervalEffectRecord
evaluate_local_array_interval_effect(
    const Function& function,
    const LocalArraySelectedProofEdgePathRecord* selected_path,
    const LocalArrayEndpointBridgeRecord* endpoint_bridge,
    bool prepared_bir_coordinate_confusion = false,
    bool raw_shape_only = false) {
  const auto stream_lookup =
      selected_path == nullptr || endpoint_bridge == nullptr
          ? LocalArrayOrderedEffectSourceStreamLookup{}
          : find_local_array_ordered_effect_source_stream(
                function, *selected_path, *endpoint_bridge);
  return evaluate_local_array_interval_effect(LocalArrayIntervalEffectInputs{
      .selected_path = selected_path,
      .endpoint_bridge = endpoint_bridge,
      .ordered_effect_sources = stream_lookup.stream,
      .duplicate_ordered_effect_source_stream = stream_lookup.duplicate,
      .prepared_bir_coordinate_confusion = prepared_bir_coordinate_confusion,
      .raw_shape_only = raw_shape_only,
  });
}

inline bool is_compare_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

inline TypeKind binary_operand_type(const BinaryInst& inst) {
  return inst.operand_type == TypeKind::Void ? inst.result.type : inst.operand_type;
}

inline TypeKind select_compare_type(const SelectInst& inst) {
  return inst.compare_type == TypeKind::Void ? inst.result.type : inst.compare_type;
}

inline std::optional<std::int64_t> parse_i32_return_immediate(const Function& function) {
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.insts.empty() ||
      block.terminator.kind != TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != Value::Kind::Immediate ||
      block.terminator.value->type != TypeKind::I32) {
    return std::nullopt;
  }

  return block.terminator.value->immediate;
}

std::string render_type(TypeKind type);
std::string render_binary_opcode(BinaryOpcode opcode);
std::string render_cast_opcode(CastOpcode opcode);
const CallArgumentSourceRelationship* find_call_argument_source_relationship(
    const CallInst& call,
    std::size_t arg_index);
CallArgumentPublicationSourceRouting find_call_argument_publication_source_routing(
    const CallInst& call,
    std::size_t arg_index);
CallArgumentSourceProducerMaterialization
find_call_argument_source_producer_materialization(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentSourceRecord route6_call_argument_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentSourceProducerRecord
route6_call_argument_source_producer_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentDirectGlobalDependencyRecord
route6_call_argument_direct_global_dependency_record(
    Route1SameBlockProducerQuery query,
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentPublicationSourceRecord
route6_call_argument_publication_source_record(
    Route1SameBlockProducerQuery query,
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index);
[[nodiscard]] Route6CallResultSourceRecord route6_call_result_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index);
[[nodiscard]] Route6CallResultLaneSourceRecord
route6_call_result_lane_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    const Value& value);
[[nodiscard]] Route6CallUseSourceIndex route6_build_call_use_source_index(
    const Function& function);
[[nodiscard]] Route6CallArgumentSourceRecord
route6_find_call_argument_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index);
// Checks that a Route 6 ArgumentValue source record still names the same call
// argument object and spelling. Prepared source-id agreement remains a
// consumer-side compatibility check.
[[nodiscard]] bool route6_call_argument_source_matches_argument_value_record(
    const Route6CallArgumentSourceRecord& source,
    const Value& value);
[[nodiscard]] Route6CallArgumentSourceProducerRecord
route6_find_call_argument_source_producer(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentDirectGlobalDependencyRecord
route6_find_call_argument_direct_global_dependency(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index);
[[nodiscard]] Route6CallArgumentPublicationSourceRecord
route6_find_call_argument_publication_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index);
[[nodiscard]] Route6CallResultSourceRecord route6_find_call_result_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    const Value& result_value);
[[nodiscard]] Route6CallResultLaneSourceRecord
route6_find_call_result_lane_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    const Value& lane_value);
std::optional<ComparisonOperandProducer> find_comparison_operand_producer(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
FusedCompareOperandProducerFacts find_fused_compare_operand_producer_facts(
    const Block& block,
    const Value& lhs,
    const Value& rhs,
    std::size_t before_instruction_index);
FusedCompareOperandProducerFacts route7_find_fused_compare_operand_producer_facts(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& lhs,
    const Value& rhs,
    std::size_t before_instruction_index);
MaterializedConditionProducerIdentity find_materialized_condition_producer_identity(
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
[[nodiscard]] Route7ComparisonOperandRecord route7_comparison_operand_record(
    const Block* block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7ComparisonInstructionRecord
route7_comparison_instruction_record(
    const Block* block,
    std::size_t instruction_index);
[[nodiscard]] Route7BranchConditionRecord route7_branch_condition_record(
    const Block* block);
[[nodiscard]] Route7ComparisonConditionIndex
route7_build_comparison_condition_index(const Function& function);
[[nodiscard]] Route7ComparisonConditionIndex
route7_build_comparison_condition_index(const Block& block);
[[nodiscard]] Route7ComparisonInstructionRecord
route7_find_comparison_instruction(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index);
[[nodiscard]] Route7ComparisonOperandRecord route7_find_comparison_operand(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7ComparisonInstructionRecord
route7_find_materialized_condition(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
[[nodiscard]] Route7BranchConditionRecord route7_find_branch_condition(
    const Route7ComparisonConditionIndex& index,
    const Block& block);
[[nodiscard]] Route8ReturnChainValueKey route8_return_chain_value_key(
    const Function* function,
    const Block& block,
    std::size_t instruction_index,
    const Value& chain_value,
    ValueNameId chain_value_name_id = kInvalidValueName);
[[nodiscard]] Route8ReturnChainRecord route8_return_chain_record(
    const Route8ReturnChainValueKey& key,
    const Value* terminal_return_value = nullptr,
    ValueNameId terminal_return_value_name_id = kInvalidValueName,
    const Value* next_operand_value = nullptr,
    ValueNameId next_operand_value_name_id = kInvalidValueName);
[[nodiscard]] Route8ReturnChainIndex route8_build_return_chain_index(
    const Function& function);
[[nodiscard]] Route8ReturnChainIndex route8_build_return_chain_index(
    const Block& block);
[[nodiscard]] Route8ReturnChainRecord route8_find_return_chain_record(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key);
[[nodiscard]] Route1SourceValueIdentity
route8_find_return_chain_terminal_value(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key);
[[nodiscard]] Route1SourceValueIdentity
route8_find_return_chain_next_operand_value(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key);
[[nodiscard]] Route4IndexReferenceValidation
route4_validate_current_block_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] Route4IndexReferenceValidation
route4_validate_block_entry_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& successor_block,
    const Value& destination_value);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_comparison_instruction_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_comparison_operand_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_materialized_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_branch_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route7ComparisonConditionIndex& route7_comparisons);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications,
    const Route7ComparisonConditionIndex& route7_comparisons);
[[nodiscard]] Route4IndexReferenceValidation
route_index_validate_current_block_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] Route4IndexReferenceValidation
route_index_validate_block_entry_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& successor_block,
    const Value& destination_value);
[[nodiscard]] Route7IndexReferenceValidation
route_index_validate_comparison_operand_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7IndexReferenceValidation
route_index_validate_materialized_condition_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
CallResultSourceIdentity find_call_result_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index);
CallResultLaneSourceIdentity find_call_result_lane_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    const Value& value);
std::string print(const Module& module);
bool validate(const Module& module, std::string* error);

}  // namespace c4c::backend::bir
