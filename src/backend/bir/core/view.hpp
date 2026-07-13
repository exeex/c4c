#pragma once

#include "ir.hpp"
#include "result.hpp"
#include "storage.hpp"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace c4c::backend::bir {

class InstView {
 public:
  InstId id() const noexcept { return id_; }
  Opcode opcode() const noexcept { return data_->opcode; }
  const InstPayload& payload() const noexcept { return data_->payload; }
  const StoreNode* store() const noexcept {
    return std::get_if<StoreNode>(&data_->payload);
  }
  const LoadNode* load() const noexcept {
    return std::get_if<LoadNode>(&data_->payload);
  }
  const GetElementPtrNode* get_element_ptr() const noexcept {
    return std::get_if<GetElementPtrNode>(&data_->payload);
  }
  const CallNode* call() const noexcept {
    return std::get_if<CallNode>(&data_->payload);
  }
  const std::vector<ValueId>& operands() const noexcept { return data_->operands; }
  const std::vector<ValueId>& results() const noexcept { return data_->results; }

 private:
  InstView(InstId id, const detail::InstData& data) : id_(id), data_(&data) {}

  InstId id_{};
  const detail::InstData* data_ = nullptr;

  friend class FunctionView;
};

class BlockView {
 public:
  BlockId id() const noexcept { return id_; }
  std::string debug_name() const { return data_->debug_name_; }

 private:
  BlockView(BlockId id, const detail::BlockData& data) : id_(id), data_(&data) {}

  BlockId id_{};
  const detail::BlockData* data_ = nullptr;

  friend class FunctionView;
};

class FunctionView {
 public:
  FunctionId id() const noexcept { return id_; }
  FunctionRevision revision() const noexcept { return data_->revision_; }
  FunctionSignature signature() const { return data_->signature_; }
  bool is_declaration() const noexcept { return data_->is_declaration_; }
  bool is_internal() const noexcept { return data_->is_internal_; }
  bool can_elide_if_unreferenced() const noexcept {
    return data_->can_elide_if_unreferenced_;
  }
  std::string link_name() const { return data_->link_name_; }
  std::vector<ValueId> parameters() const { return data_->parameters_; }
  std::vector<ValueId> values() const { return data_->value_order_.ids(); }
  std::vector<BlockId> blocks() const { return data_->block_order_.ids(); }

  Result<BlockView, ResolveError> block(BlockId id) const {
    auto resolved = data_->blocks_.get(id_, id);
    if (!resolved)
      return Result<BlockView, ResolveError>::failure(resolved.error());
    return Result<BlockView, ResolveError>::success(BlockView(id, resolved.value().get()));
  }

  Result<ValueDef, ResolveError> value(ValueId id) const {
    auto resolved = data_->values_.get(id_, id);
    if (!resolved)
      return Result<ValueDef, ResolveError>::failure(resolved.error());
    return Result<ValueDef, ResolveError>::success(resolved.value().get());
  }

  Result<ValueId, ResolveError> source_value(SourceValueId id) const {
    if (id.owner.epoch != id_.epoch)
      return Result<ValueId, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.owner != id_)
      return Result<ValueId, ResolveError>::failure(ResolveError::WrongOwner);
    const auto found = data_->values_by_source_id_.find(id.value);
    if (found == data_->values_by_source_id_.end())
      return Result<ValueId, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<ValueId, ResolveError>::success(found->second);
  }

  Result<InstView, ResolveError> instruction(InstId id) const {
    auto resolved = data_->insts_.get(id_, id);
    if (!resolved)
      return Result<InstView, ResolveError>::failure(resolved.error());
    return Result<InstView, ResolveError>::success(
        InstView(id, resolved.value().get()));
  }

  Result<std::vector<InstId>, ResolveError> instructions(BlockId id) const {
    auto resolved = data_->blocks_.get(id_, id);
    if (!resolved)
      return Result<std::vector<InstId>, ResolveError>::failure(resolved.error());
    return Result<std::vector<InstId>, ResolveError>::success(
        resolved.value().get().instruction_order_.ids());
  }

  Result<Terminator, ResolveError> terminator(BlockId id) const {
    auto resolved = data_->blocks_.get(id_, id);
    if (!resolved)
      return Result<Terminator, ResolveError>::failure(resolved.error());
    const auto& terminator = resolved.value().get().terminator_;
    if (!terminator)
      return Result<Terminator, ResolveError>::failure(ResolveError::Tombstone);
    return Result<Terminator, ResolveError>::success(*terminator);
  }

  Result<std::vector<BlockId>, ResolveError> successors(BlockId id) const {
    auto resolved = terminator(id);
    if (!resolved)
      return Result<std::vector<BlockId>, ResolveError>::failure(resolved.error());

    return Result<std::vector<BlockId>, ResolveError>::success(std::visit(
        [](const auto& term) -> std::vector<BlockId> {
          using Term = std::decay_t<decltype(term)>;
          if constexpr (std::is_same_v<Term, JumpTerm>) {
            return {term.target};
          } else if constexpr (std::is_same_v<Term, CondJumpTerm>) {
            return {term.true_target, term.false_target};
          } else {
            return {};
          }
        },
        resolved.value()));
  }

 private:
  FunctionView(const detail::ModuleData& module, FunctionId id,
               const detail::FunctionData& data)
      : module_(&module), id_(id), data_(&data) {}

  const detail::ModuleData* module_ = nullptr;
  FunctionId id_{};
  const detail::FunctionData* data_ = nullptr;

  friend class ModuleView;
};

class ModuleView {
 public:
  ModuleEpoch epoch() const noexcept { return data_->epoch_; }
  ModuleRevision revision() const noexcept { return data_->revision_; }
  IntrinsicRequirements intrinsic_requirements() const noexcept {
    return data_->intrinsic_requirements_;
  }
  std::vector<FunctionId> functions() const { return data_->function_order_.ids(); }
  std::vector<LinkNameId> link_names() const {
    std::vector<LinkNameId> ids;
    ids.reserve(data_->link_names_.size());
    for (std::size_t i = 0; i < data_->link_names_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<StructNameId> struct_names() const {
    std::vector<StructNameId> ids;
    ids.reserve(data_->struct_names_.size());
    for (std::size_t i = 0; i < data_->struct_names_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<StructDeclId> struct_declarations() const {
    std::vector<StructDeclId> ids;
    ids.reserve(data_->struct_decls_.size());
    for (std::size_t i = 0; i < data_->struct_decls_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<ConstantId> constants() const {
    std::vector<ConstantId> ids;
    ids.reserve(data_->constants_.size());
    for (std::size_t i = 0; i < data_->constants_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<StringDataId> string_data() const {
    std::vector<StringDataId> ids;
    ids.reserve(data_->string_data_.size());
    for (std::size_t i = 0; i < data_->string_data_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<ExternalDeclId> external_declarations() const {
    std::vector<ExternalDeclId> ids;
    ids.reserve(data_->external_decls_.size());
    for (std::size_t i = 0; i < data_->external_decls_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<GlobalObjectId> global_objects() const {
    std::vector<GlobalObjectId> ids;
    ids.reserve(data_->globals_.size());
    for (std::size_t i = 0; i < data_->globals_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }
  std::vector<SpecializationId> specializations() const {
    std::vector<SpecializationId> ids;
    ids.reserve(data_->specializations_.size());
    for (std::size_t i = 0; i < data_->specializations_.size(); ++i)
      ids.push_back({data_->epoch_, static_cast<SlotIndex>(i)});
    return ids;
  }

  Result<std::string, ResolveError> spelling(LinkNameId id) const {
    if (id.epoch != data_->epoch_)
      return Result<std::string, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->link_names_.size())
      return Result<std::string, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<std::string, ResolveError>::success(data_->link_names_[id.slot].spelling);
  }
  Result<std::string, ResolveError> spelling(StructNameId id) const {
    if (id.epoch != data_->epoch_)
      return Result<std::string, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->struct_names_.size())
      return Result<std::string, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<std::string, ResolveError>::success(data_->struct_names_[id.slot].spelling);
  }
  Result<c4c::LinkNameId, ResolveError> source_id(LinkNameId id) const {
    if (id.epoch != data_->epoch_)
      return Result<c4c::LinkNameId, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->link_names_.size())
      return Result<c4c::LinkNameId, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<c4c::LinkNameId, ResolveError>::success(data_->link_names_[id.slot].source_id);
  }
  Result<c4c::StructNameId, ResolveError> source_id(StructNameId id) const {
    if (id.epoch != data_->epoch_)
      return Result<c4c::StructNameId, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->struct_names_.size())
      return Result<c4c::StructNameId, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<c4c::StructNameId, ResolveError>::success(data_->struct_names_[id.slot].source_id);
  }
  Result<StructDeclaration, ResolveError> struct_declaration(StructDeclId id) const {
    if (id.epoch != data_->epoch_)
      return Result<StructDeclaration, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->struct_decls_.size())
      return Result<StructDeclaration, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<StructDeclaration, ResolveError>::success(data_->struct_decls_[id.slot]);
  }
  Result<ConstantDefinition, ResolveError> constant(ConstantId id) const {
    if (id.epoch != data_->epoch_)
      return Result<ConstantDefinition, ResolveError>::failure(
          ResolveError::WrongEpoch);
    if (id.slot >= data_->constants_.size())
      return Result<ConstantDefinition, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<ConstantDefinition, ResolveError>::success(
        data_->constants_[id.slot]);
  }
  Result<StringData, ResolveError> string_data(StringDataId id) const {
    if (id.epoch != data_->epoch_)
      return Result<StringData, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->string_data_.size())
      return Result<StringData, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<StringData, ResolveError>::success(data_->string_data_[id.slot]);
  }
  Result<StringDataId, ResolveError> string_data(
      const std::string& pool_name) const {
    const auto found = data_->string_data_by_name_.find(pool_name);
    if (found == data_->string_data_by_name_.end())
      return Result<StringDataId, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<StringDataId, ResolveError>::success(found->second);
  }
  Result<ExternalDeclaration, ResolveError> external_declaration(
      ExternalDeclId id) const {
    if (id.epoch != data_->epoch_)
      return Result<ExternalDeclaration, ResolveError>::failure(
          ResolveError::WrongEpoch);
    if (id.slot >= data_->external_decls_.size())
      return Result<ExternalDeclaration, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<ExternalDeclaration, ResolveError>::success(
        data_->external_decls_[id.slot]);
  }
  Result<ExternalDeclId, ResolveError> external_declaration(
      const std::string& source_name) const {
    const auto found = data_->external_decls_by_name_.find(source_name);
    if (found == data_->external_decls_by_name_.end())
      return Result<ExternalDeclId, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<ExternalDeclId, ResolveError>::success(found->second);
  }
  Result<GlobalObject, ResolveError> global_object(GlobalObjectId id) const {
    if (id.epoch != data_->epoch_)
      return Result<GlobalObject, ResolveError>::failure(ResolveError::WrongEpoch);
    if (id.slot >= data_->globals_.size())
      return Result<GlobalObject, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<GlobalObject, ResolveError>::success(data_->globals_[id.slot]);
  }
  Result<SpecializationMetadata, ResolveError> specialization(
      SpecializationId id) const {
    if (id.epoch != data_->epoch_)
      return Result<SpecializationMetadata, ResolveError>::failure(
          ResolveError::WrongEpoch);
    if (id.slot >= data_->specializations_.size())
      return Result<SpecializationMetadata, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<SpecializationMetadata, ResolveError>::success(
        data_->specializations_[id.slot]);
  }
  Result<GlobalObjectId, ResolveError> global_object(
      const std::string& source_name) const {
    const auto found = data_->globals_by_name_.find(source_name);
    if (found == data_->globals_by_name_.end())
      return Result<GlobalObjectId, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<GlobalObjectId, ResolveError>::success(found->second);
  }
  Result<GlobalObjectId, ResolveError> global_object(LinkNameId link_name) const {
    if (link_name.epoch != data_->epoch_)
      return Result<GlobalObjectId, ResolveError>::failure(ResolveError::WrongEpoch);
    const auto found = data_->globals_by_link_name_.find(link_name);
    if (found == data_->globals_by_link_name_.end())
      return Result<GlobalObjectId, ResolveError>::failure(ResolveError::OutOfRange);
    return Result<GlobalObjectId, ResolveError>::success(found->second);
  }
  Result<ExternalDeclId, ResolveError> external_declaration(
      LinkNameId link_name) const {
    if (link_name.epoch != data_->epoch_)
      return Result<ExternalDeclId, ResolveError>::failure(
          ResolveError::WrongEpoch);
    const auto found = data_->external_decls_by_link_name_.find(link_name);
    if (found == data_->external_decls_by_link_name_.end())
      return Result<ExternalDeclId, ResolveError>::failure(
          ResolveError::OutOfRange);
    return Result<ExternalDeclId, ResolveError>::success(found->second);
  }

  Result<FunctionView, ResolveError> function(FunctionId id) const {
    auto resolved = data_->functions_.get(data_->epoch_, id);
    if (!resolved)
      return Result<FunctionView, ResolveError>::failure(resolved.error());
    return Result<FunctionView, ResolveError>::success(
        FunctionView(*data_, id, resolved.value().get()));
  }

 private:
  explicit ModuleView(const detail::ModuleData& data) : data_(&data) {}

  const detail::ModuleData* data_ = nullptr;

  friend class RawBir;
  friend class CanonicalBir;
  friend class ModuleBuilder;
  friend class PipelineCheckpoint;
  friend class PrivateOccurrenceCandidate;
};

}  // namespace c4c::backend::bir
