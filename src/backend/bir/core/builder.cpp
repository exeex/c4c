#include "builder.hpp"

#include <atomic>
#include <limits>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace c4c::backend::bir {
namespace {

std::atomic<ModuleEpoch> next_module_epoch{1};

ModuleEpoch allocate_module_epoch() noexcept {
  auto epoch = next_module_epoch.load(std::memory_order_relaxed);
  while (epoch != 0 && epoch != std::numeric_limits<ModuleEpoch>::max()) {
    if (next_module_epoch.compare_exchange_weak(
            epoch, epoch + 1, std::memory_order_relaxed,
            std::memory_order_relaxed))
      return epoch;
  }
  if (epoch == std::numeric_limits<ModuleEpoch>::max() &&
      next_module_epoch.compare_exchange_strong(
          epoch, 0, std::memory_order_relaxed, std::memory_order_relaxed))
    return std::numeric_limits<ModuleEpoch>::max();
  return 0;
}

BuildError storage_error(detail::StorageError) noexcept {
  return BuildError::StorageExhausted;
}

bool same_owner(FunctionId function, BlockId block) noexcept {
  return block.owner == function;
}

bool same_owner(FunctionId function, ValueId value) noexcept {
  return value.owner == function;
}

bool integer_type(const Type& type) noexcept {
  switch (type.kind) {
    case TypeKind::I1:
    case TypeKind::I8:
    case TypeKind::I16:
    case TypeKind::I32:
    case TypeKind::I64:
    case TypeKind::Integer: return true;
    default: return false;
  }
}

bool floating_type(const Type& type) noexcept {
  return type.kind == TypeKind::F32 || type.kind == TypeKind::F64 ||
         type.kind == TypeKind::Floating;
}

}  // namespace

ModuleBuilder::ModuleBuilder() : data_(std::make_unique<detail::ModuleData>()) {
  data_->epoch_ = allocate_module_epoch();
}

ModuleBuilder::~ModuleBuilder() = default;

Result<LinkNameId, BuildError> ModuleBuilder::add_link_name(
    c4c::LinkNameId source_id, std::string spelling) {
  if (state_ == State::Consumed)
    return Result<LinkNameId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<LinkNameId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<LinkNameId, BuildError>::failure(BuildError::EpochExhausted);
  if (source_id == c4c::kInvalidLinkName ||
      source_id != data_->link_names_.size() + 1)
    return Result<LinkNameId, BuildError>::failure(BuildError::InvalidNameId);
  if (spelling.empty())
    return Result<LinkNameId, BuildError>::failure(BuildError::EmptyName);
  if (data_->link_names_by_spelling_.count(spelling) != 0)
    return Result<LinkNameId, BuildError>::failure(BuildError::DuplicateName);
  const LinkNameId id{data_->epoch_,
                      static_cast<SlotIndex>(data_->link_names_.size())};
  data_->link_names_.push_back({source_id, spelling});
  data_->link_names_by_source_id_.emplace(source_id, id);
  data_->link_names_by_spelling_.emplace(std::move(spelling), id);
  return Result<LinkNameId, BuildError>::success(id);
}

Result<StructNameId, BuildError> ModuleBuilder::add_struct_name(
    c4c::StructNameId source_id, std::string spelling) {
  if (state_ == State::Consumed)
    return Result<StructNameId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<StructNameId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<StructNameId, BuildError>::failure(BuildError::EpochExhausted);
  if (source_id == c4c::kInvalidStructName ||
      source_id != data_->struct_names_.size() + 1)
    return Result<StructNameId, BuildError>::failure(BuildError::InvalidNameId);
  if (spelling.empty())
    return Result<StructNameId, BuildError>::failure(BuildError::EmptyName);
  if (data_->struct_names_by_spelling_.count(spelling) != 0)
    return Result<StructNameId, BuildError>::failure(BuildError::DuplicateName);
  const StructNameId id{data_->epoch_,
                        static_cast<SlotIndex>(data_->struct_names_.size())};
  data_->struct_names_.push_back({source_id, spelling});
  data_->struct_names_by_source_id_.emplace(source_id, id);
  data_->struct_names_by_spelling_.emplace(std::move(spelling), id);
  return Result<StructNameId, BuildError>::success(id);
}

Result<StructDeclId, BuildError> ModuleBuilder::add_struct_declaration(
    c4c::StructNameId source_name_id, std::vector<StructField> fields,
    bool is_packed, bool is_opaque) {
  if (state_ == State::Consumed)
    return Result<StructDeclId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<StructDeclId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<StructDeclId, BuildError>::failure(BuildError::EpochExhausted);
  const auto name = data_->struct_names_by_source_id_.find(source_name_id);
  if (name == data_->struct_names_by_source_id_.end())
    return Result<StructDeclId, BuildError>::failure(BuildError::InvalidStructName);
  if (data_->struct_decls_by_name_.count(name->second) != 0)
    return Result<StructDeclId, BuildError>::failure(
        BuildError::DuplicateStructDeclaration);
  for (auto& field : fields) {
    if (field.type.struct_name_id == c4c::kInvalidStructName) continue;
    const auto referenced = data_->struct_names_by_source_id_.find(
        field.type.struct_name_id);
    if (referenced == data_->struct_names_by_source_id_.end())
      return Result<StructDeclId, BuildError>::failure(
          BuildError::InvalidStructName);
    field.referenced_name = referenced->second;
  }
  const StructDeclId id{data_->epoch_,
                        static_cast<SlotIndex>(data_->struct_decls_.size())};
  data_->struct_decls_.push_back(
      StructDeclaration{name->second, std::move(fields), is_packed, is_opaque});
  data_->struct_decls_by_name_.emplace(name->second, id);
  return Result<StructDeclId, BuildError>::success(id);
}

Result<StringDataId, BuildError> ModuleBuilder::add_string_data(
    std::string pool_name, std::string raw_bytes, std::int64_t byte_length) {
  if (state_ == State::Consumed)
    return Result<StringDataId, BuildError>::failure(
        BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<StringDataId, BuildError>::failure(
        BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<StringDataId, BuildError>::failure(
        BuildError::EpochExhausted);
  if (pool_name.empty())
    return Result<StringDataId, BuildError>::failure(
        BuildError::EmptyStringDataName);
  if (data_->string_data_by_name_.count(pool_name) != 0)
    return Result<StringDataId, BuildError>::failure(
        BuildError::DuplicateStringDataName);
  if (data_->string_data_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<StringDataId, BuildError>::failure(
        BuildError::StorageExhausted);

  const StringDataId id{
      data_->epoch_, static_cast<SlotIndex>(data_->string_data_.size())};
  data_->string_data_.push_back(
      StringData{pool_name, std::move(raw_bytes), byte_length});
  try {
    const auto inserted =
        data_->string_data_by_name_.emplace(std::move(pool_name), id);
    if (!inserted.second) {
      data_->string_data_.pop_back();
      return Result<StringDataId, BuildError>::failure(
          BuildError::DuplicateStringDataName);
    }
  } catch (...) {
    data_->string_data_.pop_back();
    throw;
  }
  return Result<StringDataId, BuildError>::success(id);
}

Result<ExternalDeclId, BuildError> ModuleBuilder::add_external_declaration(
    std::string source_name, Type return_type,
    ReturnExtension return_extension,
    std::optional<c4c::LinkNameId> source_link_name) {
  if (state_ == State::Consumed)
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::EpochExhausted);
  if (source_name.empty())
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::EmptyExternalName);
  if (data_->external_decls_by_name_.count(source_name) != 0)
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::DuplicateExternalDeclaration);
  if (data_->external_decls_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<ExternalDeclId, BuildError>::failure(
        BuildError::StorageExhausted);

  std::variant<LinkNameId, FallbackExternalName> identity =
      FallbackExternalName{source_name};
  if (source_link_name) {
    if (*source_link_name == c4c::kInvalidLinkName)
      return Result<ExternalDeclId, BuildError>::failure(
          BuildError::InvalidExternalLinkName);
    const auto linked =
        data_->link_names_by_source_id_.find(*source_link_name);
    if (linked == data_->link_names_by_source_id_.end() ||
        linked->second.slot >= data_->link_names_.size() ||
        data_->link_names_[linked->second.slot].spelling != source_name)
      return Result<ExternalDeclId, BuildError>::failure(
          BuildError::InvalidExternalLinkName);
    if (data_->external_decls_by_link_name_.count(linked->second) != 0)
      return Result<ExternalDeclId, BuildError>::failure(
          BuildError::DuplicateExternalDeclaration);
    identity = linked->second;
  }

  const ExternalDeclId id{
      data_->epoch_, static_cast<SlotIndex>(data_->external_decls_.size())};
  data_->external_decls_.push_back(ExternalDeclaration{
      source_name, std::move(return_type), return_extension, identity});
  try {
    data_->external_decls_by_name_.emplace(source_name, id);
    if (const auto* link_name = std::get_if<LinkNameId>(&identity))
      data_->external_decls_by_link_name_.emplace(*link_name, id);
  } catch (...) {
    data_->external_decls_by_name_.erase(source_name);
    if (const auto* link_name = std::get_if<LinkNameId>(&identity))
      data_->external_decls_by_link_name_.erase(*link_name);
    data_->external_decls_.pop_back();
    throw;
  }
  return Result<ExternalDeclId, BuildError>::success(id);
}

Result<GlobalObjectId, BuildError> ModuleBuilder::add_global_object(
    std::string source_name, Type object_type, int alignment,
    bool is_internal, bool is_weak, bool is_const,
    bool is_extern_declaration,
    std::optional<c4c::LinkNameId> source_link_name,
    std::optional<std::string> initializer_payload,
    std::vector<c4c::LinkNameId> initializer_function_links,
    SymbolVisibility visibility) {
  if (state_ == State::Consumed)
    return Result<GlobalObjectId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<GlobalObjectId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<GlobalObjectId, BuildError>::failure(BuildError::EpochExhausted);
  if (source_name.empty())
    return Result<GlobalObjectId, BuildError>::failure(BuildError::EmptyGlobalName);
  if (data_->globals_by_name_.count(source_name) != 0)
    return Result<GlobalObjectId, BuildError>::failure(BuildError::DuplicateGlobalObject);
  if (data_->globals_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<GlobalObjectId, BuildError>::failure(BuildError::StorageExhausted);

  std::variant<LinkNameId, FallbackGlobalName> identity =
      FallbackGlobalName{source_name};
  if (source_link_name) {
    if (*source_link_name == c4c::kInvalidLinkName)
      return Result<GlobalObjectId, BuildError>::failure(
          BuildError::InvalidGlobalLinkName);
    const auto linked = data_->link_names_by_source_id_.find(*source_link_name);
    if (linked == data_->link_names_by_source_id_.end() ||
        linked->second.slot >= data_->link_names_.size() ||
        data_->link_names_[linked->second.slot].spelling != source_name)
      return Result<GlobalObjectId, BuildError>::failure(
          BuildError::InvalidGlobalLinkName);
    if (data_->globals_by_link_name_.count(linked->second) != 0)
      return Result<GlobalObjectId, BuildError>::failure(
          BuildError::DuplicateGlobalObject);
    identity = linked->second;
  }

  if (!initializer_payload && !initializer_function_links.empty())
    return Result<GlobalObjectId, BuildError>::failure(
        BuildError::InvalidGlobalInitializer);
  std::vector<LinkNameId> resolved_initializer_links;
  resolved_initializer_links.reserve(initializer_function_links.size());
  for (const c4c::LinkNameId source_id : initializer_function_links) {
    if (source_id == c4c::kInvalidLinkName)
      return Result<GlobalObjectId, BuildError>::failure(
          BuildError::InvalidGlobalInitializerLinkName);
    const auto linked = data_->link_names_by_source_id_.find(source_id);
    if (linked == data_->link_names_by_source_id_.end() ||
        linked->second.slot >= data_->link_names_.size())
      return Result<GlobalObjectId, BuildError>::failure(
          BuildError::InvalidGlobalInitializerLinkName);
    resolved_initializer_links.push_back(linked->second);
  }

  std::optional<GlobalInitializer> initializer;
  if (initializer_payload)
    initializer = GlobalInitializer{std::move(*initializer_payload),
                                    std::move(resolved_initializer_links)};

  const GlobalObjectId id{data_->epoch_,
                          static_cast<SlotIndex>(data_->globals_.size())};
  data_->globals_.push_back(GlobalObject{source_name, std::move(object_type),
                                         identity, alignment, is_internal,
                                         is_weak, is_const,
                                         is_extern_declaration,
                                         visibility,
                                         std::move(initializer)});
  try {
    data_->globals_by_name_.emplace(source_name, id);
    if (const auto* link_name = std::get_if<LinkNameId>(&identity))
      data_->globals_by_link_name_.emplace(*link_name, id);
  } catch (...) {
    data_->globals_by_name_.erase(source_name);
    if (const auto* link_name = std::get_if<LinkNameId>(&identity))
      data_->globals_by_link_name_.erase(*link_name);
    data_->globals_.pop_back();
    throw;
  }
  return Result<GlobalObjectId, BuildError>::success(id);
}

Result<SpecializationId, BuildError> ModuleBuilder::add_specialization(
    std::string spec_key, std::string template_origin,
    std::string mangled_name, c4c::LinkNameId mangled_link_name_id) {
  if (state_ == State::Consumed)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::EpochExhausted);
  if (spec_key.empty() || template_origin.empty() || mangled_name.empty())
    return Result<SpecializationId, BuildError>::failure(
        BuildError::EmptySpecializationField);
  if (mangled_link_name_id == c4c::kInvalidLinkName)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::InvalidSpecializationLinkName);

  const auto linked =
      data_->link_names_by_source_id_.find(mangled_link_name_id);
  if (linked == data_->link_names_by_source_id_.end() ||
      linked->second.slot >= data_->link_names_.size() ||
      data_->link_names_[linked->second.slot].spelling != mangled_name)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::InvalidSpecializationLinkName);

  detail::ModuleData::SpecializationSemanticKey semantic_key{
      spec_key, template_origin};
  if (data_->specializations_by_semantic_key_.count(semantic_key) != 0 ||
      data_->specializations_by_link_name_.count(linked->second) != 0)
    return Result<SpecializationId, BuildError>::failure(
        BuildError::DuplicateSpecialization);
  if (data_->specializations_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<SpecializationId, BuildError>::failure(
        BuildError::StorageExhausted);

  const SpecializationId id{
      data_->epoch_, static_cast<SlotIndex>(data_->specializations_.size())};
  data_->specializations_.push_back(SpecializationMetadata{
      spec_key, template_origin, std::move(mangled_name), linked->second});
  try {
    data_->specializations_by_semantic_key_.emplace(std::move(semantic_key), id);
    data_->specializations_by_link_name_.emplace(linked->second, id);
  } catch (...) {
    data_->specializations_by_semantic_key_.erase(
        detail::ModuleData::SpecializationSemanticKey{spec_key,
                                                       template_origin});
    data_->specializations_by_link_name_.erase(linked->second);
    data_->specializations_.pop_back();
    throw;
  }
  return Result<SpecializationId, BuildError>::success(id);
}

Result<void, BuildError> ModuleBuilder::set_intrinsic_requirements(
    IntrinsicRequirements requirements) {
  if (state_ == State::Consumed)
    return Result<void, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<void, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<void, BuildError>::failure(BuildError::EpochExhausted);
  if (intrinsic_requirements_assigned_)
    return Result<void, BuildError>::failure(
        BuildError::DuplicateIntrinsicRequirements);
  data_->intrinsic_requirements_ = requirements;
  intrinsic_requirements_assigned_ = true;
  return Result<void, BuildError>::success();
}

Result<std::reference_wrapper<detail::FunctionData>, BuildError>
ModuleBuilder::mutable_function(FunctionId function) {
  if (!data_ || data_->epoch_ == 0)
    return Result<std::reference_wrapper<detail::FunctionData>, BuildError>::failure(
        BuildError::EpochExhausted);
  auto resolved = data_->functions_.get_mut(data_->epoch_, function);
  if (!resolved)
    return Result<std::reference_wrapper<detail::FunctionData>, BuildError>::failure(
        BuildError::InvalidFunction);
  return Result<std::reference_wrapper<detail::FunctionData>, BuildError>::success(
      resolved.value());
}

Result<FunctionId, BuildError> ModuleBuilder::create_function(
    FunctionSignature signature, std::string link_name, bool is_declaration,
    FunctionMetadata metadata) {
  if (state_ == State::Consumed)
    return Result<FunctionId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<FunctionId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<FunctionId, BuildError>::failure(BuildError::EpochExhausted);
  if (link_name.empty())
    return Result<FunctionId, BuildError>::failure(BuildError::EmptyLinkName);
  if ((metadata.is_internal && !metadata.can_elide_if_unreferenced) ||
      (is_declaration &&
       (metadata.is_internal || metadata.can_elide_if_unreferenced)))
    return Result<FunctionId, BuildError>::failure(
        BuildError::InvalidFunctionMetadata);
  if (signature.parameter_types.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    return Result<FunctionId, BuildError>::failure(BuildError::StorageExhausted);

  const auto existing = data_->functions_by_link_name_.find(link_name);
  if (existing != data_->functions_by_link_name_.end()) {
    auto function = data_->functions_.get_mut(data_->epoch_, existing->second);
    if (!function)
      return Result<FunctionId, BuildError>::failure(BuildError::InvalidFunction);
    auto& stored = function.value().get();
    if (stored.signature_ != signature ||
        stored.is_internal_ != metadata.is_internal ||
        stored.can_elide_if_unreferenced_ !=
            metadata.can_elide_if_unreferenced)
      return Result<FunctionId, BuildError>::failure(
          BuildError::ConflictingDeclaration);
    if (!is_declaration && !stored.is_declaration_)
      return Result<FunctionId, BuildError>::failure(BuildError::DuplicateDefinition);
    if (!is_declaration)
      stored.is_declaration_ = false;
    return Result<FunctionId, BuildError>::success(existing->second);
  }

  detail::FunctionData function;
  function.signature_ = std::move(signature);
  function.is_declaration_ = is_declaration;
  function.is_internal_ = metadata.is_internal;
  function.can_elide_if_unreferenced_ = metadata.can_elide_if_unreferenced;
  function.link_name_ = link_name;
  auto inserted = data_->functions_.emplace(data_->epoch_, std::move(function));
  if (!inserted)
    return Result<FunctionId, BuildError>::failure(storage_error(inserted.error()));
  const auto id = inserted.value();

  auto stored_result = data_->functions_.get_mut(data_->epoch_, id);
  if (!stored_result)
    return Result<FunctionId, BuildError>::failure(BuildError::InvalidFunction);
  auto& stored = stored_result.value().get();
  stored.parameters_.reserve(stored.signature_.parameter_types.size());
  for (std::size_t ordinal = 0; ordinal < stored.signature_.parameter_types.size();
       ++ordinal) {
    ValueDef parameter;
    parameter.kind = ValueKind::Parameter;
    parameter.type = stored.signature_.parameter_types[ordinal];
    parameter.definition = ParameterDef{static_cast<std::uint32_t>(ordinal)};
    auto value = stored.values_.emplace(id, std::move(parameter));
    if (!value)
      return Result<FunctionId, BuildError>::failure(storage_error(value.error()));
    if (!stored.value_order_.append(value.value()))
      return Result<FunctionId, BuildError>::failure(BuildError::StorageExhausted);
    stored.parameters_.push_back(value.value());
  }

  auto ordered = data_->function_order_.append(id);
  if (!ordered)
    return Result<FunctionId, BuildError>::failure(BuildError::StorageExhausted);
  data_->functions_by_link_name_.emplace(std::move(link_name), id);
  return Result<FunctionId, BuildError>::success(id);
}

Result<void, BuildError> ModuleBuilder::check_function_capability(
    FunctionId function, std::uint64_t scope_token) const {
  if (state_ == State::Consumed)
    return Result<void, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ != State::EditingFunction || scope_token == 0 ||
      scope_token != active_scope_token_)
    return Result<void, BuildError>::failure(BuildError::ExpiredCapability);
  if (!data_ || !data_->functions_.contains(data_->epoch_, function))
    return Result<void, BuildError>::failure(BuildError::InvalidFunction);
  return Result<void, BuildError>::success();
}

Result<void, BuildError> ModuleBuilder::with_function(FunctionId function,
                                                       FunctionEdit edit) {
  if (state_ == State::Consumed)
    return Result<void, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<void, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<void, BuildError>::failure(BuildError::EpochExhausted);
  if (!data_->functions_.contains(data_->epoch_, function))
    return Result<void, BuildError>::failure(BuildError::InvalidFunction);
  if (next_scope_token_ == std::numeric_limits<std::uint64_t>::max())
    return Result<void, BuildError>::failure(BuildError::ScopeTokenExhausted);

  ++next_scope_token_;
  active_scope_token_ = next_scope_token_;
  state_ = State::EditingFunction;
  FunctionBuilder builder(*this, function, active_scope_token_);
  try {
    auto result = edit(builder);
    active_scope_token_ = 0;
    state_ = State::Open;
    return result;
  } catch (...) {
    active_scope_token_ = 0;
    state_ = State::Open;
    throw;
  }
}

Result<RawBir, PublishFailure> ModuleBuilder::publish() && {
  if (state_ == State::Consumed)
    return Result<RawBir, PublishFailure>::failure(
        {PublishError::AlreadyConsumed, {}, std::nullopt});
  if (state_ == State::EditingFunction)
    return Result<RawBir, PublishFailure>::failure(
        {PublishError::ActiveFunctionEdit, {}, std::nullopt});
  if (!data_ || data_->epoch_ == 0)
    return Result<RawBir, PublishFailure>::failure(
        {PublishError::EpochExhausted, {}, std::nullopt});

  auto verification = FoundationVerifier::verify(*data_);
  if (!verification)
    return Result<RawBir, PublishFailure>::failure(
        {PublishError::VerificationFailed, std::move(verification), std::nullopt});

  std::vector<FunctionRevisionEntry> revisions;
  revisions.reserve(data_->function_order_.ids().size());
  for (const auto function : data_->function_order_.ids()) {
    auto resolved = data_->functions_.get(data_->epoch_, function);
    if (!resolved) {
      const FunctionRevisionDigestError identity_error{
          function.valid()
              ? FunctionRevisionDigestErrorCode::ForeignFunctionId
              : FunctionRevisionDigestErrorCode::InvalidFunctionId,
          revisions.size(), function};
      return Result<RawBir, PublishFailure>::failure(
          {PublishError::PipelineIdentityFailed, {}, identity_error});
    }
    revisions.push_back({function, resolved.value().get().revision_});
  }
  auto digest = compute_function_revision_digest(data_->epoch_, revisions);
  if (!digest)
    return Result<RawBir, PublishFailure>::failure(
        {PublishError::PipelineIdentityFailed, {}, digest.error()});
  data_->stage_stamp_ = {data_->epoch_, data_->revision_, digest.value()};

  state_ = State::Consumed;
  return Result<RawBir, PublishFailure>::success(
      RawBir(std::move(data_), detail::RawStateToken{}));
}

Result<CanonicalBir, VerificationResult> canonicalize(RawBir&& raw) {
  auto verification = FoundationVerifier::verify(
      *raw.data_, VerifyProfile::TargetIndependentCanonical);
  if (!verification)
    return Result<CanonicalBir, VerificationResult>::failure(
        std::move(verification));
  return Result<CanonicalBir, VerificationResult>::success(
      CanonicalBir(std::move(raw.data_)));
}

Result<std::reference_wrapper<detail::FunctionData>, BuildError>
FunctionBuilder::mutable_function() const {
  auto valid = parent_->check_function_capability(function_, scope_token_);
  if (!valid)
    return Result<std::reference_wrapper<detail::FunctionData>, BuildError>::failure(
        valid.error());
  return parent_->mutable_function(function_);
}

Result<ValueId, BuildError> FunctionBuilder::parameter(
    std::uint32_t ordinal) const {
  auto function = mutable_function();
  if (!function)
    return Result<ValueId, BuildError>::failure(function.error());
  const auto& parameters = function.value().get().parameters_;
  if (static_cast<std::size_t>(ordinal) >= parameters.size())
    return Result<ValueId, BuildError>::failure(BuildError::InvalidParameter);
  return Result<ValueId, BuildError>::success(parameters[ordinal]);
}

Result<ValueId, BuildError> FunctionBuilder::reserve_value(Type type) {
  auto function = mutable_function();
  if (!function)
    return Result<ValueId, BuildError>::failure(function.error());
  if (!is_well_formed(type) || type.kind == TypeKind::Void)
    return Result<ValueId, BuildError>::failure(BuildError::InvalidValueType);
  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = std::move(type);
  auto inserted =
      function.value().get().values_.emplace(function_, std::move(value));
  if (!inserted)
    return Result<ValueId, BuildError>::failure(storage_error(inserted.error()));
  if (!function.value().get().value_order_.append(inserted.value())) {
    function.value().get().values_.erase(function_, inserted.value());
    return Result<ValueId, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<ValueId, BuildError>::success(inserted.value());
}

Result<ValueId, BuildError> FunctionBuilder::reserve_source_value(
    std::uint32_t source_id, Type type) {
  auto function = mutable_function();
  if (!function)
    return Result<ValueId, BuildError>::failure(function.error());
  if (source_id == std::numeric_limits<std::uint32_t>::max())
    return Result<ValueId, BuildError>::failure(BuildError::InvalidSourceValueId);
  auto& data = function.value().get();
  if (data.values_by_source_id_.count(source_id) != 0)
    return Result<ValueId, BuildError>::failure(BuildError::DuplicateSourceValue);
  if (!is_well_formed(type) || type.kind == TypeKind::Void)
    return Result<ValueId, BuildError>::failure(BuildError::InvalidValueType);
  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = std::move(type);
  value.source_id = SourceValueId{function_, source_id};
  auto inserted = data.values_.emplace(function_, std::move(value));
  if (!inserted)
    return Result<ValueId, BuildError>::failure(storage_error(inserted.error()));
  if (!data.value_order_.append(inserted.value())) {
    data.values_.erase(function_, inserted.value());
    return Result<ValueId, BuildError>::failure(BuildError::StorageExhausted);
  }
  try {
    data.values_by_source_id_.emplace(source_id, inserted.value());
  } catch (...) {
    data.value_order_.erase(inserted.value());
    data.values_.erase(function_, inserted.value());
    throw;
  }
  return Result<ValueId, BuildError>::success(inserted.value());
}

Result<void, BuildError> FunctionBuilder::define_int_constant(
    ValueId value, std::int64_t exact_value) {
  auto function = mutable_function();
  if (!function)
    return Result<void, BuildError>::failure(function.error());
  if (!same_owner(function_, value))
    return Result<void, BuildError>::failure(BuildError::ForeignOwner);
  auto resolved = function.value().get().values_.get_mut(function_, value);
  if (!resolved)
    return Result<void, BuildError>::failure(BuildError::InvalidValue);
  auto& definition = resolved.value().get();
  if (definition.kind != ValueKind::Ordinary || !integer_type(definition.type))
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if (!std::holds_alternative<UnresolvedDef>(definition.definition))
    return Result<void, BuildError>::failure(BuildError::ValueAlreadyDefined);
  if (parent_->data_->constants_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
  const ConstantId constant{
      parent_->data_->epoch_,
      static_cast<SlotIndex>(parent_->data_->constants_.size())};
  parent_->data_->constants_.push_back(
      ConstantDefinition{definition.type, IntegerConstant{exact_value}});
  definition.definition = ConstantDef{constant};
  return Result<void, BuildError>::success();
}

Result<void, BuildError> FunctionBuilder::define_float_constant_bits(
    ValueId value, std::uint64_t exact_bits) {
  auto function = mutable_function();
  if (!function)
    return Result<void, BuildError>::failure(function.error());
  if (!same_owner(function_, value))
    return Result<void, BuildError>::failure(BuildError::ForeignOwner);
  auto resolved = function.value().get().values_.get_mut(function_, value);
  if (!resolved)
    return Result<void, BuildError>::failure(BuildError::InvalidValue);
  auto& definition = resolved.value().get();
  if (definition.kind != ValueKind::Ordinary || !floating_type(definition.type))
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if (!std::holds_alternative<UnresolvedDef>(definition.definition))
    return Result<void, BuildError>::failure(BuildError::ValueAlreadyDefined);
  if (parent_->data_->constants_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
  const ConstantId constant{
      parent_->data_->epoch_,
      static_cast<SlotIndex>(parent_->data_->constants_.size())};
  parent_->data_->constants_.push_back(
      ConstantDefinition{definition.type, FloatingConstant{exact_bits}});
  definition.definition = ConstantDef{constant};
  return Result<void, BuildError>::success();
}

Result<void, BuildError> FunctionBuilder::define_label_address_constant(
    ValueId value, BlockId target) {
  auto function = mutable_function();
  if (!function)
    return Result<void, BuildError>::failure(function.error());
  if (!same_owner(function_, value) || !same_owner(function_, target))
    return Result<void, BuildError>::failure(BuildError::ForeignOwner);
  auto resolved = function.value().get().values_.get_mut(function_, value);
  if (!resolved)
    return Result<void, BuildError>::failure(BuildError::InvalidValue);
  if (!function.value().get().blocks_.get(function_, target))
    return Result<void, BuildError>::failure(BuildError::InvalidBlock);
  auto& definition = resolved.value().get();
  if (definition.kind != ValueKind::Ordinary || definition.type.kind != TypeKind::Pointer)
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if (!std::holds_alternative<UnresolvedDef>(definition.definition))
    return Result<void, BuildError>::failure(BuildError::ValueAlreadyDefined);
  if (parent_->data_->constants_.size() >
      static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
  const ConstantId constant{parent_->data_->epoch_,
      static_cast<SlotIndex>(parent_->data_->constants_.size())};
  parent_->data_->constants_.push_back(
      ConstantDefinition{definition.type, LabelAddressConstant{target}});
  definition.definition = ConstantDef{constant};
  return Result<void, BuildError>::success();
}

Result<void, BuildError> FunctionBuilder::define_special_constant(
    ValueId value, SpecialConstantKind kind) {
  auto function = mutable_function();
  if (!function) return Result<void, BuildError>::failure(function.error());
  if (!same_owner(function_, value)) return Result<void, BuildError>::failure(BuildError::ForeignOwner);
  auto resolved = function.value().get().values_.get_mut(function_, value);
  if (!resolved) return Result<void, BuildError>::failure(BuildError::InvalidValue);
  auto& definition = resolved.value().get();
  if (definition.kind != ValueKind::Ordinary || !is_well_formed(definition.type) ||
      !std::holds_alternative<UnresolvedDef>(definition.definition))
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if ((kind == SpecialConstantKind::True || kind == SpecialConstantKind::False) &&
      definition.type != Type{TypeKind::I1, 1, "i1"})
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if (kind == SpecialConstantKind::Null && definition.type.kind != TypeKind::Pointer)
    return Result<void, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  if (parent_->data_->constants_.size() > static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max()))
    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
  const ConstantId constant{parent_->data_->epoch_, static_cast<SlotIndex>(parent_->data_->constants_.size())};
  parent_->data_->constants_.push_back(ConstantDefinition{definition.type, SpecialConstant{kind}});
  definition.definition = ConstantDef{constant};
  return Result<void, BuildError>::success();
}

Result<BlockId, BuildError> FunctionBuilder::create_block(
    std::string debug_name) {
  auto function = mutable_function();
  if (!function)
    return Result<BlockId, BuildError>::failure(function.error());
  auto& data = function.value().get();
  if (data.is_declaration_)
    return Result<BlockId, BuildError>::failure(
        BuildError::DeclarationHasNoBlocks);

  detail::BlockData block;
  block.debug_name_ = std::move(debug_name);
  auto inserted = data.blocks_.emplace(function_, std::move(block));
  if (!inserted)
    return Result<BlockId, BuildError>::failure(storage_error(inserted.error()));
  auto ordered = data.block_order_.append(inserted.value());
  if (!ordered)
    return Result<BlockId, BuildError>::failure(BuildError::StorageExhausted);
  return Result<BlockId, BuildError>::success(inserted.value());
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         InlineAsmSpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  if (!function.value().get().blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (spec.result_types.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()))
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  auto& function_data = function.value().get();
  if (spec.source_result_id &&
      (spec.result_types.size() != 1 ||
       *spec.source_result_id == std::numeric_limits<std::uint32_t>::max() ||
       function_data.values_by_source_id_.count(*spec.source_result_id) != 0))
    return Result<BuildResult, BuildError>::failure(
        spec.result_types.size() != 1 ? BuildError::InvalidValue
                                      : BuildError::DuplicateSourceValue);
  for (const auto input : spec.inputs) {
    if (!same_owner(function_, input))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    if (!function_data.values_.contains(function_, input))
      return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
  }
  for (const auto& type : spec.result_types)
    if (type.kind == TypeKind::Void)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);

  InlineAsmNode node{std::move(spec.asm_text),
                     std::move(spec.constraint_text),
                     std::move(spec.clobbers), spec.side_effects};
  detail::InstData instruction;
  instruction.opcode = Opcode::InlineAsm;
  instruction.payload = std::move(node);
  instruction.operands = std::move(spec.inputs);
  auto inserted =
      function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted)
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted.error()));
  const auto instruction_id = inserted.value();

  std::vector<ValueId> results;
  results.reserve(spec.result_types.size());
  for (std::size_t index = 0; index < spec.result_types.size(); ++index) {
    ValueDef value;
    value.kind = ValueKind::Ordinary;
    value.type = spec.result_types[index];
    value.definition =
        InstResultDef{instruction_id, static_cast<std::uint16_t>(index)};
    if (spec.source_result_id)
      value.source_id = SourceValueId{function_, *spec.source_result_id};
    auto inserted_value =
        function_data.values_.emplace(function_, std::move(value));
    if (!inserted_value) {
      for (const auto result : results)
        function_data.values_.erase(function_, result);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(
          storage_error(inserted_value.error()));
    }
    results.push_back(inserted_value.value());
  }
  if (spec.source_result_id)
    function_data.values_by_source_id_.emplace(*spec.source_result_id,
                                               results.front());
  auto stored_instruction =
      function_data.insts_.get_mut(function_, instruction_id);
  if (!stored_instruction) {
    if (spec.source_result_id)
      function_data.values_by_source_id_.erase(*spec.source_result_id);
    for (const auto result : results)
      function_data.values_.erase(function_, result);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored_instruction.value().get().results = results;

  for (const auto result : results) {
    if (!function_data.value_order_.append(result)) {
      for (const auto rollback : results)
        function_data.value_order_.erase(rollback);
      for (const auto rollback : results)
        function_data.values_.erase(function_, rollback);
      if (spec.source_result_id)
        function_data.values_by_source_id_.erase(*spec.source_result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
    }
  }

  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    for (const auto result : results)
      function_data.value_order_.erase(result);
    for (const auto result : results)
      function_data.values_.erase(function_, result);
    if (spec.source_result_id)
      function_data.values_by_source_id_.erase(*spec.source_result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, std::move(results)});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         StoreSpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !same_owner(function_, spec.value))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto value = function_data.values_.get(function_, spec.value);
  if (!value)
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
  if (!spec.destination.valid() ||
      spec.destination.epoch != parent_->data_->epoch_ ||
      spec.destination.slot >= parent_->data_->globals_.size())
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidGlobalObject);
  const auto& global = parent_->data_->globals_[spec.destination.slot];
  if (!is_well_formed(spec.stored_type) ||
      !integer_type(spec.stored_type) ||
      value.value().get().type != spec.stored_type ||
      global.object_type != spec.stored_type)
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidValueType);

  detail::InstData instruction;
  instruction.opcode = Opcode::Store;
  instruction.payload =
      StoreNode{spec.destination, std::move(spec.stored_type)};
  instruction.operands = {spec.value};
  auto inserted =
      function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted)
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, {}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         LoadSpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (spec.source_result_id == std::numeric_limits<std::uint32_t>::max())
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidSourceValueId);
  if (function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(
        BuildError::DuplicateSourceValue);
  if (!spec.source.valid() || spec.source.epoch != parent_->data_->epoch_ ||
      spec.source.slot >= parent_->data_->globals_.size())
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidGlobalObject);
  const auto& global = parent_->data_->globals_[spec.source.slot];
  if (!is_well_formed(spec.loaded_type) || !integer_type(spec.loaded_type) ||
      global.object_type != spec.loaded_type)
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidValueType);

  detail::InstData instruction;
  instruction.opcode = Opcode::Load;
  instruction.payload = LoadNode{spec.source, spec.loaded_type};
  auto inserted_instruction =
      function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted_instruction)
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted_instruction.error()));
  const auto instruction_id = inserted_instruction.value();

  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = std::move(spec.loaded_type);
  value.source_id = SourceValueId{function_, spec.source_result_id};
  value.definition = InstResultDef{instruction_id, 0};
  auto inserted_value =
      function_data.values_.emplace(function_, std::move(value));
  if (!inserted_value) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted_value.error()));
  }
  const auto result_id = inserted_value.value();
  auto stored_instruction =
      function_data.insts_.get_mut(function_, instruction_id);
  if (!stored_instruction) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored_instruction.value().get().results = {result_id};

  if (!function_data.value_order_.append(result_id)) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  try {
    const auto indexed = function_data.values_by_source_id_.emplace(
        spec.source_result_id, result_id);
    if (!indexed.second) {
      function_data.value_order_.erase(result_id);
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(
          BuildError::DuplicateSourceValue);
    }
  } catch (...) {
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    throw;
  }

  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, LocalLoadAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.result.valid() ||
      !spec.pointer_definition.valid() || !spec.object.valid() ||
      spec.result.owner != function_ || spec.pointer_definition.owner != function_ ||
      spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} ||
      !is_well_formed(spec.loaded_type) ||
      (!integer_type(spec.loaded_type) && !floating_type(spec.loaded_type)) ||
      !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (data.values_by_source_id_.count(spec.result.value) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
  const auto pointer = data.values_by_source_id_.find(spec.pointer_definition.value);
  if (pointer == data.values_by_source_id_.end())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  const auto pointer_def = data.values_.get(function_, pointer->second);
  if (!pointer_def || pointer_def.value().get().type != spec.pointer_type)
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  const auto* pointer_result = std::get_if<InstResultDef>(&pointer_def.value().get().definition);
  const auto pointer_inst = pointer_result
      ? data.insts_.get(function_, pointer_result->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* alloca = pointer_inst
      ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload)
      : nullptr;
  if (!alloca || alloca->result != spec.pointer_definition ||
      alloca->pointer_definition != spec.pointer_definition ||
      alloca->object.owner != spec.object.owner || alloca->object.value != spec.object.value ||
      alloca->owner != spec.owner ||
      alloca->pointer_type != spec.pointer_type ||
      alloca->pointee_type != spec.loaded_type || !alloca->live)
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);

  detail::InstData instruction;
  instruction.opcode = Opcode::Load;
  instruction.payload = LocalLoadAuthorityNode{spec.result, spec.pointer_definition,
      spec.object, spec.owner, spec.pointer_type, spec.loaded_type, spec.live};
  instruction.operands = {pointer->second};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = spec.loaded_type;
  value.source_id = spec.result;
  value.definition = InstResultDef{instruction_id, 0};
  auto inserted_value = data.values_.emplace(function_, std::move(value));
  if (!inserted_value) {
    data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_value.error()));
  }
  const auto result_id = inserted_value.value();
  auto stored = data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  if (!data.value_order_.append(result_id)) {
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  try {
    const auto indexed = data.values_by_source_id_.emplace(spec.result.value, result_id);
    if (!indexed.second) {
      data.value_order_.erase(result_id); data.values_.erase(function_, result_id);
      data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
    }
  } catch (...) {
    data.value_order_.erase(result_id); data.values_.erase(function_, result_id);
    data.insts_.erase(function_, instruction_id);
    throw;
  }
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    data.values_by_source_id_.erase(spec.result.value); data.value_order_.erase(result_id);
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, LocalStoreAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.pointer_definition.valid() || !spec.object.valid() ||
      spec.pointer_definition.owner != function_ || spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ || spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} || !is_well_formed(spec.stored_type) ||
      !integer_type(spec.stored_type) || !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block)) return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto pointer = data.values_by_source_id_.find(spec.pointer_definition.value);
  if (pointer == data.values_by_source_id_.end()) return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  const auto pointer_def = data.values_.get(function_, pointer->second);
  const auto* result = pointer_def ? std::get_if<InstResultDef>(&pointer_def.value().get().definition) : nullptr;
  const auto pointer_inst = result ? data.insts_.get(function_, result->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* alloca = pointer_inst ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload) : nullptr;
  if (!pointer_def || pointer_def.value().get().type != spec.pointer_type || !alloca ||
      alloca->pointer_definition != spec.pointer_definition || alloca->object.owner != spec.object.owner ||
      alloca->object.value != spec.object.value || alloca->owner != spec.owner ||
      alloca->pointee_type != spec.stored_type || !alloca->live)
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  detail::InstData instruction;
  instruction.opcode = Opcode::Store;
  instruction.payload = LocalStoreAuthorityNode{spec.pointer_definition, spec.object, spec.owner,
      spec.pointer_type, spec.stored_type, spec.immediate, spec.live};
  instruction.operands = {pointer->second};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto id = inserted.value();
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(id)) {
    data.insts_.erase(function_, id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{id, {}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, GetElementPtrSpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (spec.source_result_id == std::numeric_limits<std::uint32_t>::max())
    return Result<BuildResult, BuildError>::failure(
        BuildError::InvalidSourceValueId);
  if (function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(
        BuildError::DuplicateSourceValue);
  const bool direct_body_parameter =
      std::holds_alternative<DirectPointerBodyParameterGepBase>(spec.base.authority);
  if (!is_well_formed(spec.element_type) ||
      (!direct_body_parameter && spec.element_type.kind != TypeKind::Array))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidValueType);
  if (const auto* global =
          std::get_if<GlobalObjectId>(&spec.base.authority)) {
    if (!global->valid() || global->epoch != parent_->data_->epoch_ ||
        global->slot >= parent_->data_->globals_.size())
      return Result<BuildResult, BuildError>::failure(
          BuildError::InvalidGlobalObject);
    if (parent_->data_->globals_[global->slot].object_type !=
        spec.element_type)
      return Result<BuildResult, BuildError>::failure(
          BuildError::InvalidValueType);
  } else if (const auto* label =
                 std::get_if<LabelAddressGepBase>(&spec.base.authority)) {
    if (!same_owner(function_, label->value))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    const auto value = function_data.values_.get(function_, label->value);
    if (!value)
      return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
    const auto& definition = value.value().get();
    if (definition.kind != ValueKind::Ordinary ||
        definition.type.kind != TypeKind::Pointer)
      return Result<BuildResult, BuildError>::failure(
          BuildError::DefinitionTypeMismatch);
    const auto* constant = std::get_if<ConstantDef>(&definition.definition);
    if (!constant || !constant->constant.valid() ||
        constant->constant.epoch != parent_->data_->epoch_ ||
        constant->constant.slot >= parent_->data_->constants_.size())
      return Result<BuildResult, BuildError>::failure(
          BuildError::DefinitionTypeMismatch);
    const auto* label_constant = std::get_if<LabelAddressConstant>(
        &parent_->data_->constants_[constant->constant.slot].payload);
    if (!label_constant)
      return Result<BuildResult, BuildError>::failure(
          BuildError::DefinitionTypeMismatch);
    if (!same_owner(function_, label_constant->target))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    if (!function_data.blocks_.contains(function_, label_constant->target))
      return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  } else if (const auto* parameter =
                 std::get_if<DirectPointerBodyParameterGepBase>(&spec.base.authority)) {
    if (parameter->source_value_id == 0 || !parameter->owner.valid() ||
        parameter->owner.epoch != parent_->data_->epoch_ ||
        parameter->owner.slot >= parent_->data_->link_names_.size() ||
        parameter->pointer_type != Type{TypeKind::Pointer} ||
        parameter->parameter_index >= function_data.parameters_.size())
      return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
    const auto parameter_value = function_data.values_.get(
        function_, function_data.parameters_[parameter->parameter_index]);
    if (!parameter_value || parameter_value.value().get().kind != ValueKind::Parameter ||
        parameter_value.value().get().type != parameter->pointer_type)
      return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  } else {
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (spec.indices.empty())
    return Result<BuildResult, BuildError>::failure(
        BuildError::UnsupportedOpcode);
  for (const auto index : spec.indices) {
    if (!same_owner(function_, index))
      return Result<BuildResult, BuildError>::failure(
          BuildError::ForeignOwner);
    const auto value = function_data.values_.get(function_, index);
    if (!value)
      return Result<BuildResult, BuildError>::failure(
          BuildError::InvalidValue);
    if (!integer_type(value.value().get().type))
      return Result<BuildResult, BuildError>::failure(
          BuildError::InvalidValueType);
  }

  detail::InstData instruction;
  instruction.opcode = Opcode::GetElementPtr;
  instruction.payload = GetElementPtrNode{
      spec.base, spec.element_type, spec.inbounds};
  instruction.operands = std::move(spec.indices);
  auto inserted_instruction =
      function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted_instruction)
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted_instruction.error()));
  const auto instruction_id = inserted_instruction.value();

  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = Type{TypeKind::Pointer};
  value.source_id = SourceValueId{function_, spec.source_result_id};
  value.definition = InstResultDef{instruction_id, 0};
  auto inserted_value =
      function_data.values_.emplace(function_, std::move(value));
  if (!inserted_value) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted_value.error()));
  }
  const auto result_id = inserted_value.value();
  auto stored_instruction =
      function_data.insts_.get_mut(function_, instruction_id);
  if (!stored_instruction) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored_instruction.value().get().results = {result_id};

  if (!function_data.value_order_.append(result_id)) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  try {
    const auto indexed = function_data.values_by_source_id_.emplace(
        spec.source_result_id, result_id);
    if (!indexed.second) {
      function_data.value_order_.erase(result_id);
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(
          BuildError::DuplicateSourceValue);
    }
  } catch (...) {
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    throw;
  }

  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, LocalArrayGepAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.result.valid() ||
      !spec.pointer_definition.valid() || !spec.object.valid() ||
      spec.result.owner != function_ || spec.pointer_definition.owner != function_ ||
      spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} ||
      !is_well_formed(spec.pointee_type) || spec.pointee_type.kind != TypeKind::Array ||
      !is_well_formed(spec.element_type) || spec.element_type.kind == TypeKind::Array ||
      !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (data.values_by_source_id_.count(spec.result.value) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
  const auto pointer = data.values_by_source_id_.find(spec.pointer_definition.value);
  if (pointer == data.values_by_source_id_.end())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  const auto pointer_def = data.values_.get(function_, pointer->second);
  const auto* pointer_result = pointer_def
      ? std::get_if<InstResultDef>(&pointer_def.value().get().definition) : nullptr;
  const auto pointer_inst = pointer_result
      ? data.insts_.get(function_, pointer_result->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* alloca = pointer_inst
      ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload) : nullptr;
  if (!pointer_def || pointer_def.value().get().type != spec.pointer_type || !alloca ||
      alloca->result != spec.pointer_definition || alloca->pointer_definition != spec.pointer_definition ||
      alloca->object.owner != spec.object.owner || alloca->object.value != spec.object.value ||
      alloca->owner != spec.owner || alloca->pointer_type != spec.pointer_type ||
      alloca->pointee_type != spec.pointee_type || !alloca->live)
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  detail::InstData instruction;
  instruction.opcode = Opcode::GetElementPtr;
  instruction.payload = LocalArrayGepAuthorityNode{spec.result, spec.pointer_definition,
      spec.object, spec.owner, spec.pointer_type, spec.pointee_type,
      spec.element_type, spec.immediate_index, spec.live};
  instruction.operands = {pointer->second};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = Type{TypeKind::Pointer};
  value.source_id = spec.result;
  value.definition = InstResultDef{instruction_id, 0};
  auto inserted_value = data.values_.emplace(function_, std::move(value));
  if (!inserted_value) { data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(storage_error(inserted_value.error())); }
  const auto result_id = inserted_value.value();
  auto stored = data.insts_.get_mut(function_, instruction_id);
  if (!stored) { data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted); }
  stored.value().get().results = {result_id};
  if (!data.value_order_.append(result_id)) { data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted); }
  try {
    const auto indexed = data.values_by_source_id_.emplace(spec.result.value, result_id);
    if (!indexed.second) { data.value_order_.erase(result_id); data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue); }
  } catch (...) { data.value_order_.erase(result_id); data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id); throw; }
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    data.values_by_source_id_.erase(spec.result.value); data.value_order_.erase(result_id);
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         AbsSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !same_owner(function_, spec.operand))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  const Type i32{TypeKind::Integer, 32, "i32"};
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto operand = function_data.values_.get(function_, spec.operand);
  const auto* operand_def = operand
      ? std::get_if<InstResultDef>(&operand.value().get().definition) : nullptr;
  const auto producer = operand_def
      ? function_data.insts_.get(function_, operand_def->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(
            ResolveError::OutOfRange);
  if (spec.type != i32 || !operand || operand.value().get().type != i32 ||
      !producer || !std::holds_alternative<LoadNode>(producer.value().get().payload) ||
      function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  detail::InstData instruction;
  instruction.opcode = Opcode::Abs;
  instruction.payload = AbsNode{spec.type};
  instruction.operands = {spec.operand};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result{ValueKind::Ordinary, spec.type,
                  SourceValueId{function_, spec.source_result_id},
                  InstResultDef{instruction_id, 0}};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error()));
  }
  const auto result_id = inserted_result.value();
  auto stored = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  const auto indexed = function_data.values_by_source_id_.emplace(spec.source_result_id, result_id);
  if (!indexed.second || !function_data.value_order_.append(result_id)) {
    if (indexed.second) function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(indexed.second
        ? BuildError::StorageExhausted : BuildError::DuplicateSourceValue);
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         CallSpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto callee = parent_->data_->functions_.get(parent_->data_->epoch_,
                                                     spec.callee);
  if (!callee)
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidFunction);
  const auto& signature = callee.value().get().signature_;
  if (signature.is_variadic ||
      (signature.return_type.kind == TypeKind::Void) !=
          !spec.source_result_id ||
      (spec.source_result_id && !integer_type(signature.return_type) &&
       signature.return_type.kind != TypeKind::F32 &&
       signature.return_type.kind != TypeKind::F64) ||
      signature.parameter_types.size() != spec.arguments.size() ||
      (!spec.source_result_id && !signature.parameter_types.empty()))
    return Result<BuildResult, BuildError>::failure(
        BuildError::UnsupportedOpcode);
  for (std::size_t index = 0; index < spec.arguments.size(); ++index) {
    const auto argument = function_data.values_.get(function_, spec.arguments[index]);
    if (!argument)
      return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
    if (argument.value().get().type != signature.parameter_types[index])
      return Result<BuildResult, BuildError>::failure(
          BuildError::DefinitionTypeMismatch);
  }
  if (spec.direct_scalar_argument) {
    const auto& authority = *spec.direct_scalar_argument;
    if (authority.argument_index >= spec.arguments.size() ||
        authority.argument_index >= signature.parameter_types.size() ||
        authority.source_value_id == 0 ||
        !authority.owner.valid() || authority.owner.epoch != parent_->data_->epoch_ ||
        authority.owner.slot >= parent_->data_->link_names_.size() ||
        parent_->data_->link_names_[authority.owner.slot].spelling != function_data.link_name_ ||
        authority.parameter_index >= function_data.parameters_.size() ||
        function_data.parameters_[authority.parameter_index] !=
            spec.arguments[authority.argument_index] ||
        authority.scalar_type != signature.parameter_types[authority.argument_index])
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (spec.source_result_id &&
      function_data.values_by_source_id_.count(*spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);

  detail::InstData instruction;
  instruction.opcode = Opcode::Call;
  instruction.payload = CallNode{spec.callee, spec.direct_scalar_argument};
  instruction.operands = std::move(spec.arguments);
  auto inserted =
      function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted)
    return Result<BuildResult, BuildError>::failure(
        storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  std::optional<ValueId> result_id;
  if (spec.source_result_id) {
    ValueDef result;
    result.kind = ValueKind::Ordinary;
    result.type = signature.return_type;
    result.source_id = SourceValueId{function_, *spec.source_result_id};
    result.definition = InstResultDef{instruction_id, 0};
    auto inserted_result = function_data.values_.emplace(function_, std::move(result));
    if (!inserted_result) {
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(
          storage_error(inserted_result.error()));
    }
    result_id = inserted_result.value();
    auto stored_instruction = function_data.insts_.get_mut(function_, instruction_id);
    if (!stored_instruction) {
      function_data.values_.erase(function_, *result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
    }
    stored_instruction.value().get().results = {*result_id};
    if (!function_data.value_order_.append(*result_id)) {
      function_data.values_.erase(function_, *result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
    }
    try {
      const auto indexed = function_data.values_by_source_id_.emplace(
          *spec.source_result_id, *result_id);
      if (!indexed.second) {
        function_data.value_order_.erase(*result_id);
        function_data.values_.erase(function_, *result_id);
        function_data.insts_.erase(function_, instruction_id);
        return Result<BuildResult, BuildError>::failure(
            BuildError::DuplicateSourceValue);
      }
    } catch (...) {
      function_data.value_order_.erase(*result_id);
      function_data.values_.erase(function_, *result_id);
      function_data.insts_.erase(function_, instruction_id);
      throw;
    }
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    if (result_id) {
      function_data.values_by_source_id_.erase(*spec.source_result_id);
      function_data.value_order_.erase(*result_id);
      function_data.values_.erase(function_, *result_id);
    }
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, result_id ? std::vector<ValueId>{*result_id}
                                             : std::vector<ValueId>{}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         BinarySpec spec) {
  auto function = mutable_function();
  if (!function)
    return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !same_owner(function_, spec.lhs) ||
      (spec.opcode != BinaryOpcode::FNeg && !same_owner(function_, spec.rhs)))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const Type f64{TypeKind::F64, 64, "double"};
  const Type f32{TypeKind::F32, 32, "float"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type i64{TypeKind::Integer, 64, "i64"};
  const auto lhs = function_data.values_.get(function_, spec.lhs);
  const auto rhs = function_data.values_.get(function_, spec.rhs);
  const bool exact_fadd = spec.opcode == BinaryOpcode::FAdd && spec.type == f64 &&
      lhs && rhs && lhs.value().get().type == f64 && rhs.value().get().type == f64;
  const bool exact_fmul = spec.opcode == BinaryOpcode::FMul && spec.type == f64 &&
      lhs && rhs && lhs.value().get().type == f64 && rhs.value().get().type == f64;
  const bool exact_float_fmul = spec.opcode == BinaryOpcode::FMul && spec.type == f32 &&
      lhs && rhs && lhs.value().get().type == f32 && rhs.value().get().type == f32;
  const bool exact_add = spec.opcode == BinaryOpcode::Add && spec.type == i32 &&
      lhs && rhs && lhs.value().get().type == i32 && rhs.value().get().type == i32;
  const auto* direct_scalar = spec.direct_scalar_lhs ? &*spec.direct_scalar_lhs : nullptr;
  const auto* direct_scalar_rhs = spec.direct_scalar_rhs ? &*spec.direct_scalar_rhs : nullptr;
  const bool exact_direct_scalar_add = exact_add && direct_scalar &&
      direct_scalar->source_value_id != 0 && direct_scalar->scalar_type == i32 &&
      direct_scalar->owner.valid() && direct_scalar->owner.epoch == parent_->data_->epoch_ &&
      direct_scalar->owner.slot < parent_->data_->link_names_.size() &&
      parent_->data_->link_names_[direct_scalar->owner.slot].spelling ==
          function_data.link_name_ &&
      direct_scalar->parameter_index < function_data.parameters_.size() &&
      function_data.parameters_[direct_scalar->parameter_index] == spec.lhs;
  const bool exact_direct_scalar_fneg = spec.opcode == BinaryOpcode::FNeg &&
      lhs && !spec.rhs.valid() && direct_scalar &&
      direct_scalar->source_value_id != 0 && direct_scalar->scalar_type == spec.type &&
      direct_scalar->owner.valid() &&
      direct_scalar->owner.epoch == parent_->data_->epoch_ &&
      direct_scalar->owner.slot < parent_->data_->link_names_.size() &&
      parent_->data_->link_names_[direct_scalar->owner.slot].spelling ==
          function_data.link_name_ &&
      direct_scalar->parameter_index < function_data.parameters_.size() &&
      function_data.parameters_[direct_scalar->parameter_index] == spec.lhs;
  const bool exact_direct_scalar_fmul = spec.opcode == BinaryOpcode::FMul &&
      floating_type(spec.type) && lhs && rhs &&
      lhs.value().get().type == spec.type && rhs.value().get().type == spec.type &&
      direct_scalar && direct_scalar->source_value_id != 0 &&
      direct_scalar->scalar_type == spec.type &&
      direct_scalar->owner.valid() &&
      direct_scalar->owner.epoch == parent_->data_->epoch_ &&
      direct_scalar->owner.slot < parent_->data_->link_names_.size() &&
      parent_->data_->link_names_[direct_scalar->owner.slot].spelling ==
          function_data.link_name_ &&
      direct_scalar->parameter_index < function_data.parameters_.size() &&
      function_data.parameters_[direct_scalar->parameter_index] == spec.lhs;
  const bool exact_direct_scalar_rhs_add = exact_add && direct_scalar_rhs &&
      direct_scalar_rhs->source_value_id != 0 && direct_scalar_rhs->scalar_type == i32 &&
      direct_scalar_rhs->owner.valid() && direct_scalar_rhs->owner.epoch == parent_->data_->epoch_ &&
      direct_scalar_rhs->owner.slot < parent_->data_->link_names_.size() &&
      parent_->data_->link_names_[direct_scalar_rhs->owner.slot].spelling == function_data.link_name_ &&
      direct_scalar_rhs->parameter_index < function_data.parameters_.size() &&
      function_data.parameters_[direct_scalar_rhs->parameter_index] == spec.rhs;
  const bool exact_direct_scalar_rhs_fmul = spec.opcode == BinaryOpcode::FMul &&
      floating_type(spec.type) && lhs && rhs &&
      lhs.value().get().type == spec.type && rhs.value().get().type == spec.type &&
      direct_scalar_rhs && direct_scalar_rhs->source_value_id != 0 &&
      direct_scalar_rhs->scalar_type == spec.type &&
      direct_scalar_rhs->owner.valid() &&
      direct_scalar_rhs->owner.epoch == parent_->data_->epoch_ &&
      direct_scalar_rhs->owner.slot < parent_->data_->link_names_.size() &&
      parent_->data_->link_names_[direct_scalar_rhs->owner.slot].spelling ==
          function_data.link_name_ &&
      direct_scalar_rhs->parameter_index < function_data.parameters_.size() &&
      function_data.parameters_[direct_scalar_rhs->parameter_index] == spec.rhs;
  const bool exact_sext_add = spec.opcode == BinaryOpcode::Add && spec.type == i64 &&
      lhs && rhs && lhs.value().get().type == i64 && rhs.value().get().type == i64;
  const bool exact_mul = spec.opcode == BinaryOpcode::Mul && spec.type == i32 &&
      lhs && rhs && lhs.value().get().type == i32 && rhs.value().get().type == i32;
  const auto* lhs_def = lhs ? std::get_if<InstResultDef>(&lhs.value().get().definition)
                            : nullptr;
  const auto lhs_producer = lhs_def
      ? function_data.insts_.get(function_, lhs_def->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(
            ResolveError::OutOfRange);
  const bool exact_cttz_add = exact_add && lhs_producer && [&] {
    const auto* intrinsic = std::get_if<IntrinsicCallNode>(&lhs_producer.value().get().payload);
    return intrinsic && intrinsic->kind == IntrinsicKind::Cttz && intrinsic->type == i32;
  }();
  const bool exact_ctlz_add = exact_add && lhs_producer && [&] {
    const auto* intrinsic = std::get_if<IntrinsicCallNode>(&lhs_producer.value().get().payload);
    return intrinsic && intrinsic->kind == IntrinsicKind::Ctlz && intrinsic->type == i32 &&
        intrinsic->zero_count_is_undef == std::optional<bool>{true};
  }();
  const bool exact_ctpop_add = exact_add && lhs_producer && [&] {
    const auto* intrinsic = std::get_if<IntrinsicCallNode>(&lhs_producer.value().get().payload);
    return intrinsic && intrinsic->kind == IntrinsicKind::Ctpop && intrinsic->type == i32 &&
        !intrinsic->zero_count_is_undef.has_value();
  }();
  const bool exact_fptosi_add = exact_add && lhs_producer && [&] {
    const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
    return cast && cast->kind == CastKind::FPToSI && cast->from_type == f64 &&
        cast->to_type == i32;
  }();
  const bool exact_fptoui_add = exact_add && lhs_producer && [&] {
    const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
    return cast && cast->kind == CastKind::FPToUI && cast->from_type == f64 &&
        cast->to_type == i32;
  }();
  const bool exact_wide_ffs_trunc_add = exact_add && lhs_producer && [&] {
    const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
    return cast && cast->kind == CastKind::Trunc &&
        cast->from_type == i64 && cast->to_type == i32;
  }();
  const bool exact_ffs_add = (exact_add || exact_sext_add) && lhs_producer && [&] {
    const auto* intrinsic = std::get_if<IntrinsicCallNode>(&lhs_producer.value().get().payload);
    return intrinsic && intrinsic->kind == IntrinsicKind::Cttz && intrinsic->type == spec.type;
  }();
  if ((!exact_fadd && !exact_fmul && !exact_float_fmul &&
       !exact_add && !exact_sext_add && !exact_mul &&
       !exact_direct_scalar_add && !exact_direct_scalar_fneg &&
       !exact_direct_scalar_fmul && !exact_direct_scalar_rhs_add &&
       !exact_direct_scalar_rhs_fmul) ||
      function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (!exact_direct_scalar_add && !exact_direct_scalar_fneg &&
      !exact_direct_scalar_fmul && !exact_direct_scalar_rhs_add &&
      !exact_direct_scalar_rhs_fmul && !lhs_def)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (!exact_direct_scalar_add && !exact_direct_scalar_fneg &&
      !exact_direct_scalar_fmul && !exact_direct_scalar_rhs_add &&
      !exact_direct_scalar_rhs_fmul && (!lhs_producer ||
      (exact_fadd && !std::holds_alternative<CallNode>(lhs_producer.value().get().payload)) ||
      (exact_fmul && [&] {
        const auto* binary = std::get_if<BinaryNode>(&lhs_producer.value().get().payload);
        if (const auto* binary = std::get_if<BinaryNode>(&lhs_producer.value().get().payload))
          return binary->opcode != BinaryOpcode::FAdd || binary->type != f64;
        const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
        return !cast || ((cast->kind != CastKind::FPExt || cast->from_type != f32 ||
                          cast->to_type != f64) &&
                         (cast->kind != CastKind::SIToFP || cast->from_type != i32 ||
                          cast->to_type != f64) &&
                         (cast->kind != CastKind::UIToFP || cast->from_type != i32 ||
                          cast->to_type != f64));
      }()) ||
      (exact_float_fmul && [&] {
        const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
        return !cast || cast->kind != CastKind::FPTrunc || cast->from_type != f64 ||
            cast->to_type != f32;
      }()) ||
      (exact_add && !std::holds_alternative<LoadNode>(lhs_producer.value().get().payload) &&
       !std::holds_alternative<AbsNode>(lhs_producer.value().get().payload) &&
       !exact_cttz_add && !exact_ctlz_add && !exact_ctpop_add && !exact_fptosi_add && !exact_fptoui_add && !exact_wide_ffs_trunc_add && !exact_ffs_add) ||
      (exact_sext_add && [&] {
        const auto* cast = std::get_if<CastNode>(&lhs_producer.value().get().payload);
        return !exact_ffs_add && (!cast || cast->kind != CastKind::SExt || cast->from_type != i32 ||
            cast->to_type != i64);
      }()) ||
      (exact_mul && [&] {
        const auto* binary = std::get_if<BinaryNode>(&lhs_producer.value().get().payload);
        return !binary || binary->opcode != BinaryOpcode::Add || binary->type != i32;
      }())))
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if ((exact_add || exact_sext_add || exact_mul) && !exact_direct_scalar_rhs_add) {
    const auto* rhs_constant = std::get_if<ConstantDef>(&rhs.value().get().definition);
    if (!rhs_constant) return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
    const auto* integer = rhs_constant->constant.valid() &&
            rhs_constant->constant.epoch == parent_->data_->epoch_ &&
            rhs_constant->constant.slot < parent_->data_->constants_.size()
        ? std::get_if<IntegerConstant>(
              &parent_->data_->constants_[rhs_constant->constant.slot].payload)
        : nullptr;
    if (!integer || integer->value != ((exact_add || exact_sext_add) ? 1 : 2))
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }

  detail::InstData instruction;
  instruction.opcode = Opcode::Binary;
  instruction.payload = BinaryNode{spec.opcode, spec.type, spec.direct_scalar_lhs,
                                   spec.direct_scalar_rhs};
  instruction.operands = spec.opcode == BinaryOpcode::FNeg
      ? std::vector<ValueId>{spec.lhs}
      : std::vector<ValueId>{spec.lhs, spec.rhs};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted)
    return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result{ValueKind::Ordinary, spec.type,
                  SourceValueId{function_, spec.source_result_id},
                  InstResultDef{instruction_id, 0}};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error()));
  }
  const auto result_id = inserted_result.value();
  auto stored = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  try {
    const auto indexed = function_data.values_by_source_id_.emplace(
        spec.source_result_id, result_id);
    if (!indexed.second) {
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
    }
    if (!function_data.value_order_.append(result_id)) {
      function_data.values_by_source_id_.erase(spec.source_result_id);
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
    }
  } catch (...) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    throw;
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data ||
      !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                         CompareSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !same_owner(function_, spec.lhs) ||
      !same_owner(function_, spec.rhs))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  const Type i1{TypeKind::I1, 1, "i1"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type i64{TypeKind::Integer, 64, "i64"};
  const Type f64{TypeKind::F64, 64, "double"};
  if (spec.source_result_id == std::numeric_limits<std::uint32_t>::max())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  const auto lhs = function_data.values_.get(function_, spec.lhs);
  const auto rhs = function_data.values_.get(function_, spec.rhs);
  const auto* lhs_def = lhs ? std::get_if<InstResultDef>(&lhs.value().get().definition) : nullptr;
  const auto lhs_producer = lhs_def
      ? function_data.insts_.get(function_, lhs_def->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* rhs_def = rhs ? std::get_if<ConstantDef>(&rhs.value().get().definition) : nullptr;
  const auto* integer = rhs_def && rhs_def->constant.valid() &&
          rhs_def->constant.epoch == parent_->data_->epoch_ &&
          rhs_def->constant.slot < parent_->data_->constants_.size()
      ? std::get_if<IntegerConstant>(&parent_->data_->constants_[rhs_def->constant.slot].payload)
      : nullptr;
  const bool slt = spec.predicate == ComparePredicate::Slt && spec.type == i32 &&
      lhs && rhs && lhs.value().get().type == i32 && rhs.value().get().type == i32 &&
      lhs_producer && std::holds_alternative<LoadNode>(lhs_producer.value().get().payload) &&
      integer && integer->value == 7;
  const bool olt = spec.predicate == ComparePredicate::OLt && spec.type == f64 &&
      lhs && rhs && lhs.value().get().type == f64 && rhs.value().get().type == f64 &&
      lhs_producer && [&] {
        const auto* binary = std::get_if<BinaryNode>(&lhs_producer.value().get().payload);
        return binary && binary->opcode == BinaryOpcode::FMul && binary->type == f64;
      }();
  const bool ffs_eq_zero = spec.predicate == ComparePredicate::Eq &&
      (spec.type == i32 || spec.type == i64) && lhs && rhs &&
      lhs.value().get().type == spec.type && rhs.value().get().type == spec.type &&
      integer && integer->value == 0;
  const auto* lhs_parameter = lhs ? std::get_if<ParameterDef>(&lhs.value().get().definition) : nullptr;
  const auto* lhs_inst = lhs ? std::get_if<InstResultDef>(&lhs.value().get().definition) : nullptr;
  const auto lhs_cast_producer = lhs_inst
      ? function_data.insts_.get(function_, lhs_inst->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* lhs_cast = lhs_cast_producer
      ? std::get_if<CastNode>(&lhs_cast_producer.value().get().payload)
      : nullptr;
  const auto* ptrtoint_operand = lhs_cast && lhs_cast->kind == CastKind::PtrToInt &&
          lhs_cast_producer.value().get().operands.size() == 1
      ? &lhs_cast_producer.value().get().operands[0]
      : nullptr;
  const auto ptrtoint_source = ptrtoint_operand
      ? function_data.values_.get(function_, *ptrtoint_operand)
      : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* ptrtoint_parameter = ptrtoint_source
      ? std::get_if<ParameterDef>(&ptrtoint_source.value().get().definition)
      : nullptr;
  const bool truthiness_ne = spec.predicate == ComparePredicate::Ne && integer_type(spec.type) &&
      lhs && rhs && lhs.value().get().type == spec.type && rhs.value().get().type == spec.type &&
      lhs_parameter && integer && integer->value == 0 && spec.direct_scalar_truthiness_lhs &&
      spec.direct_scalar_truthiness_lhs->source_value_id != 0 &&
      spec.direct_scalar_truthiness_lhs->parameter_index == lhs_parameter->ordinal &&
      spec.direct_scalar_truthiness_lhs->scalar_type == spec.type &&
      spec.direct_scalar_truthiness_lhs->owner.valid();
  const bool pointer_truthiness_ne = spec.predicate == ComparePredicate::Ne &&
      spec.type == i64 && lhs && rhs && lhs.value().get().type == i64 &&
      rhs.value().get().type == i64 && integer && integer->value == 0 &&
      spec.direct_pointer_truthiness && lhs_cast && lhs_cast->kind == CastKind::PtrToInt &&
      lhs_cast->from_type == Type{TypeKind::Pointer} && lhs_cast->to_type == i64 &&
      ptrtoint_parameter &&
      spec.direct_pointer_truthiness->source_value_id != 0 &&
      spec.direct_pointer_truthiness->parameter_index == ptrtoint_parameter->ordinal &&
      spec.direct_pointer_truthiness->pointer_type == Type{TypeKind::Pointer} &&
      spec.direct_pointer_truthiness->owner.valid();
  if (!function_data.blocks_.contains(function_, block) || (!slt && !olt && !ffs_eq_zero && !truthiness_ne && !pointer_truthiness_ne) ||
      function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  detail::InstData instruction;
  instruction.opcode = Opcode::Compare;
  instruction.payload = CompareNode{spec.predicate, spec.type,
                                    spec.direct_scalar_truthiness_lhs,
                                    spec.direct_pointer_truthiness};
  instruction.operands = {spec.lhs, spec.rhs};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result{ValueKind::Ordinary, i1, SourceValueId{function_, spec.source_result_id},
                  InstResultDef{instruction_id, 0}};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error()));
  }
  const auto result_id = inserted_result.value();
  auto stored = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  const auto indexed = function_data.values_by_source_id_.emplace(spec.source_result_id, result_id);
  if (!indexed.second || !function_data.value_order_.append(result_id)) {
    if (indexed.second) function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(indexed.second
        ? BuildError::StorageExhausted : BuildError::DuplicateSourceValue);
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, IntrinsicCallSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (!is_well_formed(spec.type) || !integer_type(spec.type) ||
      spec.callee_link_name.epoch != parent_->data_->epoch_ ||
      spec.callee_link_name.slot >= parent_->data_->link_names_.size() ||
      function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  const bool count_flag = spec.kind == IntrinsicKind::Cttz ||
                          spec.kind == IntrinsicKind::Ctlz;
  if (spec.arguments.size() != (count_flag ? 2U : 1U) ||
      spec.zero_count_is_undef.has_value() != count_flag)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  for (std::size_t index = 0; index < spec.arguments.size(); ++index) {
    const auto argument = function_data.values_.get(function_, spec.arguments[index]);
    if (!argument) return Result<BuildResult, BuildError>::failure(BuildError::InvalidValue);
    const Type expected = index == 0 ? spec.type : Type{TypeKind::Integer, 1, "i1"};
    if (argument.value().get().type != expected)
      return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  }
  detail::InstData instruction;
  instruction.opcode = Opcode::Call;
  instruction.payload = IntrinsicCallNode{spec.kind, spec.callee_link_name,
                                          spec.type, spec.zero_count_is_undef};
  instruction.operands = std::move(spec.arguments);
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result;
  result.kind = ValueKind::Ordinary;
  result.type = spec.type;
  result.source_id = SourceValueId{function_, spec.source_result_id};
  result.definition = InstResultDef{instruction_id, 0};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error()));
  }
  const auto result_id = inserted_result.value();
  auto stored_instruction = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored_instruction) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored_instruction.value().get().results = {result_id};
  try {
    function_data.values_by_source_id_.emplace(spec.source_result_id, result_id);
    function_data.value_order_.append(result_id);
  } catch (...) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    throw;
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block, SelectSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type i64{TypeKind::Integer, 64, "i64"};
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if ((spec.type != i32 && spec.type != i64) || function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (spec.condition) {
    if (!same_owner(function_, *spec.condition))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    const auto condition = function_data.values_.get(function_, *spec.condition);
    const auto* definition = condition ? std::get_if<InstResultDef>(&condition.value().get().definition) : nullptr;
    const auto producer = definition ? function_data.insts_.get(function_, definition->instruction)
                                     : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
    const auto* compare = producer ? std::get_if<CompareNode>(&producer.value().get().payload) : nullptr;
    if (!condition || condition.value().get().type != Type{TypeKind::I1, 1, "i1"} || !compare ||
        compare->predicate != ComparePredicate::Eq || compare->type != spec.type)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (spec.false_value) {
    if (!same_owner(function_, *spec.false_value))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    const auto false_value = function_data.values_.get(function_, *spec.false_value);
    const auto* definition = false_value ? std::get_if<InstResultDef>(&false_value.value().get().definition) : nullptr;
    const auto producer = definition ? function_data.insts_.get(function_, definition->instruction)
                                     : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
    const auto* binary = producer ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
    if (!false_value || false_value.value().get().type != spec.type || !binary ||
        binary->opcode != BinaryOpcode::Add || binary->type != spec.type)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  } else if (spec.type != i64 || spec.condition) {
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  detail::InstData instruction;
  instruction.opcode = Opcode::Select;
  instruction.payload = SelectNode{spec.type};
  if (spec.condition) instruction.operands.push_back(*spec.condition);
  if (spec.false_value) instruction.operands.push_back(*spec.false_value);
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result{ValueKind::Ordinary, spec.type,
                  SourceValueId{function_, spec.source_result_id},
                  InstResultDef{instruction_id, 0}};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error()));
  }
  const auto result_id = inserted_result.value();
  auto stored = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  try {
    const auto indexed = function_data.values_by_source_id_.emplace(spec.source_result_id, result_id);
    if (!indexed.second) {
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
    }
    if (!function_data.value_order_.append(result_id)) {
      function_data.values_by_source_id_.erase(spec.source_result_id);
      function_data.values_.erase(function_, result_id);
      function_data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
    }
  } catch (...) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    throw;
  }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id);
    function_data.value_order_.erase(result_id);
    function_data.values_.erase(function_, result_id);
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, SelectedMemcpySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.destination.valid() ||
      !spec.source.valid() || !spec.destination_object.valid() ||
      !spec.source_object.valid() || spec.destination.owner != function_ ||
      spec.source.owner != function_ || spec.destination_object.owner != function_ ||
      spec.source_object.owner != function_ ||
      spec.destination.value == spec.source.value ||
      spec.destination_object.value == spec.source_object.value ||
      spec.destination_object_owner != spec.source_object_owner ||
      !spec.destination_object_owner.valid() ||
      spec.destination_object_owner.epoch != parent_->data_->epoch_ ||
      spec.destination_object_owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} || spec.size_bytes <= 0 ||
      !spec.destination_live_at_site || !spec.source_live_at_site)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  detail::InstData instruction;
  instruction.opcode = Opcode::SelectedMemcpy;
  instruction.payload = SelectedMemcpyNode{
      spec.destination, spec.source, spec.destination_object, spec.source_object,
      spec.destination_object_owner, spec.source_object_owner, spec.pointer_type,
      spec.size_bytes, spec.destination_live_at_site, spec.source_live_at_site};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, Amd64SysVOverflowAggregateMemcpySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.va_list_pointer.valid() ||
      !spec.va_list_object.valid() || !spec.owner.valid() ||
      !spec.overflow_field_address.valid() || !spec.overflow_pointer_load.valid() ||
      !spec.destination.valid() || !spec.destination_object.valid() ||
      !spec.final_load.valid() || spec.va_list_pointer.owner != function_ ||
      spec.va_list_object.owner != function_ || spec.overflow_field_address.owner != function_ ||
      spec.overflow_pointer_load.owner != function_ || spec.destination.owner != function_ ||
      spec.destination_object.owner != function_ || spec.final_load.owner != function_ ||
      spec.va_list_pointer.value == spec.destination.value ||
      spec.va_list_object.value == spec.destination_object.value ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.payload_type.kind != TypeKind::Struct || spec.size_bytes <= 0 ||
      !spec.va_list_live || !spec.destination_live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  detail::InstData instruction;
  instruction.opcode = Opcode::Amd64SysVOverflowAggregateMemcpy;
  instruction.payload = Amd64SysVOverflowAggregateMemcpyNode{
      spec.va_list_pointer, spec.va_list_object, spec.owner,
      spec.overflow_field_address, spec.overflow_pointer_load, spec.destination,
      spec.destination_object, spec.final_load, spec.payload_type, spec.size_bytes,
      spec.va_list_live, spec.destination_live};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block, CastSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !same_owner(function_, spec.operand))
    return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& function_data = function.value().get();
  if (!function_data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const Type i64{TypeKind::Integer, 64, "i64"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type f64{TypeKind::F64, 64, "double"};
  const Type f32{TypeKind::F32, 32, "float"};
  auto operand = function_data.values_.get(function_, spec.operand);
  const bool intrinsic_trunc =
      spec.kind == CastKind::Trunc && spec.from_type == i64 && spec.to_type == i32;
  const bool scalar_sext =
      spec.kind == CastKind::SExt && spec.from_type == i32 && spec.to_type == i64;
  const bool scalar_fptrunc =
      spec.kind == CastKind::FPTrunc && spec.from_type == f64 && spec.to_type == f32;
  const bool scalar_fpext =
      spec.kind == CastKind::FPExt && spec.from_type == f32 && spec.to_type == f64;
  const bool scalar_sitofp =
      spec.kind == CastKind::SIToFP && spec.from_type == i32 && spec.to_type == f64;
  const bool scalar_uitofp =
      spec.kind == CastKind::UIToFP && spec.from_type == i32 && spec.to_type == f64;
  const bool scalar_fptosi =
      spec.kind == CastKind::FPToSI && spec.from_type == f64 && spec.to_type == i32;
  const bool scalar_fptoui =
      spec.kind == CastKind::FPToUI && spec.from_type == f64 && spec.to_type == i32;
  const bool pointer_truthiness_ptrtoint =
      spec.kind == CastKind::PtrToInt &&
      spec.from_type == Type{TypeKind::Pointer} && spec.to_type == i64;
  if ((!intrinsic_trunc && !scalar_sext && !scalar_fptrunc && !scalar_fpext && !scalar_sitofp && !scalar_uitofp && !scalar_fptosi && !scalar_fptoui && !pointer_truthiness_ptrtoint) || !operand ||
      operand.value().get().type != spec.from_type ||
      function_data.values_by_source_id_.count(spec.source_result_id) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  const auto* operand_def = std::get_if<InstResultDef>(&operand.value().get().definition);
  const auto* operand_parameter = std::get_if<ParameterDef>(&operand.value().get().definition);
  if (!operand_def && !operand_parameter)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (pointer_truthiness_ptrtoint && !operand_parameter)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (!pointer_truthiness_ptrtoint && !operand_def)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  if (intrinsic_trunc) {
    const auto producer = function_data.insts_.get(function_, operand_def->instruction);
    if (!producer || (!std::holds_alternative<IntrinsicCallNode>(producer.value().get().payload) &&
                      !std::holds_alternative<SelectNode>(producer.value().get().payload)))
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (scalar_fptrunc) {
    const auto producer = function_data.insts_.get(function_, operand_def->instruction);
    const auto* binary = producer
        ? std::get_if<BinaryNode>(&producer.value().get().payload)
        : nullptr;
    if (!binary || binary->opcode != BinaryOpcode::FMul || binary->type != f64)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (scalar_fpext) {
    const auto producer = function_data.insts_.get(function_, operand_def->instruction);
    const auto* cast = producer ? std::get_if<CastNode>(&producer.value().get().payload) : nullptr;
    if (!cast || cast->kind != CastKind::FPTrunc || cast->from_type != f64 || cast->to_type != f32)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (scalar_sitofp || scalar_uitofp) {
    const auto producer = function_data.insts_.get(function_, operand_def->instruction);
    const auto* binary = producer ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
    if (!binary || binary->opcode != BinaryOpcode::Add || binary->type != i32)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  if (scalar_fptosi || scalar_fptoui) {
    const auto producer = function_data.insts_.get(function_, operand_def->instruction);
    const auto* binary = producer ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
    if (!binary || binary->opcode != BinaryOpcode::FAdd || binary->type != f64)
      return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  }
  detail::InstData instruction;
  instruction.opcode = Opcode::Cast;
  instruction.payload = CastNode{spec.kind, spec.from_type, spec.to_type};
  instruction.operands = {spec.operand};
  auto inserted = function_data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef result{ValueKind::Ordinary, spec.to_type,
                  SourceValueId{function_, spec.source_result_id}, InstResultDef{instruction_id, 0}};
  auto inserted_result = function_data.values_.emplace(function_, std::move(result));
  if (!inserted_result) { function_data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(storage_error(inserted_result.error())); }
  const auto result_id = inserted_result.value();
  auto stored = function_data.insts_.get_mut(function_, instruction_id);
  if (!stored) { function_data.values_.erase(function_, result_id); function_data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted); }
  stored.value().get().results = {result_id};
  try { function_data.values_by_source_id_.emplace(spec.source_result_id, result_id); function_data.value_order_.append(result_id); }
  catch (...) { function_data.values_by_source_id_.erase(spec.source_result_id); function_data.values_.erase(function_, result_id); function_data.insts_.erase(function_, instruction_id); throw; }
  auto block_data = function_data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    function_data.values_by_source_id_.erase(spec.source_result_id); function_data.value_order_.erase(result_id); function_data.values_.erase(function_, result_id); function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block, PhiSpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block)) return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block) || !is_well_formed(spec.type) ||
      spec.type.kind == TypeKind::Void || spec.incoming.empty())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidValueType);
  const auto result = data.values_by_source_id_.find(spec.source_result_id);
  if (result == data.values_by_source_id_.end())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  auto result_def = data.values_.get_mut(function_, result->second);
  if (!result_def || result_def.value().get().type != spec.type ||
      !std::holds_alternative<UnresolvedDef>(result_def.value().get().definition))
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  detail::InstData instruction;
  instruction.opcode = Opcode::Phi;
  PhiNode node{spec.type, {}};
  node.incoming.reserve(spec.incoming.size());
  instruction.operands.reserve(spec.incoming.size());
  for (const auto& incoming : spec.incoming) {
    if (!same_owner(function_, incoming.value) || !same_owner(function_, incoming.predecessor) ||
        incoming.destination != block || !data.values_.contains(function_, incoming.value) ||
        !data.blocks_.contains(function_, incoming.predecessor))
      return Result<BuildResult, BuildError>::failure(BuildError::ForeignOwner);
    const auto value = data.values_.get(function_, incoming.value);
    if (!value || value.value().get().type != spec.type)
      return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
    for (const auto& prior : node.incoming)
      if (prior.edge.predecessor == incoming.predecessor &&
          prior.edge.occurrence == incoming.occurrence)
        return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
    node.incoming.push_back({incoming.value, {incoming.predecessor, block, incoming.occurrence}});
    instruction.operands.push_back(incoming.value);
  }
  instruction.payload = std::move(node);
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto stored = data.insts_.get_mut(function_, instruction_id);
  if (!stored) { data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted); }
  stored.value().get().results = {result->second};
  result_def.value().get().definition = InstResultDef{instruction_id, 0};
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    result_def.value().get().definition = UnresolvedDef{};
    data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result->second}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(BlockId block,
                                                        AllocaAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.result.valid() ||
      !spec.pointer_definition.valid() || !spec.object.valid() ||
      spec.result != spec.pointer_definition || spec.result.owner != function_ ||
      spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} ||
      !is_well_formed(spec.pointee_type) || spec.pointee_type.kind == TypeKind::Void ||
      !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto result = data.values_by_source_id_.find(spec.result.value);
  if (result == data.values_by_source_id_.end())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  auto result_def = data.values_.get_mut(function_, result->second);
  if (!result_def || result_def.value().get().type != spec.pointer_type ||
      !std::holds_alternative<UnresolvedDef>(result_def.value().get().definition))
    return Result<BuildResult, BuildError>::failure(BuildError::DefinitionTypeMismatch);
  detail::InstData instruction;
  instruction.opcode = Opcode::AllocaAuthority;
  instruction.payload = AllocaAuthorityNode{spec.result, spec.pointer_definition,
                                             spec.object, spec.owner,
                                             spec.pointer_type, spec.pointee_type,
                                             spec.live};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto stored = data.insts_.get_mut(function_, instruction_id);
  if (!stored) { data.insts_.erase(function_, instruction_id); return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted); }
  stored.value().get().results = {result->second};
  result_def.value().get().definition = InstResultDef{instruction_id, 0};
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    result_def.value().get().definition = UnresolvedDef{};
    data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result->second}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, StackSaveAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.result.valid() ||
      !spec.pointer_definition.valid() || !spec.object.valid() ||
      spec.result != spec.pointer_definition || spec.result.owner != function_ ||
      spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} ||
      spec.pointee_type != Type{TypeKind::Pointer} || !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  if (data.values_by_source_id_.count(spec.result.value) != 0)
    return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
  detail::InstData instruction;
  instruction.opcode = Opcode::StackSaveAuthority;
  instruction.payload = StackSaveAuthorityNode{spec.result, spec.pointer_definition,
      spec.object, spec.owner, spec.pointer_type, spec.pointee_type, spec.live};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  ValueDef value;
  value.kind = ValueKind::Ordinary;
  value.type = Type{TypeKind::Pointer};
  value.source_id = spec.result;
  value.definition = InstResultDef{instruction_id, 0};
  auto inserted_value = data.values_.emplace(function_, std::move(value));
  if (!inserted_value) {
    data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(storage_error(inserted_value.error()));
  }
  const auto result_id = inserted_value.value();
  auto stored = data.insts_.get_mut(function_, instruction_id);
  if (!stored) {
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  stored.value().get().results = {result_id};
  if (!data.value_order_.append(result_id)) {
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  try {
    const auto indexed = data.values_by_source_id_.emplace(spec.result.value, result_id);
    if (!indexed.second) {
      data.value_order_.erase(result_id); data.values_.erase(function_, result_id);
      data.insts_.erase(function_, instruction_id);
      return Result<BuildResult, BuildError>::failure(BuildError::DuplicateSourceValue);
    }
  } catch (...) {
    data.value_order_.erase(result_id); data.values_.erase(function_, result_id);
    data.insts_.erase(function_, instruction_id); throw;
  }
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    data.values_by_source_id_.erase(spec.result.value); data.value_order_.erase(result_id);
    data.values_.erase(function_, result_id); data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {result_id}});
}

Result<BuildResult, BuildError> FunctionBuilder::append(
    BlockId block, StackRestoreAuthoritySpec spec) {
  auto function = mutable_function();
  if (!function) return Result<BuildResult, BuildError>::failure(function.error());
  if (!same_owner(function_, block) || !spec.saved_pointer_definition.valid() ||
      !spec.object.valid() || spec.saved_pointer_definition.owner != function_ ||
      spec.object.owner != function_ || !spec.owner.valid() ||
      spec.owner.epoch != parent_->data_->epoch_ ||
      spec.owner.slot >= parent_->data_->link_names_.size() ||
      spec.pointer_type != Type{TypeKind::Pointer} ||
      spec.pointee_type != Type{TypeKind::Pointer} || !spec.live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  auto& data = function.value().get();
  if (!data.blocks_.contains(function_, block))
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidBlock);
  const auto saved = data.values_by_source_id_.find(spec.saved_pointer_definition.value);
  if (saved == data.values_by_source_id_.end())
    return Result<BuildResult, BuildError>::failure(BuildError::InvalidSourceValueId);
  const auto value = data.values_.get(function_, saved->second);
  const auto* definition = value ? std::get_if<InstResultDef>(&value.value().get().definition) : nullptr;
  const auto producer = definition
      ? data.insts_.get(function_, definition->instruction)
      : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
  const auto* stack_save = producer
      ? std::get_if<StackSaveAuthorityNode>(&producer.value().get().payload) : nullptr;
  if (!value || value.value().get().type != Type{TypeKind::Pointer} || !stack_save ||
      stack_save->result != spec.saved_pointer_definition ||
      stack_save->pointer_definition != spec.saved_pointer_definition ||
      stack_save->object.owner != spec.object.owner ||
      stack_save->object.value != spec.object.value || stack_save->owner != spec.owner ||
      stack_save->pointer_type != spec.pointer_type ||
      stack_save->pointee_type != spec.pointee_type || !stack_save->live)
    return Result<BuildResult, BuildError>::failure(BuildError::UnsupportedOpcode);
  detail::InstData instruction;
  instruction.opcode = Opcode::StackRestoreAuthority;
  instruction.payload = StackRestoreAuthorityNode{spec.saved_pointer_definition,
      spec.object, spec.owner, spec.pointer_type, spec.pointee_type, spec.live};
  instruction.operands = {saved->second};
  auto inserted = data.insts_.emplace(function_, std::move(instruction));
  if (!inserted) return Result<BuildResult, BuildError>::failure(storage_error(inserted.error()));
  const auto instruction_id = inserted.value();
  auto block_data = data.blocks_.get_mut(function_, block);
  if (!block_data || !block_data.value().get().instruction_order_.append(instruction_id)) {
    data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(BuildResult{instruction_id, {}});
}

Result<void, BuildError> FunctionBuilder::set_terminator(
    BlockId block, TerminatorSpec terminator) {
  auto function_result = mutable_function();
  if (!function_result)
    return Result<void, BuildError>::failure(function_result.error());
  auto& function = function_result.value().get();
  if (!same_owner(function_, block))
    return Result<void, BuildError>::failure(BuildError::ForeignOwner);
  auto block_result = function.blocks_.get_mut(function_, block);
  if (!block_result)
    return Result<void, BuildError>::failure(BuildError::InvalidBlock);
  if (block_result.value().get().terminator_)
    return Result<void, BuildError>::failure(BuildError::TerminatorAlreadySet);

  auto checked = std::visit(
      [&](const auto& term) -> Result<void, BuildError> {
        using Term = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<Term, JumpTerm>) {
          if (!same_owner(function_, term.target))
            return Result<void, BuildError>::failure(BuildError::ForeignOwner);
          if (!function.blocks_.contains(function_, term.target))
            return Result<void, BuildError>::failure(BuildError::InvalidBlock);
        } else if constexpr (std::is_same_v<Term, CondJumpTerm>) {
          if (!same_owner(function_, term.condition) ||
              !same_owner(function_, term.true_target) ||
              !same_owner(function_, term.false_target))
            return Result<void, BuildError>::failure(BuildError::ForeignOwner);
          auto condition = function.values_.get(function_, term.condition);
          if (!condition)
            return Result<void, BuildError>::failure(BuildError::InvalidValue);
          if (condition.value().get().type.kind != TypeKind::I1)
            return Result<void, BuildError>::failure(
                BuildError::InvalidConditionType);
          if (!function.blocks_.contains(function_, term.true_target) ||
              !function.blocks_.contains(function_, term.false_target))
            return Result<void, BuildError>::failure(BuildError::InvalidBlock);
        } else if constexpr (std::is_same_v<Term, IndirectJumpTerm>) {
          if (!same_owner(function_, term.address))
            return Result<void, BuildError>::failure(BuildError::ForeignOwner);
          const auto address = function.values_.get(function_, term.address);
          if (!address)
            return Result<void, BuildError>::failure(BuildError::InvalidValue);
          if (address.value().get().type.kind != TypeKind::Pointer)
            return Result<void, BuildError>::failure(BuildError::InvalidValueType);
          if (term.targets.empty())
            return Result<void, BuildError>::failure(BuildError::InvalidBlock);
          std::unordered_set<std::uint32_t> targets;
          for (const auto target : term.targets) {
            if (!same_owner(function_, target))
              return Result<void, BuildError>::failure(BuildError::ForeignOwner);
            if (!function.blocks_.contains(function_, target) ||
                !targets.insert(target.slot).second)
              return Result<void, BuildError>::failure(BuildError::InvalidBlock);
          }
        } else if constexpr (std::is_same_v<Term, SwitchTerm>) {
          if (!same_owner(function_, term.selector) ||
              !same_owner(function_, term.default_target))
            return Result<void, BuildError>::failure(BuildError::ForeignOwner);
          const auto selector = function.values_.get(function_, term.selector);
          if (!selector)
            return Result<void, BuildError>::failure(BuildError::InvalidValue);
          if (!integer_type(selector.value().get().type))
            return Result<void, BuildError>::failure(BuildError::InvalidValueType);
          if (!function.blocks_.contains(function_, term.default_target))
            return Result<void, BuildError>::failure(BuildError::InvalidBlock);
          for (const auto target : term.case_targets) {
            if (!same_owner(function_, target))
              return Result<void, BuildError>::failure(BuildError::ForeignOwner);
            if (!function.blocks_.contains(function_, target))
              return Result<void, BuildError>::failure(BuildError::InvalidBlock);
          }
        } else if constexpr (std::is_same_v<Term, ReturnTerm>) {
          const bool returns_void =
              function.signature_.return_type.kind == TypeKind::Void;
          if (returns_void != !term.value)
            return Result<void, BuildError>::failure(BuildError::InvalidReturn);
          if (term.value) {
            if (!same_owner(function_, *term.value))
              return Result<void, BuildError>::failure(BuildError::ForeignOwner);
            auto value = function.values_.get(function_, *term.value);
            if (!value)
              return Result<void, BuildError>::failure(BuildError::InvalidValue);
            if (value.value().get().type != function.signature_.return_type)
              return Result<void, BuildError>::failure(BuildError::InvalidReturn);
          }
        }
        return Result<void, BuildError>::success();
      },
      terminator);
  if (!checked)
    return checked;

  block_result.value().get().terminator_ = std::move(terminator);
  return Result<void, BuildError>::success();
}

}  // namespace c4c::backend::bir
