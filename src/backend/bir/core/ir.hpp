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
  Count,
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

constexpr std::uint16_t variable_arity = UINT16_MAX;
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
constexpr std::uint8_t known_refinements = binary_refinement | phi_refinement;

template <class... Payloads>
bool accepts_one_of(const InstPayload& payload) noexcept {
  return (std::holds_alternative<Payloads>(payload) || ...);
}

using PayloadAcceptance = bool (*)(const InstPayload&) noexcept;

struct NodeKindRegistryEntry {
  NodeKindSchema schema;
  PayloadAcceptance accepts_payload;
};

template <class... Payloads>
constexpr NodeKindRegistryEntry make_node_kind_entry(
    NodeKind kind, NodeFamily legacy_family, NodeValueModel value_model,
    NodeSsaParticipation ssa, NodeSemanticFamily semantic_family,
    std::uint8_t refinements, OperandArityPolicy operand_policy,
    std::uint16_t minimum_operands, std::uint16_t maximum_operands,
    ResultArityPolicy result_policy, NodeEffect effects,
    NodeControlBehavior control, NodeTrapBehavior trap, NodeStage stage_owner,
    std::uint8_t admitted_stages, NodeTypePolicy type_policy,
    NodeMirDisposition mir) noexcept {
  return {{kind,
           {legacy_family, (refinements & binary_refinement) != 0,
            operand_policy, minimum_operands, maximum_operands, result_policy,
            effects, admitted_stages},
           value_model,
           ssa,
           semantic_family,
           refinements,
           control,
           trap,
           stage_owner,
           type_policy,
           sizeof...(Payloads) == 1
               ? NodePayloadPolicy::OneClosedPayload
               : NodePayloadPolicy::ClosedPayloadAlternatives,
           mir},
          &accepts_one_of<Payloads...>};
}

inline constexpr std::array<NodeKindRegistryEntry,
                            static_cast<std::size_t>(NodeKind::Count)>
    node_kind_registry{{
        make_node_kind_entry<InlineAsmNode>(
            NodeKind::InlineAsm, NodeFamily::Semantic,
            NodeValueModel::MultipleResults, NodeSsaParticipation::SsaEligible,
            NodeSemanticFamily::Intrinsic, 0, OperandArityPolicy::Variable, 0,
            variable_arity, ResultArityPolicy::Many, NodeEffect::Unknown,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::MayTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::RequiresExpansion),
        make_node_kind_entry<StoreNode, LocalStoreAuthorityNode>(
            NodeKind::Store, NodeFamily::Memory,
            NodeValueModel::NoOrdinaryResult,
            NodeSsaParticipation::NeverSsa, NodeSemanticFamily::Memory, 0,
            OperandArityPolicy::Fixed, 1, 1, ResultArityPolicy::Zero,
            NodeEffect::WritesMemory, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::MayTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::NoResultType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<LoadNode, LocalLoadAuthorityNode>(
            NodeKind::Load, NodeFamily::Memory,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Memory, 0,
            OperandArityPolicy::Variable, 0, 1, ResultArityPolicy::One,
            NodeEffect::ReadsMemory, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::MayTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<GetElementPtrNode, LocalArrayGepAuthorityNode>(
            NodeKind::GetElementPtr, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Aggregate, 0,
            OperandArityPolicy::Variable, 1, variable_arity,
            ResultArityPolicy::One, NodeEffect::None,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::CannotTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<AbsNode>(
            NodeKind::Abs, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Arithmetic, 0,
            OperandArityPolicy::Fixed, 1, 1, ResultArityPolicy::One,
            NodeEffect::None, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::CannotTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<CallNode, IntrinsicCallNode>(
            NodeKind::Call, NodeFamily::Call, NodeValueModel::MultipleResults,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Call, 0,
            OperandArityPolicy::Variable, 0, variable_arity,
            ResultArityPolicy::Many, NodeEffect::Unknown,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::MayTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::RequiresAllocationOrFrameFacts),
        make_node_kind_entry<BinaryNode>(
            NodeKind::Binary, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Arithmetic,
            binary_refinement, OperandArityPolicy::Fixed, 2, 2,
            ResultArityPolicy::One, NodeEffect::None,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::MayTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<CompareNode>(
            NodeKind::Compare, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Compare,
            binary_refinement, OperandArityPolicy::Fixed, 2, 2,
            ResultArityPolicy::One, NodeEffect::None,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::CannotTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<SelectNode>(
            NodeKind::Select, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Compare, 0,
            OperandArityPolicy::Variable, 0, variable_arity,
            ResultArityPolicy::One, NodeEffect::None,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::CannotTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<SelectedMemcpyNode>(
            NodeKind::SelectedMemcpy, NodeFamily::Memory,
            NodeValueModel::NoOrdinaryResult,
            NodeSsaParticipation::NeverSsa, NodeSemanticFamily::Memory, 0,
            OperandArityPolicy::Fixed, 0, 0, ResultArityPolicy::Zero,
            NodeEffect::ReadsAndWritesMemory,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::MayTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::NoResultType,
            NodeMirDisposition::RequiresExpansion),
        make_node_kind_entry<Amd64SysVOverflowAggregateMemcpyNode>(
            NodeKind::Amd64SysVOverflowAggregateMemcpy, NodeFamily::Memory,
            NodeValueModel::NoOrdinaryResult,
            NodeSsaParticipation::NeverSsa, NodeSemanticFamily::Memory, 0,
            OperandArityPolicy::Fixed, 0, 0, ResultArityPolicy::Zero,
            NodeEffect::ReadsAndWritesMemory,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::MayTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::NoResultType,
            NodeMirDisposition::RequiresExpansion),
        make_node_kind_entry<CastNode>(
            NodeKind::Cast, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Conversion, 0,
            OperandArityPolicy::Fixed, 1, 1, ResultArityPolicy::One,
            NodeEffect::None, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::CannotTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::StoredValueType,
            NodeMirDisposition::OneRecordRealizable),
        make_node_kind_entry<PhiNode>(
            NodeKind::Phi, NodeFamily::Semantic,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::PhiMerge,
            phi_refinement, OperandArityPolicy::Variable, 1, variable_arity,
            ResultArityPolicy::One, NodeEffect::None,
            NodeControlBehavior::FallsThrough, NodeTrapBehavior::CannotTrap,
            NodeStage::Raw, semantic_node_stages, NodeTypePolicy::StoredValueType,
            NodeMirDisposition::ForbiddenAtMirBoundary),
        make_node_kind_entry<AllocaAuthorityNode>(
            NodeKind::AllocaAuthority, NodeFamily::Authority,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Authority, 0,
            OperandArityPolicy::Fixed, 0, 0, ResultArityPolicy::One,
            NodeEffect::None, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::CannotTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::StoredValueType,
            NodeMirDisposition::RequiresAllocationOrFrameFacts),
        make_node_kind_entry<StackSaveAuthorityNode>(
            NodeKind::StackSaveAuthority, NodeFamily::Authority,
            NodeValueModel::SingleOrdinaryResult,
            NodeSsaParticipation::SsaEligible, NodeSemanticFamily::Authority, 0,
            OperandArityPolicy::Fixed, 0, 0, ResultArityPolicy::One,
            NodeEffect::Unknown, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::MayTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::StoredValueType,
            NodeMirDisposition::RequiresAllocationOrFrameFacts),
        make_node_kind_entry<StackRestoreAuthorityNode>(
            NodeKind::StackRestoreAuthority, NodeFamily::Authority,
            NodeValueModel::NoOrdinaryResult,
            NodeSsaParticipation::NeverSsa, NodeSemanticFamily::Authority, 0,
            OperandArityPolicy::Fixed, 1, 1, ResultArityPolicy::Zero,
            NodeEffect::Unknown, NodeControlBehavior::FallsThrough,
            NodeTrapBehavior::MayTrap, NodeStage::Raw, semantic_node_stages,
            NodeTypePolicy::NoResultType,
            NodeMirDisposition::RequiresAllocationOrFrameFacts),
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
      descriptor.maximum_operands == variable_arity ||
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
  if (node_kind_registry.size() != static_cast<std::size_t>(NodeKind::Count))
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
                    static_cast<std::size_t>(NodeKind::Count),
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
