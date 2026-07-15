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
  // Bootstrap compatibility storage: instruction identity lives in the
  // function instruction arena, while this index selects one entry from the
  // producer's ordered result list.
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
  // Concrete type is a value fact and is intentionally not inferred from the
  // producer's NodeKind traits.
  Type type{};
  std::optional<SourceValueId> source_id;
  std::variant<UnresolvedDef, ParameterDef, InstResultDef, ConstantDef>
      definition = UnresolvedDef{};
};

enum class NodeKind : std::uint8_t {
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

// Compatibility spelling for clients that still describe the closed node
// vocabulary as opcodes. New pass code should use NodeKind.
using Opcode = NodeKind;

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

using GetElementPtrBaseAuthority =
    std::variant<GlobalObjectId, LabelAddressGepBase>;

struct GetElementPtrBase {
  GetElementPtrBaseAuthority authority = GlobalObjectId{};

  GetElementPtrBase() = default;
  GetElementPtrBase(GlobalObjectId global) : authority(global) {}
  GetElementPtrBase(LabelAddressGepBase label) : authority(label) {}
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

struct CallNode {
  FunctionId callee{};
};

enum class BinaryOpcode : std::uint8_t { FAdd, FMul, Add, Mul };

struct BinaryNode {
  BinaryOpcode opcode = BinaryOpcode::FAdd;
  Type type{};
};

enum class ComparePredicate : std::uint8_t { Slt, OLt, Eq };

struct CompareNode {
  ComparePredicate predicate = ComparePredicate::Slt;
  Type type{};
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

enum class CastKind : std::uint8_t { Trunc, SExt, FPTrunc, FPExt, SIToFP, UIToFP, FPToSI, FPToUI };

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

enum class NodeFamily : std::uint8_t { Semantic, Memory, Call, Authority };
enum class OperandArityPolicy : std::uint8_t { Fixed, Variable };
enum class ResultArityPolicy : std::uint8_t { Zero, One, Many };
enum class NodeEffect : std::uint8_t {
  None,
  ReadsMemory,
  WritesMemory,
  ReadsAndWritesMemory,
  Unknown,
};
enum class NodeStage : std::uint8_t {
  Raw = 1,
  Canonical = 2,
  Prepared = 4,
};

struct NodeKindDescriptor {
  NodeFamily family;
  bool is_binary;
  OperandArityPolicy operand_arity;
  std::uint16_t minimum_operands;
  std::uint16_t maximum_operands;
  ResultArityPolicy result_arity;
  NodeEffect effects;
  std::uint8_t legal_stages;
};

namespace detail {

constexpr std::uint16_t variable_arity = UINT16_MAX;
constexpr std::uint8_t all_node_stages =
    static_cast<std::uint8_t>(NodeStage::Raw) |
    static_cast<std::uint8_t>(NodeStage::Canonical) |
    static_cast<std::uint8_t>(NodeStage::Prepared);

template <class... Payloads>
bool accepts_one_of(const InstPayload& payload) noexcept {
  return (std::holds_alternative<Payloads>(payload) || ...);
}

template <NodeKind Kind, NodeFamily Family, bool Binary,
          OperandArityPolicy OperandPolicy, std::uint16_t MinimumOperands,
          std::uint16_t MaximumOperands, ResultArityPolicy ResultPolicy,
          NodeEffect Effects, std::uint8_t LegalStages, class... Payloads>
struct node_kind_schema {
  static constexpr NodeKindDescriptor descriptor{
      Family, Binary, OperandPolicy, MinimumOperands, MaximumOperands,
      ResultPolicy, Effects, LegalStages};

  static bool accepts(const InstPayload& payload) noexcept {
    return accepts_one_of<Payloads...>(payload);
  }
};

template <NodeKind>
struct node_kind_traits;

template <>
struct node_kind_traits<NodeKind::InlineAsm>
    : node_kind_schema<NodeKind::InlineAsm, NodeFamily::Semantic, false,
                       OperandArityPolicy::Variable, 0, variable_arity,
                       ResultArityPolicy::Many, NodeEffect::Unknown,
                       all_node_stages, InlineAsmNode> {};
template <>
struct node_kind_traits<NodeKind::Store>
    : node_kind_schema<NodeKind::Store, NodeFamily::Memory, false,
                       OperandArityPolicy::Fixed, 1, 1,
                       ResultArityPolicy::Zero, NodeEffect::WritesMemory,
                       all_node_stages, StoreNode, LocalStoreAuthorityNode> {};
template <>
struct node_kind_traits<NodeKind::Load>
    : node_kind_schema<NodeKind::Load, NodeFamily::Memory, false,
                       OperandArityPolicy::Variable, 0, 1,
                       ResultArityPolicy::One, NodeEffect::ReadsMemory,
                       all_node_stages, LoadNode, LocalLoadAuthorityNode> {};
template <>
struct node_kind_traits<NodeKind::GetElementPtr>
    : node_kind_schema<NodeKind::GetElementPtr, NodeFamily::Semantic, false,
                       OperandArityPolicy::Variable, 1, variable_arity,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, GetElementPtrNode,
                       LocalArrayGepAuthorityNode> {};
template <>
struct node_kind_traits<NodeKind::Abs>
    : node_kind_schema<NodeKind::Abs, NodeFamily::Semantic, false,
                       OperandArityPolicy::Fixed, 1, 1,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, AbsNode> {};
template <>
struct node_kind_traits<NodeKind::Call>
    : node_kind_schema<NodeKind::Call, NodeFamily::Call, false,
                       OperandArityPolicy::Variable, 0, variable_arity,
                       ResultArityPolicy::Many, NodeEffect::Unknown,
                       all_node_stages, CallNode, IntrinsicCallNode> {};
template <>
struct node_kind_traits<NodeKind::Binary>
    : node_kind_schema<NodeKind::Binary, NodeFamily::Semantic, true,
                       OperandArityPolicy::Fixed, 2, 2,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, BinaryNode> {};
template <>
struct node_kind_traits<NodeKind::Compare>
    : node_kind_schema<NodeKind::Compare, NodeFamily::Semantic, true,
                       OperandArityPolicy::Fixed, 2, 2,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, CompareNode> {};
template <>
struct node_kind_traits<NodeKind::Select>
    : node_kind_schema<NodeKind::Select, NodeFamily::Semantic, false,
                       OperandArityPolicy::Variable, 0, variable_arity,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, SelectNode> {};
template <>
struct node_kind_traits<NodeKind::SelectedMemcpy>
    : node_kind_schema<NodeKind::SelectedMemcpy, NodeFamily::Memory, false,
                       OperandArityPolicy::Fixed, 0, 0,
                       ResultArityPolicy::Zero,
                       NodeEffect::ReadsAndWritesMemory, all_node_stages,
                       SelectedMemcpyNode> {};
template <>
struct node_kind_traits<NodeKind::Amd64SysVOverflowAggregateMemcpy>
    : node_kind_schema<NodeKind::Amd64SysVOverflowAggregateMemcpy,
                       NodeFamily::Memory, false, OperandArityPolicy::Fixed, 0,
                       0, ResultArityPolicy::Zero,
                       NodeEffect::ReadsAndWritesMemory, all_node_stages,
                       Amd64SysVOverflowAggregateMemcpyNode> {};
template <>
struct node_kind_traits<NodeKind::Cast>
    : node_kind_schema<NodeKind::Cast, NodeFamily::Semantic, false,
                       OperandArityPolicy::Fixed, 1, 1,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, CastNode> {};
template <>
struct node_kind_traits<NodeKind::Phi>
    : node_kind_schema<NodeKind::Phi, NodeFamily::Semantic, false,
                       OperandArityPolicy::Variable, 1, variable_arity,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, PhiNode> {};
template <>
struct node_kind_traits<NodeKind::AllocaAuthority>
    : node_kind_schema<NodeKind::AllocaAuthority, NodeFamily::Authority, false,
                       OperandArityPolicy::Fixed, 0, 0,
                       ResultArityPolicy::One, NodeEffect::None,
                       all_node_stages, AllocaAuthorityNode> {};
template <>
struct node_kind_traits<NodeKind::StackSaveAuthority>
    : node_kind_schema<NodeKind::StackSaveAuthority, NodeFamily::Authority,
                       false, OperandArityPolicy::Fixed, 0, 0,
                       ResultArityPolicy::One, NodeEffect::Unknown,
                       all_node_stages, StackSaveAuthorityNode> {};
template <>
struct node_kind_traits<NodeKind::StackRestoreAuthority>
    : node_kind_schema<NodeKind::StackRestoreAuthority, NodeFamily::Authority,
                       false, OperandArityPolicy::Fixed, 1, 1,
                       ResultArityPolicy::Zero, NodeEffect::Unknown,
                       all_node_stages, StackRestoreAuthorityNode> {};

template <NodeKind Kind>
constexpr NodeKindDescriptor node_kind_descriptor_v =
    node_kind_traits<Kind>::descriptor;

template <NodeKind Kind>
bool payload_accepted_by(const InstPayload& payload) noexcept {
  return node_kind_traits<Kind>::accepts(payload);
}

}  // namespace detail

template <NodeKind Kind>
constexpr NodeKindDescriptor node_kind_descriptor() noexcept {
  return detail::node_kind_descriptor_v<Kind>;
}

template <NodeKind Kind>
constexpr bool is_semantic_node_kind_v =
    node_kind_descriptor<Kind>().family == NodeFamily::Semantic;

template <NodeKind Kind>
constexpr bool is_binary_node_kind_v = node_kind_descriptor<Kind>().is_binary;

inline std::optional<NodeKindDescriptor> node_kind_descriptor(
    NodeKind kind) noexcept {
#define C4C_BIR_KIND_CASE(name)                                               \
  case NodeKind::name: return node_kind_descriptor<NodeKind::name>()
  switch (kind) {
    C4C_BIR_KIND_CASE(InlineAsm);
    C4C_BIR_KIND_CASE(Store);
    C4C_BIR_KIND_CASE(Load);
    C4C_BIR_KIND_CASE(GetElementPtr);
    C4C_BIR_KIND_CASE(Abs);
    C4C_BIR_KIND_CASE(Call);
    C4C_BIR_KIND_CASE(Binary);
    C4C_BIR_KIND_CASE(Compare);
    C4C_BIR_KIND_CASE(Select);
    C4C_BIR_KIND_CASE(SelectedMemcpy);
    C4C_BIR_KIND_CASE(Amd64SysVOverflowAggregateMemcpy);
    C4C_BIR_KIND_CASE(Cast);
    C4C_BIR_KIND_CASE(Phi);
    C4C_BIR_KIND_CASE(AllocaAuthority);
    C4C_BIR_KIND_CASE(StackSaveAuthority);
    C4C_BIR_KIND_CASE(StackRestoreAuthority);
  }
#undef C4C_BIR_KIND_CASE
  return std::nullopt;
}

inline bool is_semantic_node_kind(NodeKind kind) noexcept {
  const auto descriptor = node_kind_descriptor(kind);
  return descriptor && descriptor->family == NodeFamily::Semantic;
}

inline bool is_binary_node_kind(NodeKind kind) noexcept {
  const auto descriptor = node_kind_descriptor(kind);
  return descriptor && descriptor->is_binary;
}

inline bool node_kind_legal_in(NodeKind kind, NodeStage stage) noexcept {
  const auto descriptor = node_kind_descriptor(kind);
  return descriptor &&
         (descriptor->legal_stages & static_cast<std::uint8_t>(stage)) != 0;
}

inline bool node_kind_accepts_payload(NodeKind kind,
                                      const InstPayload& payload) noexcept {
#define C4C_BIR_PAYLOAD_CASE(name)                                            \
  case NodeKind::name:                                                       \
    return detail::payload_accepted_by<NodeKind::name>(payload)
  switch (kind) {
    C4C_BIR_PAYLOAD_CASE(InlineAsm);
    C4C_BIR_PAYLOAD_CASE(Store);
    C4C_BIR_PAYLOAD_CASE(Load);
    C4C_BIR_PAYLOAD_CASE(GetElementPtr);
    C4C_BIR_PAYLOAD_CASE(Abs);
    C4C_BIR_PAYLOAD_CASE(Call);
    C4C_BIR_PAYLOAD_CASE(Binary);
    C4C_BIR_PAYLOAD_CASE(Compare);
    C4C_BIR_PAYLOAD_CASE(Select);
    C4C_BIR_PAYLOAD_CASE(SelectedMemcpy);
    C4C_BIR_PAYLOAD_CASE(Amd64SysVOverflowAggregateMemcpy);
    C4C_BIR_PAYLOAD_CASE(Cast);
    C4C_BIR_PAYLOAD_CASE(Phi);
    C4C_BIR_PAYLOAD_CASE(AllocaAuthority);
    C4C_BIR_PAYLOAD_CASE(StackSaveAuthority);
    C4C_BIR_PAYLOAD_CASE(StackRestoreAuthority);
  }
#undef C4C_BIR_PAYLOAD_CASE
  return false;
}

inline bool node_kind_accepts_arity(NodeKind kind, std::size_t operand_count,
                                    std::size_t result_count) noexcept {
  const auto descriptor = node_kind_descriptor(kind);
  if (!descriptor) return false;

  const bool operands_match =
      operand_count >= descriptor->minimum_operands &&
      (descriptor->maximum_operands == detail::variable_arity ||
       operand_count <= descriptor->maximum_operands) &&
      (descriptor->operand_arity == OperandArityPolicy::Variable ||
       descriptor->minimum_operands == descriptor->maximum_operands);
  const bool results_match =
      descriptor->result_arity == ResultArityPolicy::Many ||
      (descriptor->result_arity == ResultArityPolicy::Zero &&
       result_count == 0) ||
      (descriptor->result_arity == ResultArityPolicy::One &&
       result_count == 1);
  return operands_match && results_match;
}

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
  // InstId is owned by FunctionData::insts_; it is deliberately not repeated
  // in this arena payload. Operands are ordered input uses. Results are
  // bootstrap compatibility storage paired with InstResultDef::result_index;
  // their concrete types remain authoritative in ValueDef.
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
