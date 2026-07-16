#pragma once

#include "ir.hpp"
#include "result.hpp"
#include "view.hpp"
#include "../verify/verifier.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::bir {

enum class BuildError {
  EpochExhausted,
  AlreadyConsumed,
  ActiveFunctionEdit,
  ExpiredCapability,
  ScopeTokenExhausted,
  EmptyLinkName,
  InvalidNameId,
  EmptyName,
  DuplicateName,
  InvalidStructName,
  DuplicateStructDeclaration,
  ConflictingDeclaration,
  DuplicateDefinition,
  InvalidFunction,
  InvalidFunctionMetadata,
  InvalidParameter,
  DeclarationHasNoBlocks,
  ForeignOwner,
  InvalidBlock,
  InvalidValue,
  InvalidValueType,
  InvalidSourceValueId,
  DuplicateSourceValue,
  EmptyStringDataName,
  DuplicateStringDataName,
  EmptyExternalName,
  InvalidExternalLinkName,
  DuplicateExternalDeclaration,
  EmptyGlobalName,
  InvalidGlobalLinkName,
  InvalidGlobalInitializer,
  InvalidGlobalInitializerLinkName,
  InvalidGlobalObject,
  DuplicateGlobalObject,
  EmptySpecializationField,
  InvalidSpecializationLinkName,
  DuplicateSpecialization,
  DuplicateIntrinsicRequirements,
  ValueAlreadyDefined,
  DefinitionTypeMismatch,
  TerminatorAlreadySet,
  InvalidConditionType,
  InvalidReturn,
  UnsupportedOpcode,
  StorageExhausted,
};

enum class PublishError {
  AlreadyConsumed,
  ActiveFunctionEdit,
  EpochExhausted,
  VerificationFailed,
  PipelineIdentityFailed,
};

struct PublishFailure {
  PublishError reason = PublishError::VerificationFailed;
  VerificationResult verification;
  std::optional<FunctionRevisionDigestError> identity_error;
};

class ModuleBuilder;
class PipelineCheckpoint;

namespace pipeline_internal {
struct CheckpointAccess;
}

namespace detail {
class RawStateToken {
 private:
  RawStateToken() = default;
  friend class ::c4c::backend::bir::ModuleBuilder;
};
}  // namespace detail

class RawBir {
 public:
  RawBir(RawBir&&) noexcept = default;
  RawBir& operator=(RawBir&&) noexcept = default;
  RawBir(const RawBir&) = delete;
  RawBir& operator=(const RawBir&) = delete;

  ModuleView view() const { return ModuleView(*data_); }
  PipelineStageStamp stage_stamp() const noexcept { return data_->stage_stamp_; }

 private:
  RawBir(std::unique_ptr<detail::ModuleData> data, detail::RawStateToken token)
      : data_(std::move(data)), token_(token) {}

  std::unique_ptr<detail::ModuleData> data_;
  detail::RawStateToken token_;

  friend class ModuleBuilder;
  friend class FoundationVerifier;
  friend Result<CanonicalBir, VerificationResult> canonicalize(RawBir&&);
  friend struct pipeline_internal::CheckpointAccess;
};

class CanonicalBir {
 public:
  CanonicalBir(CanonicalBir&&) noexcept = default;
  CanonicalBir& operator=(CanonicalBir&&) noexcept = default;
  CanonicalBir(const CanonicalBir&) = delete;
  CanonicalBir& operator=(const CanonicalBir&) = delete;

  ModuleView view() const { return ModuleView(*data_); }
  PipelineStageStamp stage_stamp() const noexcept { return data_->stage_stamp_; }

 private:
  explicit CanonicalBir(std::unique_ptr<detail::ModuleData> data)
      : data_(std::move(data)) {}

  std::unique_ptr<detail::ModuleData> data_;

  friend Result<CanonicalBir, VerificationResult> canonicalize(RawBir&&);
};

Result<CanonicalBir, VerificationResult> canonicalize(RawBir&& raw);

struct BuildResult {
  InstId instruction{};
  std::vector<ValueId> results;
};

struct InlineAsmSpec {
  std::string asm_text;
  std::string constraint_text;
  std::vector<std::string> clobbers;
  bool side_effects = false;
  std::vector<ValueId> inputs;
  std::vector<Type> result_types;
  // Present only for the bounded source-backed inline-asm result receipt.
  std::optional<std::uint32_t> source_result_id;
};

struct StoreSpec {
  GlobalObjectId destination{};
  Type stored_type{};
  ValueId value{};
};

struct LoadSpec {
  GlobalObjectId source{};
  Type loaded_type{};
  std::uint32_t source_result_id = 0;
};

struct LocalLoadAuthoritySpec {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type loaded_type{};
  bool live = false;
};

struct LocalStoreAuthoritySpec {
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type stored_type{};
  std::int64_t immediate = 0;
  bool live = false;
};

struct GetElementPtrSpec {
  GetElementPtrBase base{};
  Type element_type{};
  bool inbounds = false;
  std::vector<ValueId> indices;
  std::uint32_t source_result_id = 0;
};

struct LocalArrayGepAuthoritySpec {
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

struct AbsSpec {
  Type type{};
  ValueId operand{};
  std::uint32_t source_result_id = 0;
};

struct CallSpec {
  FunctionId callee{};
  std::vector<ValueId> arguments;
  std::optional<std::uint32_t> source_result_id;
  std::optional<DirectScalarBodyParameterFixedDirectCallArgument>
      direct_scalar_argument;
  std::optional<DirectZeroArgScalarFloatingCallResult>
      direct_zero_arg_scalar_floating_result;
};

struct BinarySpec {
  BinaryOpcode opcode = BinaryOpcode::FAdd;
  Type type{};
  ValueId lhs{};
  ValueId rhs{};
  std::uint32_t source_result_id = 0;
  std::optional<DirectScalarBodyParameterBinaryLhs> direct_scalar_lhs;
  std::optional<DirectScalarBodyParameterBinaryRhs> direct_scalar_rhs;
};

struct CompareSpec {
  ComparePredicate predicate = ComparePredicate::Slt;
  Type type{};
  ValueId lhs{};
  ValueId rhs{};
  std::uint32_t source_result_id = 0;
  std::optional<DirectScalarBodyParameterTruthinessComparisonLhs>
      direct_scalar_truthiness_lhs;
  std::optional<DirectPointerBodyParameterTruthiness>
      direct_pointer_truthiness;
};

struct SelectSpec {
  Type type{};
  std::optional<ValueId> condition;
  std::optional<ValueId> false_value;
  std::uint32_t source_result_id = 0;
};

struct SelectedMemcpySpec {
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

struct Amd64SysVOverflowAggregateMemcpySpec {
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

struct IntrinsicCallSpec {
  IntrinsicKind kind = IntrinsicKind::Ctpop;
  LinkNameId callee_link_name{};
  Type type{};
  std::vector<ValueId> arguments;
  std::optional<bool> zero_count_is_undef;
  std::uint32_t source_result_id = 0;
};

struct CastSpec {
  CastKind kind = CastKind::Trunc;
  Type from_type{};
  Type to_type{};
  ValueId operand{};
  std::uint32_t source_result_id = 0;
};

struct PhiIncomingSpec {
  ValueId value{};
  BlockId predecessor{};
  BlockId destination{};
  std::uint32_t occurrence = 0;
};

struct PhiSpec {
  Type type{};
  std::uint32_t source_result_id = 0;
  std::vector<PhiIncomingSpec> incoming;
};

struct AllocaAuthoritySpec {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

struct StackSaveAuthoritySpec {
  SourceValueId result{};
  SourceValueId pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

struct StackRestoreAuthoritySpec {
  SourceValueId saved_pointer_definition{};
  SourceObjectId object{};
  LinkNameId owner{};
  Type pointer_type{TypeKind::Pointer};
  Type pointee_type{};
  bool live = false;
};

using TerminatorSpec = Terminator;

class FunctionBuilder;

class ModuleBuilder {
 public:
  ModuleBuilder();
  ~ModuleBuilder();

  ModuleBuilder(const ModuleBuilder&) = delete;
  ModuleBuilder& operator=(const ModuleBuilder&) = delete;
  ModuleBuilder(ModuleBuilder&&) = delete;
  ModuleBuilder& operator=(ModuleBuilder&&) = delete;

  Result<FunctionId, BuildError> create_function(FunctionSignature signature,
                                                  std::string link_name,
                                                  bool is_declaration,
                                                  FunctionMetadata metadata = {});
  Result<LinkNameId, BuildError> add_link_name(c4c::LinkNameId source_id,
                                               std::string spelling);
  Result<StructNameId, BuildError> add_struct_name(c4c::StructNameId source_id,
                                                   std::string spelling);
  Result<StructDeclId, BuildError> add_struct_declaration(
      c4c::StructNameId source_name_id, std::vector<StructField> fields,
      bool is_packed, bool is_opaque);
  Result<StringDataId, BuildError> add_string_data(std::string pool_name,
                                                  std::string raw_bytes,
                                                  std::int64_t byte_length);
  Result<ExternalDeclId, BuildError> add_external_declaration(
      std::string source_name, Type return_type,
      ReturnExtension return_extension,
      std::optional<c4c::LinkNameId> source_link_name = std::nullopt);
  Result<GlobalObjectId, BuildError> add_global_object(
      std::string source_name, Type object_type, int alignment,
      bool is_internal, bool is_weak, bool is_const,
      bool is_extern_declaration,
      std::optional<c4c::LinkNameId> source_link_name = std::nullopt,
      std::optional<std::string> initializer_payload = std::nullopt,
      std::vector<c4c::LinkNameId> initializer_function_links = {},
      SymbolVisibility visibility = SymbolVisibility::Default);
  Result<SpecializationId, BuildError> add_specialization(
      std::string spec_key, std::string template_origin,
      std::string mangled_name, c4c::LinkNameId mangled_link_name_id);
  Result<void, BuildError> set_intrinsic_requirements(
      IntrinsicRequirements requirements);

  using FunctionEdit =
      std::function<Result<void, BuildError>(FunctionBuilder&)>;
  Result<void, BuildError> with_function(FunctionId function,
                                          FunctionEdit edit);
  Result<RawBir, PublishFailure> publish() &&;

 private:
  enum class State : std::uint8_t { Open, EditingFunction, Consumed };

  Result<void, BuildError> check_function_capability(
      FunctionId function, std::uint64_t scope_token) const;
  Result<std::reference_wrapper<detail::FunctionData>, BuildError>
  mutable_function(FunctionId function);

  std::unique_ptr<detail::ModuleData> data_;
  State state_ = State::Open;
  std::uint64_t active_scope_token_ = 0;
  std::uint64_t next_scope_token_ = 0;
  bool intrinsic_requirements_assigned_ = false;

  friend class FunctionBuilder;
};

class FunctionBuilder {
 public:
  FunctionBuilder(const FunctionBuilder&) = delete;
  FunctionBuilder& operator=(const FunctionBuilder&) = delete;
  FunctionBuilder(FunctionBuilder&&) = delete;
  FunctionBuilder& operator=(FunctionBuilder&&) = delete;

  FunctionId id() const noexcept { return function_; }
  Result<ValueId, BuildError> parameter(std::uint32_t ordinal) const;
  Result<ValueId, BuildError> reserve_value(Type type);
  Result<ValueId, BuildError> reserve_source_value(std::uint32_t source_id,
                                                   Type type);
  Result<void, BuildError> define_int_constant(ValueId value,
                                               std::int64_t exact_value);
  Result<void, BuildError> define_float_constant_bits(ValueId value,
                                                      std::uint64_t exact_bits);
  Result<void, BuildError> define_label_address_constant(ValueId value,
                                                         BlockId target);
  Result<void, BuildError> define_special_constant(ValueId value,
                                                   SpecialConstantKind kind);
  Result<BlockId, BuildError> create_block(std::string debug_name = {});
  Result<BuildResult, BuildError> append(BlockId block, InlineAsmSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, StoreSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, LoadSpec spec);
  Result<BuildResult, BuildError> append(BlockId block,
                                         LocalLoadAuthoritySpec spec);
  Result<BuildResult, BuildError> append(BlockId block,
                                         LocalStoreAuthoritySpec spec);
  Result<BuildResult, BuildError> append(BlockId block,
                                         GetElementPtrSpec spec);
  Result<BuildResult, BuildError> append(BlockId block,
                                         LocalArrayGepAuthoritySpec spec);
  Result<BuildResult, BuildError> append(BlockId block, AbsSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, CallSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, BinarySpec spec);
  Result<BuildResult, BuildError> append(BlockId block, CompareSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, SelectSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, SelectedMemcpySpec spec);
  Result<BuildResult, BuildError> append(
      BlockId block, Amd64SysVOverflowAggregateMemcpySpec spec);
  Result<BuildResult, BuildError> append(BlockId block, IntrinsicCallSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, CastSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, PhiSpec spec);
  Result<BuildResult, BuildError> append(BlockId block, AllocaAuthoritySpec spec);
  Result<BuildResult, BuildError> append(BlockId block, StackSaveAuthoritySpec spec);
  Result<BuildResult, BuildError> append(BlockId block, StackRestoreAuthoritySpec spec);
  Result<void, BuildError> set_terminator(BlockId block,
                                          TerminatorSpec terminator);

 private:
  FunctionBuilder(ModuleBuilder& parent, FunctionId function,
                  std::uint64_t scope_token)
      : parent_(&parent), function_(function), scope_token_(scope_token) {}

  Result<std::reference_wrapper<detail::FunctionData>, BuildError>
  mutable_function() const;

  ModuleBuilder* parent_;
  FunctionId function_;
  std::uint64_t scope_token_;

  friend class ModuleBuilder;
};

}  // namespace c4c::backend::bir
