#pragma once

#include "ids.hpp"
#include "storage.hpp"
#include "type.hpp"
#include "../pipeline/identity.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace c4c::backend::bir {

struct ParameterDef {
  std::uint32_t ordinal = 0;
};

struct InstResultDef {
  InstId instruction{};
  std::uint16_t result_index = 0;
};

struct UnresolvedDef {};

struct IntegerConstant {
  std::int64_t value = 0;
};

struct FloatingConstant {
  std::uint64_t bits = 0;
};

struct LabelAddressConstant {
  BlockId target{};
};

enum class SpecialConstantKind : std::uint8_t {
  Null, Undef, Poison, ZeroInitializer, True, False,
};

struct SpecialConstant {
  SpecialConstantKind kind = SpecialConstantKind::Undef;
};

using ConstantPayload = std::variant<IntegerConstant, FloatingConstant,
                                     LabelAddressConstant, SpecialConstant>;

struct ConstantDefinition {
  Type type{};
  ConstantPayload payload = IntegerConstant{};
};

struct ConstantDef {
  ConstantId constant{};
};

struct StringData {
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_length = 0;
};

enum class ReturnExtension : std::uint8_t { None, SignExt, ZeroExt };

struct FallbackExternalName {
  std::string name;
};

struct ExternalDeclaration {
  std::string source_name;
  Type return_type;
  ReturnExtension return_extension = ReturnExtension::None;
  std::variant<LinkNameId, FallbackExternalName> identity =
      FallbackExternalName{};
};

struct FallbackGlobalName {
  std::string name;
};

struct GlobalInitializer {
  std::string opaque_payload;
  std::vector<LinkNameId> function_links;
};

enum class SymbolVisibility : std::uint8_t { Default, Hidden, Protected };

struct GlobalObject {
  std::string source_name;
  Type object_type;
  std::variant<LinkNameId, FallbackGlobalName> identity = FallbackGlobalName{};
  int alignment = 0;
  bool is_internal = false;
  bool is_weak = false;
  bool is_const = false;
  bool is_extern_declaration = true;
  SymbolVisibility visibility = SymbolVisibility::Default;
  std::optional<GlobalInitializer> initializer;
};

struct IntrinsicRequirements {
  bool need_va_start = false;
  bool need_va_end = false;
  bool need_va_copy = false;
  bool need_memcpy = false;
  bool need_memset = false;
  bool need_stacksave = false;
  bool need_stackrestore = false;
  bool need_abs = false;
  bool need_ptrmask = false;
  bool prefer_semantic_va_ops = false;
};

inline bool operator==(const IntrinsicRequirements& lhs,
                       const IntrinsicRequirements& rhs) noexcept {
  return lhs.need_va_start == rhs.need_va_start &&
         lhs.need_va_end == rhs.need_va_end &&
         lhs.need_va_copy == rhs.need_va_copy &&
         lhs.need_memcpy == rhs.need_memcpy &&
         lhs.need_memset == rhs.need_memset &&
         lhs.need_stacksave == rhs.need_stacksave &&
         lhs.need_stackrestore == rhs.need_stackrestore &&
         lhs.need_abs == rhs.need_abs && lhs.need_ptrmask == rhs.need_ptrmask &&
         lhs.prefer_semantic_va_ops == rhs.prefer_semantic_va_ops;
}

inline bool operator!=(const IntrinsicRequirements& lhs,
                       const IntrinsicRequirements& rhs) noexcept {
  return !(lhs == rhs);
}

struct SpecializationMetadata {
  std::string spec_key;
  std::string template_origin;
  std::string mangled_name;
  LinkNameId mangled_link_name{};
};

struct ValueDef {
  ValueKind kind = ValueKind::Parameter;
  Type type{};
  std::optional<SourceValueId> source_id;
  std::variant<UnresolvedDef, ParameterDef, InstResultDef, ConstantDef>
      definition = UnresolvedDef{};
};

enum class Opcode : std::uint8_t {
  InlineAsm,
  Store,
  Load,
  GetElementPtr,
  Abs,
  Call,
  Binary,
  Compare,
  Select,
  SelectedMemcpy,
  Amd64SysVOverflowAggregateMemcpy,
  Cast,
  Phi,
  AllocaAuthority,
  StackSaveAuthority,
  StackRestoreAuthority,
};

struct InlineAsmNode {
  std::string asm_text;
  std::string constraint_text;
  std::vector<std::string> clobbers;
  bool side_effects = false;
};

struct StoreNode {
  GlobalObjectId destination{};
  Type stored_type{};
};

struct LocalStoreAuthorityNode {
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type stored_type{};
  std::int64_t immediate = 0;
  bool live = false;
};

struct LoadNode {
  GlobalObjectId source{};
  Type loaded_type{};
};

// Receipt of one producer-selected direct local-scalar load.  The local
// pointer/object facts stay native source identities; no local spelling is
// retained or recovered.
struct LocalLoadAuthorityNode {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type loaded_type{};
  bool live = false;
};

// A GEP base is deliberately narrower than a general pointer value.  The
// label-address alternative can only name the exact current-function constant
// ValueId that defines a LabelAddressConstant; it is not an SSA operand.
struct LabelAddressGepBase {
  ValueId value{};
};

// Receipt of the one producer-selected direct, non-expanded body parameter
// used as a typed GEP base.  This retains native LIR identity plus the Raw-BIR
// parameter ordinal; it is not a recoverable spelling or generic SSA base.
struct DirectPointerBodyParameterGepBase {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  Type pointer_type{TypeKind::Pointer};
  LinkNameId owner{};
};

using GetElementPtrBaseAuthority =
    std::variant<GlobalObjectId, LabelAddressGepBase,
                 DirectPointerBodyParameterGepBase>;

struct GetElementPtrBase {
  GetElementPtrBaseAuthority authority = GlobalObjectId{};

  GetElementPtrBase() = default;
  GetElementPtrBase(GlobalObjectId global) : authority(global) {}
  GetElementPtrBase(LabelAddressGepBase label) : authority(label) {}
  GetElementPtrBase(DirectPointerBodyParameterGepBase parameter)
      : authority(parameter) {}
};

struct GetElementPtrNode {
  GetElementPtrBase base{};
  Type element_type{};
  bool inbounds = false;
};

// Receipt of the producer-selected direct static-local-array GEP.  The native
// local-object binding and i64 immediate remain explicit; no local spelling is
// reconstructed as a generic GEP base.
struct LocalArrayGepAuthorityNode {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  Type element_type{};
  std::int64_t immediate_index = 0;
  bool live = false;
};

struct AbsNode {
  Type type{};
};

struct DirectScalarBodyParameterBinaryLhs {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  Type scalar_type{TypeKind::Void};
  LinkNameId owner{};
};

struct DirectScalarBodyParameterBinaryRhs {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  Type scalar_type{TypeKind::Void};
  LinkNameId owner{};
};

// Receipt of the one producer-authorized direct-scalar parameter used as the
// LHS of an integer truthiness comparison.  This remains a bounded authority
// carrier, not general parameter-use support.
struct DirectScalarBodyParameterTruthinessComparisonLhs {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  Type scalar_type{TypeKind::Void};
  LinkNameId owner{};
};

// Receipt of the one producer-authorized direct-pointer parameter used through
// PtrToInt as the LHS of an integer truthiness comparison.
struct DirectPointerBodyParameterTruthiness {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  Type pointer_type{TypeKind::Pointer};
  LinkNameId owner{};
};

// Receipt of one producer-authorized direct-scalar current-function parameter
// passed as a selected structured argument of a fixed direct call.
struct DirectScalarBodyParameterFixedDirectCallArgument {
  std::uint32_t source_value_id = 0;
  std::uint32_t parameter_index = 0;
  std::uint32_t argument_index = 0;
  Type scalar_type{TypeKind::Void};
  LinkNameId owner{};
};

struct CallNode {
  FunctionId callee{};
  std::optional<DirectScalarBodyParameterFixedDirectCallArgument>
      direct_scalar_argument;
};

enum class BinaryOpcode : std::uint8_t { FAdd, FSub, FMul, FNeg, Add, Mul };

struct BinaryNode {
  BinaryOpcode opcode = BinaryOpcode::FAdd;
  Type type{};
  std::optional<DirectScalarBodyParameterBinaryLhs> direct_scalar_lhs;
  std::optional<DirectScalarBodyParameterBinaryRhs> direct_scalar_rhs;
};

enum class ComparePredicate : std::uint8_t { Slt, OLt, Eq, Ne };

struct CompareNode {
  ComparePredicate predicate = ComparePredicate::Slt;
  Type type{};
  std::optional<DirectScalarBodyParameterTruthinessComparisonLhs>
      direct_scalar_truthiness_lhs;
  std::optional<DirectPointerBodyParameterTruthiness>
      direct_pointer_truthiness;
};

// This is intentionally a receipt of the producer-verified wide ffs select
// result, not a general select lowering.  Its condition and arms remain LIR
// producer authority and are not materialized in Raw BIR.
struct SelectNode {
  Type type{};
};

// Receipt of the one producer-selected fixed-aggregate byval memcpy authority.
// It deliberately carries source identities rather than presentation operands.
struct SelectedMemcpyNode {
  SourceValueId destination{};
  SourceValueId source{};
  SourceObjectId destination_object{};
  SourceObjectId source_object{};
  LinkNameId destination_object_owner{};
  LinkNameId source_object_owner{};
  Type pointer_type{TypeKind::Pointer};
  std::int64_t size_bytes = 0;
  bool destination_live_at_site = false;
  bool source_live_at_site = false;
};

// Receipt of the one producer-verified AMD64 SysV aggregate va_arg overflow
// copy.  The overflow source is derived storage, so it deliberately retains
// the checked field/load chain instead of inventing a source local object.
struct Amd64SysVOverflowAggregateMemcpyNode {
  SourceValueId va_list_pointer{};
  SourceObjectId va_list_object{};
  LinkNameId owner{};
  SourceValueId overflow_field_address{};
  SourceValueId overflow_pointer_load{};
  SourceValueId destination{};
  SourceObjectId destination_object{};
  SourceValueId final_load{};
  Type payload_type{};
  std::int64_t size_bytes = 0;
  bool va_list_live = false;
  bool destination_live = false;
};

enum class IntrinsicKind : std::uint8_t { Cttz, Ctlz, Ctpop };

struct IntrinsicCallNode {
  IntrinsicKind kind = IntrinsicKind::Ctpop;
  LinkNameId callee_link_name{};
  Type type{};
  std::optional<bool> zero_count_is_undef;
};

enum class CastKind : std::uint8_t { Trunc, SExt, FPTrunc, FPExt, SIToFP, UIToFP, FPToSI, FPToUI, PtrToInt };

struct CastNode {
  CastKind kind = CastKind::Trunc;
  Type from_type{};
  Type to_type{};
};

// Edge identity includes the successor occurrence: a conditional branch may
// legitimately contribute two distinct edges to the same destination.
struct PhiEdge {
  BlockId predecessor{};
  BlockId destination{};
  std::uint32_t occurrence = 0;
};

struct PhiIncoming {
  ValueId value{};
  PhiEdge edge{};
};

struct PhiNode {
  Type type{};
  std::vector<PhiIncoming> incoming;
};

// Receipt of one producer-selected hoisted local alloca.  These are source
// identities and typed authority, never a recovered local spelling.
struct AllocaAuthorityNode {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

// Receipt of the producer-selected VLA stack-save result.  This retains only
// its native current-function identity and typed local-object authority.
struct StackSaveAuthorityNode {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

// Receipt of the producer-selected VLA stack-restore checkpoint consumption.
// The transition kind is implicit in this one selected node; its saved pointer
// is retained both as an operand and as source authority.
struct StackRestoreAuthorityNode {
  SourceValueId saved_pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

using InstPayload =
    std::variant<InlineAsmNode, StoreNode, LoadNode, GetElementPtrNode,
    AbsNode, CallNode, BinaryNode, CompareNode, SelectNode, SelectedMemcpyNode,
    Amd64SysVOverflowAggregateMemcpyNode,
    IntrinsicCallNode, CastNode, PhiNode, AllocaAuthorityNode,
    LocalLoadAuthorityNode, LocalStoreAuthorityNode, LocalArrayGepAuthorityNode,
    StackSaveAuthorityNode, StackRestoreAuthorityNode>;

class BlockView;
class FunctionView;
class ModuleView;
class RawBir;
class CanonicalBir;
class ModuleBuilder;
class FunctionBuilder;
class FoundationVerifier;
class PipelineCheckpoint;
class PrivateOccurrenceCandidate;

namespace pipeline_internal {
struct ForkAccess;
}

struct JumpTerm {
  BlockId target{};
};

struct CondJumpTerm {
  ValueId condition{};
  BlockId true_target{};
  BlockId false_target{};
};

struct IndirectJumpTerm {
  ValueId address{};
  std::vector<BlockId> targets;
};

// A switch retains only the target-independent CFG authority: the integer
// selector, its default destination, and the ordered case destinations.  LIR
// presentation labels and case spellings are deliberately not carried here.
struct SwitchTerm {
  ValueId selector{};
  BlockId default_target{};
  std::vector<BlockId> case_targets;
};

struct ReturnTerm {
  std::optional<ValueId> value;
};

struct UnreachableTerm {};

using Terminator =
    std::variant<JumpTerm, CondJumpTerm, IndirectJumpTerm, SwitchTerm, ReturnTerm,
                 UnreachableTerm>;

struct FunctionSignature {
  Type return_type{};
  std::vector<Type> parameter_types;
  bool is_variadic = false;
};

struct FunctionMetadata {
  bool is_internal = false;
  bool can_elide_if_unreferenced = false;
};

inline bool operator==(const FunctionMetadata& lhs,
                       const FunctionMetadata& rhs) noexcept {
  return lhs.is_internal == rhs.is_internal &&
         lhs.can_elide_if_unreferenced == rhs.can_elide_if_unreferenced;
}

inline bool operator!=(const FunctionMetadata& lhs,
                       const FunctionMetadata& rhs) noexcept {
  return !(lhs == rhs);
}

struct StructField {
  Type type;
  StructNameId referenced_name{};
};

struct StructDeclaration {
  StructNameId name{};
  std::vector<StructField> fields;
  bool is_packed = false;
  bool is_opaque = false;
};

inline bool operator==(const FunctionSignature& lhs,
                       const FunctionSignature& rhs) noexcept {
  return lhs.return_type == rhs.return_type &&
         lhs.parameter_types == rhs.parameter_types &&
         lhs.is_variadic == rhs.is_variadic;
}

inline bool operator!=(const FunctionSignature& lhs,
                       const FunctionSignature& rhs) noexcept {
  return !(lhs == rhs);
}

namespace detail {

struct InstData {
  Opcode opcode = Opcode::InlineAsm;
  InstPayload payload = InlineAsmNode{};
  std::vector<ValueId> operands;
  std::vector<ValueId> results;
};

struct BlockData {
 private:
  IdOrder<InstId> instruction_order_;
  std::optional<Terminator> terminator_;
  std::string debug_name_;

  friend class ::c4c::backend::bir::FunctionView;
  friend class ::c4c::backend::bir::BlockView;
  friend class ::c4c::backend::bir::FunctionBuilder;
  friend class ::c4c::backend::bir::FoundationVerifier;
};

struct FunctionData {
 private:
  FunctionRevision revision_{};
  FunctionSignature signature_;
  bool is_declaration_ = false;
  bool is_internal_ = false;
  bool can_elide_if_unreferenced_ = false;
  std::string link_name_;
  SlotMap<BlockData, BlockId, FunctionId> blocks_;
  SlotMap<InstData, InstId, FunctionId> insts_;
  SlotMap<ValueDef, ValueId, FunctionId> values_;
  IdOrder<BlockId> block_order_;
  IdOrder<ValueId> value_order_;
  std::vector<ValueId> parameters_;
  std::unordered_map<std::uint32_t, ValueId> values_by_source_id_;

  friend class ::c4c::backend::bir::ModuleView;
  friend class ::c4c::backend::bir::FunctionView;
  friend class ::c4c::backend::bir::ModuleBuilder;
  friend class ::c4c::backend::bir::FunctionBuilder;
  friend class ::c4c::backend::bir::FoundationVerifier;
};

struct ModuleData {
 private:
  struct SpecializationSemanticKey {
    std::string spec_key;
    std::string template_origin;

    bool operator==(const SpecializationSemanticKey& other) const noexcept {
      return spec_key == other.spec_key &&
             template_origin == other.template_origin;
    }
  };
  struct SpecializationSemanticKeyHash {
    std::size_t operator()(const SpecializationSemanticKey& key) const noexcept {
      auto result = std::hash<std::string>{}(key.spec_key);
      return detail::hash_combine(
          result, std::hash<std::string>{}(key.template_origin));
    }
  };
  struct LinkNameData {
    c4c::LinkNameId source_id = c4c::kInvalidLinkName;
    std::string spelling;
  };
  struct StructNameData {
    c4c::StructNameId source_id = c4c::kInvalidStructName;
    std::string spelling;
  };

  ModuleEpoch epoch_ = 0;
  ModuleRevision revision_{};
  PipelineStageStamp stage_stamp_{};
  SlotMap<FunctionData, FunctionId, ModuleEpoch> functions_;
  IdOrder<FunctionId> function_order_;
  std::unordered_map<std::string, FunctionId> functions_by_link_name_;
  std::vector<LinkNameData> link_names_;
  std::unordered_map<c4c::LinkNameId, LinkNameId> link_names_by_source_id_;
  // Ordered rows and typed IDs are primary; this verifier-checked spelling
  // mirror only enforces ABI lookup and uniqueness.
  std::unordered_map<std::string, LinkNameId> link_names_by_spelling_;
  std::vector<StructNameData> struct_names_;
  std::unordered_map<c4c::StructNameId, StructNameId> struct_names_by_source_id_;
  // Ordered rows and typed source/BIR IDs are primary; this verifier-checked
  // spelling mirror only provides lookup and uniqueness.
  std::unordered_map<std::string, StructNameId> struct_names_by_spelling_;
  std::vector<StructDeclaration> struct_decls_;
  // Structured-authority index: typed StructNameId to ordered StructDeclId.
  std::unordered_map<StructNameId, StructDeclId> struct_decls_by_name_;
  std::vector<ConstantDefinition> constants_;
  std::vector<StringData> string_data_;
  // The source pool name is explicit source identity, while StringDataId and
  // order are BIR identity; this verifier-checked map is a secondary index.
  std::unordered_map<std::string, StringDataId> string_data_by_name_;
  std::vector<ExternalDeclaration> external_decls_;
  // ExternalDeclId and the LinkNameId/fallback identity variant are primary;
  // source spelling is a verifier-checked secondary lookup/uniqueness index.
  std::unordered_map<std::string, ExternalDeclId> external_decls_by_name_;
  std::unordered_map<LinkNameId, ExternalDeclId> external_decls_by_link_name_;
  std::vector<GlobalObject> globals_;
  // GlobalObjectId and the LinkNameId/fallback identity variant are primary;
  // source spelling is a verifier-checked secondary lookup/uniqueness index.
  std::unordered_map<std::string, GlobalObjectId> globals_by_name_;
  std::unordered_map<LinkNameId, GlobalObjectId> globals_by_link_name_;
  std::vector<SpecializationMetadata> specializations_;
  std::unordered_map<SpecializationSemanticKey, SpecializationId,
                     SpecializationSemanticKeyHash>
      specializations_by_semantic_key_;
  std::unordered_map<LinkNameId, SpecializationId>
      specializations_by_link_name_;
  IntrinsicRequirements intrinsic_requirements_{};

  friend class ::c4c::backend::bir::ModuleView;
  friend class ::c4c::backend::bir::FunctionView;
  friend class ::c4c::backend::bir::ModuleBuilder;
  friend class ::c4c::backend::bir::FunctionBuilder;
  friend class ::c4c::backend::bir::FoundationVerifier;
  friend class ::c4c::backend::bir::RawBir;
  friend class ::c4c::backend::bir::CanonicalBir;
  friend class ::c4c::backend::bir::PipelineCheckpoint;
  friend class ::c4c::backend::bir::PrivateOccurrenceCandidate;
  friend struct ::c4c::backend::bir::pipeline_internal::ForkAccess;
};

}  // namespace detail

}  // namespace c4c::backend::bir
