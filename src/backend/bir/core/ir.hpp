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

using ConstantPayload = std::variant<IntegerConstant, FloatingConstant>;

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
  Cast,
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

struct LoadNode {
  GlobalObjectId source{};
  Type loaded_type{};
};

struct GetElementPtrNode {
  GlobalObjectId base{};
  Type element_type{};
  bool inbounds = false;
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

using InstPayload =
    std::variant<InlineAsmNode, StoreNode, LoadNode, GetElementPtrNode,
    AbsNode, CallNode, BinaryNode, CompareNode, SelectNode, IntrinsicCallNode, CastNode>;

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

struct ReturnTerm {
  std::optional<ValueId> value;
};

struct UnreachableTerm {};

using Terminator =
    std::variant<JumpTerm, CondJumpTerm, ReturnTerm, UnreachableTerm>;

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
