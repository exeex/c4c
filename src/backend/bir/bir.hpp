#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {

struct Module;

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
};

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
  }
  return "unknown";
}

enum class IntrinsicOperationKind : unsigned char {
  None,
  FAbs,
  Crc32W,
  VectorLoad,
  VectorAdd,
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
};

struct CallResultAbiInfo {
  TypeKind type = TypeKind::Void;
  AbiValueClass primary_class = AbiValueClass::None;
  AbiValueClass secondary_class = AbiValueClass::None;
  bool returned_in_memory = false;
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

struct InlineAsmMetadata {
  // Inline assembly payload text is final spelling passed through for dumps and
  // target emission diagnostics; it is not BIR lookup authority.
  std::string asm_text;
  std::string constraints;
  std::string args_text;
  bool side_effects = false;
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
  bool has_immediate_operand = false;
  bool requires_immediate_operand = false;
  bool has_side_effects = false;
};

struct CallInst {
  std::optional<Value> result;
  std::string callee;
  // Direct user/extern calls carry LinkNameId when LIR provides one.
  // Runtime/intrinsic placeholder calls synthesized by BIR lowering keep this
  // invalid and use callee only as an explicit compatibility/display token.
  LinkNameId callee_link_name_id = kInvalidLinkName;
  std::optional<Value> callee_value;
  std::vector<Value> args;
  std::vector<TypeKind> arg_types;
  std::vector<CallArgAbiInfo> arg_abi;
  // Final type spelling for aggregate call results; return_type remains the
  // scalar ABI class and structured_return_type_name is display/dump text.
  std::optional<std::string> structured_return_type_name;
  std::string return_type_name;
  TypeKind return_type = TypeKind::Void;
  std::optional<CallResultAbiInfo> result_abi;
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

struct ReturnTerminator {
  std::optional<Value> value;
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
  Terminator(const ReturnTerminator& ret) : kind(TerminatorKind::Return), value(ret.value) {}
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
  std::vector<Block> blocks;
  std::vector<AtomicOperation> atomic_operations;
  bool is_declaration = false;
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
std::string print(const Module& module);
bool validate(const Module& module, std::string* error);

}  // namespace c4c::backend::bir
