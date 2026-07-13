#include "builder.hpp"

#include <atomic>
#include <limits>
#include <type_traits>
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
    std::vector<c4c::LinkNameId> initializer_function_links) {
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
    FunctionSignature signature, std::string link_name, bool is_declaration) {
  if (state_ == State::Consumed)
    return Result<FunctionId, BuildError>::failure(BuildError::AlreadyConsumed);
  if (state_ == State::EditingFunction)
    return Result<FunctionId, BuildError>::failure(BuildError::ActiveFunctionEdit);
  if (!data_ || data_->epoch_ == 0)
    return Result<FunctionId, BuildError>::failure(BuildError::EpochExhausted);
  if (link_name.empty())
    return Result<FunctionId, BuildError>::failure(BuildError::EmptyLinkName);
  if (signature.parameter_types.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    return Result<FunctionId, BuildError>::failure(BuildError::StorageExhausted);

  const auto existing = data_->functions_by_link_name_.find(link_name);
  if (existing != data_->functions_by_link_name_.end()) {
    auto function = data_->functions_.get_mut(data_->epoch_, existing->second);
    if (!function)
      return Result<FunctionId, BuildError>::failure(BuildError::InvalidFunction);
    auto& stored = function.value().get();
    if (stored.signature_ != signature)
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
  auto stored_instruction =
      function_data.insts_.get_mut(function_, instruction_id);
  if (!stored_instruction) {
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
    function_data.insts_.erase(function_, instruction_id);
    return Result<BuildResult, BuildError>::failure(BuildError::StorageExhausted);
  }
  return Result<BuildResult, BuildError>::success(
      BuildResult{instruction_id, std::move(results)});
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
