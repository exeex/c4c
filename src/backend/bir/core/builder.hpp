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
                                                  bool is_declaration);
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
  Result<BlockId, BuildError> create_block(std::string debug_name = {});
  Result<BuildResult, BuildError> append(BlockId block, InlineAsmSpec spec);
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
