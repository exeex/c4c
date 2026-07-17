#pragma once

#include "ids.hpp"
#include "storage.hpp"
#include "type.hpp"
#include "../pipeline/identity.hpp"

#include <array>
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
  Unary,
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
  VaStartAuthority,
};

// The closed vocabulary size is derived from its last reviewed kind rather
// than exposed as an enum value that could be mistaken for a valid node kind.
inline constexpr std::size_t node_kind_count =
    static_cast<std::size_t>(NodeKind::VaStartAuthority) + 1;

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

enum class DirectZeroArgScalarFloatingCallRole : std::uint8_t {
  Invalid,
  ResultIntoFloatingBinaryLhs,
};

// Receipt of one producer-authorized direct zero-argument scalar floating call
// result consumed as the LHS of a downstream floating binary operation.
struct DirectZeroArgScalarFloatingCallResult {
  std::uint32_t source_result_id = 0;
  LinkNameId owner{};
  LinkNameId callee{};
  Type return_type{TypeKind::Void};
  DirectZeroArgScalarFloatingCallRole role =
      DirectZeroArgScalarFloatingCallRole::Invalid;
};

enum class DirectOneDoubleArgScalarFloatingCallRole : std::uint8_t {
  Invalid,
  DirectCallResult,
};

// Receipt of one producer-authorized direct nonvariadic double(double) call
// result.  This preserves the native result/owner/callee/type tuple and does
// not imply any selected downstream consumer.
struct DirectOneDoubleArgScalarFloatingCallResult {
  std::uint32_t source_result_id = 0;
  LinkNameId owner{};
  LinkNameId callee{};
  Type return_type{TypeKind::Void};
  Type argument_type{TypeKind::Void};
  DirectOneDoubleArgScalarFloatingCallRole role =
      DirectOneDoubleArgScalarFloatingCallRole::Invalid;
};

struct CallNode {
  FunctionId callee{};
  std::optional<DirectScalarBodyParameterFixedDirectCallArgument>
      direct_scalar_argument;
  std::optional<DirectZeroArgScalarFloatingCallResult>
      direct_zero_arg_scalar_floating_result;
  std::optional<DirectOneDoubleArgScalarFloatingCallResult>
      direct_one_double_arg_scalar_floating_result;
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

// Receipt of the producer-selected direct-local va_start destination.  The
// va_list pointer is retained as source identity plus exact local authority.
struct VaStartAuthorityNode {
  SourceValueId ap{};
  SourceValueId pointer_definition{};
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
    StackSaveAuthorityNode, StackRestoreAuthorityNode, VaStartAuthorityNode>;

enum class NodeFamily : std::uint8_t {
  Semantic,
  Memory,
  Call,
  Authority,
  Count,
};
enum class OperandArityPolicy : std::uint8_t { Fixed, Variable, Count };
enum class ResultArityPolicy : std::uint8_t { Zero, One, Many, Count };
enum class NodeEffect : std::uint8_t {
  None,
  ReadsMemory,
  WritesMemory,
  ReadsAndWritesMemory,
  Unknown,
  Count,
};
enum class NodeStage : std::uint8_t {
  Raw = 1,
  Canonical = 2,
  Prepared = 4,
  PseudoPreallocation = 8,
  Allocated = 16,
  MirReadyMachine = 32,
};

enum class NodeValueModel : std::uint8_t {
  NoOrdinaryResult,
  SingleOrdinaryResult,
  MultipleResults,
  Count,
};
enum class NodeSsaParticipation : std::uint8_t { NeverSsa, SsaEligible, Count };
enum class NodeSemanticFamily : std::uint8_t {
  Arithmetic,
  Compare,
  Conversion,
  Memory,
  Aggregate,
  Call,
  PhiMerge,
  Authority,
  Intrinsic,
  Preparation,
  Pseudo,
  AllocationAction,
  Machine,
  Count,
};
enum class NodeControlBehavior : std::uint8_t {
  FallsThrough,
  TerminatorNoSuccessor,
  TerminatorOneSuccessor,
  TerminatorTwoSuccessors,
  TerminatorVariableSuccessors,
  Count,
};
enum class NodeTrapBehavior : std::uint8_t { CannotTrap, MayTrap, Count };
enum class NodeTypePolicy : std::uint8_t {
  NoResultType,
  StoredValueType,
  Count,
};
enum class NodePayloadPolicy : std::uint8_t {
  OneClosedPayload,
  ClosedPayloadAlternatives,
  Count,
};
enum class NodeMirDisposition : std::uint8_t {
  OneRecordRealizable,
  RequiresExpansion,
  RequiresAllocationOrFrameFacts,
  MachineOnly,
  ForbiddenAtMirBoundary,
  Count,
};
enum class NodeKindRefinement : std::uint8_t {
  None = 0,
  BinaryForm = 1,
  PhiMergeForm = 2,
  UnaryForm = 4,
};
enum class NodeTag : std::uint8_t {
  ValueProducing,
  SsaEligible,
  Semantic,
  Memory,
  CallLike,
  Terminator,
  ReadsMemory,
  WritesMemory,
  MayTrap,
  RequiresExpansion,
  RequiresAllocationOrFrameFacts,
  MachineOnly,
  Count,
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

struct NodeKindSchema {
  NodeKind kind;
  NodeKindDescriptor descriptor;
  NodeValueModel value_model;
  NodeSsaParticipation ssa_participation;
  NodeSemanticFamily semantic_family;
  std::uint8_t refinements;
  NodeControlBehavior control;
  NodeTrapBehavior trap;
  NodeStage stage_owner;
  NodeTypePolicy type_policy;
  NodePayloadPolicy payload_policy;
  NodeMirDisposition mir_disposition;
};

namespace detail {

constexpr std::uint16_t unbounded_arity = UINT16_MAX;
constexpr std::uint8_t semantic_node_stages =
    static_cast<std::uint8_t>(NodeStage::Raw) |
    static_cast<std::uint8_t>(NodeStage::Canonical) |
    static_cast<std::uint8_t>(NodeStage::Prepared);
constexpr std::uint8_t known_node_stages =
    semantic_node_stages |
    static_cast<std::uint8_t>(NodeStage::PseudoPreallocation) |
    static_cast<std::uint8_t>(NodeStage::Allocated) |
    static_cast<std::uint8_t>(NodeStage::MirReadyMachine);
constexpr std::uint8_t binary_refinement =
    static_cast<std::uint8_t>(NodeKindRefinement::BinaryForm);
constexpr std::uint8_t phi_refinement =
    static_cast<std::uint8_t>(NodeKindRefinement::PhiMergeForm);
constexpr std::uint8_t unary_refinement =
    static_cast<std::uint8_t>(NodeKindRefinement::UnaryForm);
constexpr std::uint8_t known_refinements =
    binary_refinement | phi_refinement | unary_refinement;

struct OperandAritySpec {
  OperandArityPolicy policy;
  std::uint16_t minimum;
  std::uint16_t maximum;
};

struct StageSet {
  std::uint8_t bits;
};

struct RefinementSet {
  std::uint8_t bits;
};

consteval OperandAritySpec fixed_arity(std::uint16_t count) noexcept {
  return {OperandArityPolicy::Fixed, count, count};
}

consteval OperandAritySpec
variable_arity(std::uint16_t minimum = 0,
               std::uint16_t maximum = unbounded_arity) noexcept {
  return {OperandArityPolicy::Variable, minimum, maximum};
}

template <NodeStage... Stages> consteval StageSet stages() noexcept {
  return {(std::uint8_t{0} | ... | static_cast<std::uint8_t>(Stages))};
}

template <NodeKindRefinement... Refinements>
consteval RefinementSet refinements() noexcept {
  return {(std::uint8_t{0} | ... | static_cast<std::uint8_t>(Refinements))};
}

inline constexpr auto semantic_stages =
    stages<NodeStage::Raw, NodeStage::Canonical, NodeStage::Prepared>();

struct NodeKindSpec {
  NodeKind kind;
  NodeFamily legacy_family;
  NodeValueModel value_model;
  NodeSsaParticipation ssa;
  NodeSemanticFamily semantic_family;
  RefinementSet refinements;
  OperandAritySpec operands;
  ResultArityPolicy results;
  NodeEffect effects;
  NodeControlBehavior control;
  NodeTrapBehavior trap;
  NodeStage stage_owner;
  StageSet admitted_stages;
  NodeTypePolicy type_policy;
  NodeMirDisposition mir;
};

template <class... Payloads>
bool accepts_one_of(const InstPayload& payload) noexcept {
  return (std::holds_alternative<Payloads>(payload) || ...);
}

using PayloadAcceptance = bool (*)(const InstPayload&) noexcept;

struct NodeKindRegistryEntry {
  NodeKindSchema schema;
  PayloadAcceptance accepts_payload;
};

constexpr bool valid_authoring_spec(const NodeKindSpec &spec) noexcept {
  const bool enums_valid =
      static_cast<std::size_t>(spec.kind) <
          node_kind_count &&
      static_cast<std::uint8_t>(spec.legacy_family) <
          static_cast<std::uint8_t>(NodeFamily::Count) &&
      static_cast<std::uint8_t>(spec.value_model) <
          static_cast<std::uint8_t>(NodeValueModel::Count) &&
      static_cast<std::uint8_t>(spec.ssa) <
          static_cast<std::uint8_t>(NodeSsaParticipation::Count) &&
      static_cast<std::uint8_t>(spec.semantic_family) <
          static_cast<std::uint8_t>(NodeSemanticFamily::Count) &&
      static_cast<std::uint8_t>(spec.operands.policy) <
          static_cast<std::uint8_t>(OperandArityPolicy::Count) &&
      static_cast<std::uint8_t>(spec.results) <
          static_cast<std::uint8_t>(ResultArityPolicy::Count) &&
      static_cast<std::uint8_t>(spec.effects) <
          static_cast<std::uint8_t>(NodeEffect::Count) &&
      static_cast<std::uint8_t>(spec.control) <
          static_cast<std::uint8_t>(NodeControlBehavior::Count) &&
      static_cast<std::uint8_t>(spec.trap) <
          static_cast<std::uint8_t>(NodeTrapBehavior::Count) &&
      static_cast<std::uint8_t>(spec.type_policy) <
          static_cast<std::uint8_t>(NodeTypePolicy::Count) &&
      static_cast<std::uint8_t>(spec.mir) <
          static_cast<std::uint8_t>(NodeMirDisposition::Count);
  const bool arity_valid =
      (spec.operands.policy == OperandArityPolicy::Fixed &&
       spec.operands.minimum == spec.operands.maximum) ||
      (spec.operands.policy == OperandArityPolicy::Variable &&
       (spec.operands.maximum == unbounded_arity ||
        spec.operands.minimum <= spec.operands.maximum));
  const auto stage_owner = static_cast<std::uint8_t>(spec.stage_owner);
  const bool stage_owner_valid =
      stage_owner != 0 && (stage_owner & (stage_owner - 1)) == 0 &&
      (stage_owner & known_node_stages) != 0;
  const bool stages_valid =
      spec.admitted_stages.bits != 0 &&
      (spec.admitted_stages.bits & ~known_node_stages) == 0 &&
      (spec.admitted_stages.bits & stage_owner) != 0;
  const bool refinements_valid =
      (spec.refinements.bits & ~known_refinements) == 0 &&
      ((spec.refinements.bits & binary_refinement) == 0 ||
       (spec.operands.policy == OperandArityPolicy::Fixed &&
        spec.operands.minimum == 2)) &&
      ((spec.refinements.bits & unary_refinement) == 0 ||
       (spec.operands.policy == OperandArityPolicy::Fixed &&
        spec.operands.minimum == 1)) &&
      (spec.refinements.bits & (binary_refinement | unary_refinement)) !=
          (binary_refinement | unary_refinement) &&
      ((spec.refinements.bits & phi_refinement) == 0 ||
       spec.semantic_family == NodeSemanticFamily::PhiMerge);
  const bool result_valid =
      (spec.value_model == NodeValueModel::NoOrdinaryResult &&
       spec.results == ResultArityPolicy::Zero &&
       spec.type_policy == NodeTypePolicy::NoResultType &&
       spec.ssa == NodeSsaParticipation::NeverSsa) ||
      (spec.value_model == NodeValueModel::SingleOrdinaryResult &&
       spec.results == ResultArityPolicy::One &&
       spec.type_policy != NodeTypePolicy::NoResultType) ||
      (spec.value_model == NodeValueModel::MultipleResults &&
       spec.results == ResultArityPolicy::Many &&
       spec.type_policy != NodeTypePolicy::NoResultType);
  const bool mir_valid = spec.mir != NodeMirDisposition::MachineOnly ||
                         (spec.semantic_family == NodeSemanticFamily::Machine &&
                          spec.stage_owner == NodeStage::MirReadyMachine);
  const bool family_stage_valid =
      (spec.semantic_family != NodeSemanticFamily::Preparation ||
       spec.stage_owner == NodeStage::Prepared) &&
      (spec.semantic_family != NodeSemanticFamily::Pseudo ||
       spec.stage_owner == NodeStage::PseudoPreallocation) &&
      (spec.semantic_family != NodeSemanticFamily::AllocationAction ||
       spec.stage_owner == NodeStage::Allocated) &&
      (spec.semantic_family != NodeSemanticFamily::Machine ||
       spec.stage_owner == NodeStage::MirReadyMachine);
  return enums_valid && arity_valid && stage_owner_valid && stages_valid &&
         refinements_valid && result_valid && mir_valid && family_stage_valid;
}

template <class... Payloads>
consteval NodeKindRegistryEntry make_node_kind_entry(NodeKindSpec spec) {
  if (!valid_authoring_spec(spec))
    throw "invalid NodeKind authoring specification";
  return {
      {spec.kind,
       {spec.legacy_family, (spec.refinements.bits & binary_refinement) != 0,
        spec.operands.policy, spec.operands.minimum, spec.operands.maximum,
        spec.results, spec.effects, spec.admitted_stages.bits},
       spec.value_model,
       spec.ssa,
       spec.semantic_family,
       spec.refinements.bits,
       spec.control,
       spec.trap,
       spec.stage_owner,
       spec.type_policy,
       sizeof...(Payloads) == 1 ? NodePayloadPolicy::OneClosedPayload
                                : NodePayloadPolicy::ClosedPayloadAlternatives,
       spec.mir},
      &accepts_one_of<Payloads...>};
}

inline constexpr std::array<NodeKindRegistryEntry,
                            node_kind_count>
    node_kind_registry{{
        make_node_kind_entry<InlineAsmNode>(
            {.kind = NodeKind::InlineAsm,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::MultipleResults,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Intrinsic,
             .refinements = refinements<>(),
             .operands = variable_arity(),
             .results = ResultArityPolicy::Many,
             .effects = NodeEffect::Unknown,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::RequiresExpansion}),
        make_node_kind_entry<StoreNode, LocalStoreAuthorityNode>(
            {.kind = NodeKind::Store,
             .legacy_family = NodeFamily::Memory,
             .value_model = NodeValueModel::NoOrdinaryResult,
             .ssa = NodeSsaParticipation::NeverSsa,
             .semantic_family = NodeSemanticFamily::Memory,
             .refinements = refinements<>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::Zero,
             .effects = NodeEffect::WritesMemory,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::NoResultType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<LoadNode, LocalLoadAuthorityNode>(
            {.kind = NodeKind::Load,
             .legacy_family = NodeFamily::Memory,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Memory,
             .refinements = refinements<>(),
             .operands = variable_arity(0, 1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::ReadsMemory,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<GetElementPtrNode, LocalArrayGepAuthorityNode>(
            {.kind = NodeKind::GetElementPtr,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Aggregate,
             .refinements = refinements<>(),
             .operands = variable_arity(1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<AbsNode>(
            {.kind = NodeKind::Abs,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Arithmetic,
             .refinements = refinements<>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<CallNode, IntrinsicCallNode>(
            {.kind = NodeKind::Call,
             .legacy_family = NodeFamily::Call,
             .value_model = NodeValueModel::MultipleResults,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Call,
             .refinements = refinements<>(),
             .operands = variable_arity(),
             .results = ResultArityPolicy::Many,
             .effects = NodeEffect::Unknown,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::RequiresAllocationOrFrameFacts}),
        make_node_kind_entry<BinaryNode>(
            {.kind = NodeKind::Unary,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Arithmetic,
             .refinements = refinements<NodeKindRefinement::UnaryForm>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<BinaryNode>(
            {.kind = NodeKind::Binary,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Arithmetic,
             .refinements = refinements<NodeKindRefinement::BinaryForm>(),
             .operands = fixed_arity(2),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<CompareNode>(
            {.kind = NodeKind::Compare,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Compare,
             .refinements = refinements<NodeKindRefinement::BinaryForm>(),
             .operands = fixed_arity(2),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<SelectNode>(
            {.kind = NodeKind::Select,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Compare,
             .refinements = refinements<>(),
             .operands = variable_arity(),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<SelectedMemcpyNode>(
            {.kind = NodeKind::SelectedMemcpy,
             .legacy_family = NodeFamily::Memory,
             .value_model = NodeValueModel::NoOrdinaryResult,
             .ssa = NodeSsaParticipation::NeverSsa,
             .semantic_family = NodeSemanticFamily::Memory,
             .refinements = refinements<>(),
             .operands = fixed_arity(0),
             .results = ResultArityPolicy::Zero,
             .effects = NodeEffect::ReadsAndWritesMemory,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::NoResultType,
             .mir = NodeMirDisposition::RequiresExpansion}),
        make_node_kind_entry<Amd64SysVOverflowAggregateMemcpyNode>(
            {.kind = NodeKind::Amd64SysVOverflowAggregateMemcpy,
             .legacy_family = NodeFamily::Memory,
             .value_model = NodeValueModel::NoOrdinaryResult,
             .ssa = NodeSsaParticipation::NeverSsa,
             .semantic_family = NodeSemanticFamily::Memory,
             .refinements = refinements<>(),
             .operands = fixed_arity(0),
             .results = ResultArityPolicy::Zero,
             .effects = NodeEffect::ReadsAndWritesMemory,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::NoResultType,
             .mir = NodeMirDisposition::RequiresExpansion}),
        make_node_kind_entry<CastNode>(
            {.kind = NodeKind::Cast,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Conversion,
             .refinements = refinements<>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::OneRecordRealizable}),
        make_node_kind_entry<PhiNode>(
            {.kind = NodeKind::Phi,
             .legacy_family = NodeFamily::Semantic,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::PhiMerge,
             .refinements = refinements<NodeKindRefinement::PhiMergeForm>(),
             .operands = variable_arity(1),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::ForbiddenAtMirBoundary}),
        make_node_kind_entry<AllocaAuthorityNode>(
            {.kind = NodeKind::AllocaAuthority,
             .legacy_family = NodeFamily::Authority,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Authority,
             .refinements = refinements<>(),
             .operands = fixed_arity(0),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::None,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::CannotTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::RequiresAllocationOrFrameFacts}),
        make_node_kind_entry<StackSaveAuthorityNode>(
            {.kind = NodeKind::StackSaveAuthority,
             .legacy_family = NodeFamily::Authority,
             .value_model = NodeValueModel::SingleOrdinaryResult,
             .ssa = NodeSsaParticipation::SsaEligible,
             .semantic_family = NodeSemanticFamily::Authority,
             .refinements = refinements<>(),
             .operands = fixed_arity(0),
             .results = ResultArityPolicy::One,
             .effects = NodeEffect::Unknown,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::StoredValueType,
             .mir = NodeMirDisposition::RequiresAllocationOrFrameFacts}),
        make_node_kind_entry<StackRestoreAuthorityNode>(
            {.kind = NodeKind::StackRestoreAuthority,
             .legacy_family = NodeFamily::Authority,
             .value_model = NodeValueModel::NoOrdinaryResult,
             .ssa = NodeSsaParticipation::NeverSsa,
             .semantic_family = NodeSemanticFamily::Authority,
             .refinements = refinements<>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::Zero,
             .effects = NodeEffect::Unknown,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::NoResultType,
             .mir = NodeMirDisposition::RequiresAllocationOrFrameFacts}),
        make_node_kind_entry<VaStartAuthorityNode>(
            {.kind = NodeKind::VaStartAuthority,
             .legacy_family = NodeFamily::Authority,
             .value_model = NodeValueModel::NoOrdinaryResult,
             .ssa = NodeSsaParticipation::NeverSsa,
             .semantic_family = NodeSemanticFamily::Authority,
             .refinements = refinements<>(),
             .operands = fixed_arity(1),
             .results = ResultArityPolicy::Zero,
             .effects = NodeEffect::WritesMemory,
             .control = NodeControlBehavior::FallsThrough,
             .trap = NodeTrapBehavior::MayTrap,
             .stage_owner = NodeStage::Raw,
             .admitted_stages = semantic_stages,
             .type_policy = NodeTypePolicy::NoResultType,
             .mir = NodeMirDisposition::RequiresAllocationOrFrameFacts}),
    }};

constexpr bool valid_stage(NodeStage stage) noexcept {
  const auto value = static_cast<std::uint8_t>(stage);
  return value != 0 && (value & (value - 1)) == 0 &&
         (value & known_node_stages) != 0;
}

constexpr bool validate_node_kind_schema(const NodeKindSchema& schema) noexcept {
  const auto& descriptor = schema.descriptor;
  const bool enums_valid =
      static_cast<std::uint8_t>(descriptor.family) <
          static_cast<std::uint8_t>(NodeFamily::Count) &&
      static_cast<std::uint8_t>(descriptor.operand_arity) <
          static_cast<std::uint8_t>(OperandArityPolicy::Count) &&
      static_cast<std::uint8_t>(descriptor.result_arity) <
          static_cast<std::uint8_t>(ResultArityPolicy::Count) &&
      static_cast<std::uint8_t>(descriptor.effects) <
          static_cast<std::uint8_t>(NodeEffect::Count) &&
      static_cast<std::uint8_t>(schema.value_model) <
          static_cast<std::uint8_t>(NodeValueModel::Count) &&
      static_cast<std::uint8_t>(schema.ssa_participation) <
          static_cast<std::uint8_t>(NodeSsaParticipation::Count) &&
      static_cast<std::uint8_t>(schema.semantic_family) <
          static_cast<std::uint8_t>(NodeSemanticFamily::Count) &&
      static_cast<std::uint8_t>(schema.control) <
          static_cast<std::uint8_t>(NodeControlBehavior::Count) &&
      static_cast<std::uint8_t>(schema.trap) <
          static_cast<std::uint8_t>(NodeTrapBehavior::Count) &&
      static_cast<std::uint8_t>(schema.type_policy) <
          static_cast<std::uint8_t>(NodeTypePolicy::Count) &&
      static_cast<std::uint8_t>(schema.payload_policy) <
          static_cast<std::uint8_t>(NodePayloadPolicy::Count) &&
      static_cast<std::uint8_t>(schema.mir_disposition) <
          static_cast<std::uint8_t>(NodeMirDisposition::Count);
  const bool fixed_arity_valid =
      descriptor.operand_arity != OperandArityPolicy::Fixed ||
      descriptor.minimum_operands == descriptor.maximum_operands;
  const bool bounds_valid =
      descriptor.maximum_operands == unbounded_arity ||
      descriptor.minimum_operands <= descriptor.maximum_operands;
  const bool result_valid =
      (schema.value_model == NodeValueModel::NoOrdinaryResult &&
       descriptor.result_arity == ResultArityPolicy::Zero &&
       schema.type_policy == NodeTypePolicy::NoResultType &&
       schema.ssa_participation == NodeSsaParticipation::NeverSsa) ||
      (schema.value_model == NodeValueModel::SingleOrdinaryResult &&
       descriptor.result_arity == ResultArityPolicy::One &&
       schema.type_policy != NodeTypePolicy::NoResultType) ||
      (schema.value_model == NodeValueModel::MultipleResults &&
       descriptor.result_arity == ResultArityPolicy::Many &&
       schema.type_policy != NodeTypePolicy::NoResultType);
  const bool refinements_valid =
      (schema.refinements & ~known_refinements) == 0 &&
      ((schema.refinements & binary_refinement) == 0 ||
       (descriptor.operand_arity == OperandArityPolicy::Fixed &&
        descriptor.minimum_operands == 2)) &&
      ((schema.refinements & phi_refinement) == 0 ||
       schema.semantic_family == NodeSemanticFamily::PhiMerge);
  const auto owner = static_cast<std::uint8_t>(schema.stage_owner);
  const bool stages_valid = valid_stage(schema.stage_owner) &&
                            descriptor.legal_stages != 0 &&
                            (descriptor.legal_stages & ~known_node_stages) == 0 &&
                            (descriptor.legal_stages & owner) != 0;
  const bool mir_valid =
      schema.mir_disposition != NodeMirDisposition::MachineOnly ||
      (schema.semantic_family == NodeSemanticFamily::Machine &&
       schema.stage_owner == NodeStage::MirReadyMachine);
  const bool family_stage_valid =
      (schema.semantic_family != NodeSemanticFamily::Preparation ||
       schema.stage_owner == NodeStage::Prepared) &&
      (schema.semantic_family != NodeSemanticFamily::Pseudo ||
       schema.stage_owner == NodeStage::PseudoPreallocation) &&
      (schema.semantic_family != NodeSemanticFamily::AllocationAction ||
       schema.stage_owner == NodeStage::Allocated) &&
      (schema.semantic_family != NodeSemanticFamily::Machine ||
       schema.stage_owner == NodeStage::MirReadyMachine);
  return enums_valid && fixed_arity_valid && bounds_valid && result_valid &&
         refinements_valid && stages_valid && mir_valid && family_stage_valid;
}

constexpr bool validate_node_kind_registry() noexcept {
  if (node_kind_registry.size() != node_kind_count)
    return false;
  for (std::size_t i = 0; i < node_kind_registry.size(); ++i) {
    const auto& entry = node_kind_registry[i];
    if (static_cast<std::size_t>(entry.schema.kind) != i ||
        entry.accepts_payload == nullptr ||
        !validate_node_kind_schema(entry.schema))
      return false;
  }
  return true;
}

static_assert(validate_node_kind_registry(),
              "NodeKind registry must be complete, ordered, and valid");

template <NodeKind Kind>
constexpr const NodeKindRegistryEntry& node_kind_entry() noexcept {
  static_assert(static_cast<std::size_t>(Kind) <
                    node_kind_count,
                "unknown NodeKind is not queryable");
  return node_kind_registry[static_cast<std::size_t>(Kind)];
}

inline const NodeKindRegistryEntry* node_kind_entry(NodeKind kind) noexcept {
  const auto index = static_cast<std::size_t>(kind);
  if (index >= node_kind_registry.size()) return nullptr;
  return &node_kind_registry[index];
}

}  // namespace detail

template <NodeKind Kind>
constexpr NodeKindDescriptor node_kind_descriptor() noexcept {
  return detail::node_kind_entry<Kind>().schema.descriptor;
}

template <NodeKind Kind>
constexpr NodeKindSchema node_kind_schema() noexcept {
  return detail::node_kind_entry<Kind>().schema;
}

template <NodeKind Kind>
constexpr bool is_semantic_node_kind_v =
    node_kind_descriptor<Kind>().family == NodeFamily::Semantic;

template <NodeKind Kind>
constexpr bool is_binary_node_kind_v = node_kind_descriptor<Kind>().is_binary;

constexpr bool node_schema_has_tag(const NodeKindSchema& schema,
                                   NodeTag tag) noexcept {
  switch (tag) {
    case NodeTag::ValueProducing:
      return schema.value_model != NodeValueModel::NoOrdinaryResult;
    case NodeTag::SsaEligible:
      return schema.ssa_participation == NodeSsaParticipation::SsaEligible;
    case NodeTag::Semantic:
      return schema.descriptor.family == NodeFamily::Semantic;
    case NodeTag::Memory:
      return schema.semantic_family == NodeSemanticFamily::Memory;
    case NodeTag::CallLike:
      return schema.semantic_family == NodeSemanticFamily::Call;
    case NodeTag::Terminator:
      return schema.control != NodeControlBehavior::FallsThrough;
    case NodeTag::ReadsMemory:
      return schema.descriptor.effects == NodeEffect::ReadsMemory ||
             schema.descriptor.effects == NodeEffect::ReadsAndWritesMemory ||
             schema.descriptor.effects == NodeEffect::Unknown;
    case NodeTag::WritesMemory:
      return schema.descriptor.effects == NodeEffect::WritesMemory ||
             schema.descriptor.effects == NodeEffect::ReadsAndWritesMemory ||
             schema.descriptor.effects == NodeEffect::Unknown;
    case NodeTag::MayTrap: return schema.trap == NodeTrapBehavior::MayTrap;
    case NodeTag::RequiresExpansion:
      return schema.mir_disposition == NodeMirDisposition::RequiresExpansion;
    case NodeTag::RequiresAllocationOrFrameFacts:
      return schema.mir_disposition ==
             NodeMirDisposition::RequiresAllocationOrFrameFacts;
    case NodeTag::MachineOnly:
      return schema.mir_disposition == NodeMirDisposition::MachineOnly;
    case NodeTag::Count: return false;
  }
  return false;
}

template <NodeKind Kind, NodeTag Tag>
constexpr bool node_has_tag_v = node_schema_has_tag(node_kind_schema<Kind>(), Tag);

template <NodeKind Kind, NodeStage Stage>
constexpr bool node_kind_admitted_in_v =
    detail::valid_stage(Stage) &&
    (node_kind_schema<Kind>().descriptor.legal_stages &
     static_cast<std::uint8_t>(Stage)) != 0;

template <NodeKind Kind, NodeStage Stage>
constexpr bool is_ssa_eligible_in_v =
    node_kind_admitted_in_v<Kind, Stage> &&
    node_has_tag_v<Kind, NodeTag::SsaEligible>;

template <NodeKind Kind>
constexpr bool is_value_producing_v =
    node_has_tag_v<Kind, NodeTag::ValueProducing>;
template <NodeKind Kind>
constexpr bool is_memory_op_v = node_has_tag_v<Kind, NodeTag::Memory>;
template <NodeKind Kind>
constexpr bool is_call_like_v = node_has_tag_v<Kind, NodeTag::CallLike>;
template <NodeKind Kind>
constexpr bool is_terminator_v = node_has_tag_v<Kind, NodeTag::Terminator>;
template <NodeKind Kind>
constexpr bool may_read_memory_v = node_has_tag_v<Kind, NodeTag::ReadsMemory>;
template <NodeKind Kind>
constexpr bool may_write_memory_v = node_has_tag_v<Kind, NodeTag::WritesMemory>;
template <NodeKind Kind>
constexpr bool may_trap_v = node_has_tag_v<Kind, NodeTag::MayTrap>;
template <NodeKind Kind>
constexpr bool requires_expansion_v =
    node_has_tag_v<Kind, NodeTag::RequiresExpansion>;
template <NodeKind Kind>
constexpr bool requires_allocation_or_frame_v =
    node_has_tag_v<Kind, NodeTag::RequiresAllocationOrFrameFacts>;
template <NodeKind Kind>
constexpr bool is_machine_only_v = node_has_tag_v<Kind, NodeTag::MachineOnly>;

inline std::optional<NodeKindDescriptor> node_kind_descriptor(
    NodeKind kind) noexcept {
  const auto* entry = detail::node_kind_entry(kind);
  if (!entry) return std::nullopt;
  return entry->schema.descriptor;
}

inline std::optional<NodeKindSchema> node_kind_schema(NodeKind kind) noexcept {
  const auto* entry = detail::node_kind_entry(kind);
  if (!entry) return std::nullopt;
  return entry->schema;
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
  const auto schema = node_kind_schema(kind);
  return schema && detail::valid_stage(stage) &&
         (schema->descriptor.legal_stages & static_cast<std::uint8_t>(stage)) !=
             0;
}

inline bool node_has_tag(NodeKind kind, NodeTag tag) noexcept {
  const auto schema = node_kind_schema(kind);
  return schema && static_cast<std::uint8_t>(tag) <
                       static_cast<std::uint8_t>(NodeTag::Count) &&
         node_schema_has_tag(*schema, tag);
}

inline bool is_ssa_eligible_in(NodeKind kind, NodeStage stage) noexcept {
  return node_kind_legal_in(kind, stage) &&
         node_has_tag(kind, NodeTag::SsaEligible);
}

inline bool is_value_producing(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::ValueProducing);
}
inline bool is_memory_op(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::Memory);
}
inline bool is_call_like(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::CallLike);
}
inline bool is_terminator(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::Terminator);
}
inline bool may_read_memory(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::ReadsMemory);
}
inline bool may_write_memory(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::WritesMemory);
}
inline bool may_trap(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::MayTrap);
}
inline bool requires_expansion(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::RequiresExpansion);
}
inline bool requires_allocation_or_frame(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::RequiresAllocationOrFrameFacts);
}
inline bool is_machine_only(NodeKind kind) noexcept {
  return node_has_tag(kind, NodeTag::MachineOnly);
}

inline bool node_kind_accepts_payload(NodeKind kind,
                                      const InstPayload& payload) noexcept {
  const auto* entry = detail::node_kind_entry(kind);
  return entry && entry->accepts_payload(payload);
}

inline bool node_kind_accepts_arity(NodeKind kind, std::size_t operand_count,
                                    std::size_t result_count) noexcept {
  const auto descriptor = node_kind_descriptor(kind);
  if (!descriptor) return false;

  const bool operands_match =
      operand_count >= descriptor->minimum_operands &&
      (descriptor->maximum_operands == detail::unbounded_arity ||
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
