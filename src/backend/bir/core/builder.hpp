#pragma once

#include "ir.hpp"
#include "result.hpp"
#include "view.hpp"
#include "../verify/verifier.hpp"

#include <cstdint>
#include <functional>
#include <memory>
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
  ConflictingDeclaration,
  DuplicateDefinition,
  InvalidFunction,
  InvalidParameter,
  DeclarationHasNoBlocks,
  ForeignOwner,
  InvalidBlock,
  InvalidValue,
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
};

struct PublishFailure {
  PublishError reason = PublishError::VerificationFailed;
  VerificationResult verification;
};

class ModuleBuilder;

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

 private:
  RawBir(std::unique_ptr<detail::ModuleData> data, detail::RawStateToken token)
      : data_(std::move(data)), token_(token) {}

  std::unique_ptr<detail::ModuleData> data_;
  detail::RawStateToken token_;

  friend class ModuleBuilder;
};

struct BuildResult {
  InstId instruction{};
  std::vector<ValueId> results;
};

struct UnsupportedInstSpec {};

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
  Result<BlockId, BuildError> create_block(std::string debug_name = {});
  Result<BuildResult, BuildError> append(BlockId block,
                                         UnsupportedInstSpec);
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
