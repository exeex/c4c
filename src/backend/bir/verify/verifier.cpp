#include "../core/builder.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace c4c::backend::bir {
namespace {

template <class Entity>
void report(VerificationResult& result, VerificationRule rule,
            FunctionId function, Entity entity, std::string message) {
  result.errors.push_back(
      VerificationError{rule, function, VerificationEntity{std::move(entity)},
                        std::move(message)});
}

bool known(ValueKind kind) noexcept {
  switch (kind) {
    case ValueKind::Parameter:
    case ValueKind::Ordinary:
      return true;
  }
  return false;
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

bool known(ReturnExtension extension) noexcept {
  switch (extension) {
    case ReturnExtension::None:
    case ReturnExtension::SignExt:
    case ReturnExtension::ZeroExt: return true;
  }
  return false;
}

bool known(SymbolVisibility visibility) noexcept {
  switch (visibility) {
    case SymbolVisibility::Default:
    case SymbolVisibility::Hidden:
    case SymbolVisibility::Protected: return true;
  }
  return false;
}

bool opcode_matches_payload(const detail::InstData& instruction) noexcept {
  switch (instruction.opcode) {
    case Opcode::InlineAsm:
      return std::holds_alternative<InlineAsmNode>(instruction.payload);
    case Opcode::Store:
      return std::holds_alternative<StoreNode>(instruction.payload);
    case Opcode::Load:
      return std::holds_alternative<LoadNode>(instruction.payload);
    case Opcode::GetElementPtr:
      return std::holds_alternative<GetElementPtrNode>(instruction.payload);
    case Opcode::Call:
      return std::holds_alternative<CallNode>(instruction.payload) ||
             std::holds_alternative<IntrinsicCallNode>(instruction.payload);
    case Opcode::Binary:
      return std::holds_alternative<BinaryNode>(instruction.payload);
    case Opcode::Cast:
      return std::holds_alternative<CastNode>(instruction.payload);
  }
  return false;
}

template <class Id>
std::unordered_map<Id, std::size_t> counts(const std::vector<Id>& ids) {
  std::unordered_map<Id, std::size_t> result;
  for (const auto id : ids) ++result[id];
  return result;
}

}  // namespace

VerificationResult FoundationVerifier::verify(const detail::ModuleData& module,
                                               VerifyProfile) {
  VerificationResult result;
  if (module.epoch_ == 0)
    report(result, VerificationRule::ModuleEpoch, {}, ModuleEntity{},
           "module epoch must be nonzero");

  for (std::size_t index = 0; index < module.link_names_.size(); ++index) {
    const LinkNameId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& entry = module.link_names_[index];
    const auto by_source = module.link_names_by_source_id_.find(entry.source_id);
    const auto by_spelling = module.link_names_by_spelling_.find(entry.spelling);
    if (!id.valid() || entry.source_id == c4c::kInvalidLinkName ||
        entry.source_id != index + 1 || entry.spelling.empty() ||
        by_source == module.link_names_by_source_id_.end() ||
        by_source->second != id ||
        by_spelling == module.link_names_by_spelling_.end() ||
        by_spelling->second != id)
      report(result, VerificationRule::ModuleNameTable, {}, id,
             "link-name order, identity, spelling, and indexes must agree");
  }
  if (module.link_names_by_source_id_.size() != module.link_names_.size() ||
      module.link_names_by_spelling_.size() != module.link_names_.size())
    report(result, VerificationRule::ModuleNameTable, {}, ModuleEntity{},
           "link-name index sizes must match the ordered table");

  for (std::size_t index = 0; index < module.struct_names_.size(); ++index) {
    const StructNameId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& entry = module.struct_names_[index];
    const auto by_source = module.struct_names_by_source_id_.find(entry.source_id);
    const auto by_spelling = module.struct_names_by_spelling_.find(entry.spelling);
    if (!id.valid() || entry.source_id == c4c::kInvalidStructName ||
        entry.source_id != index + 1 || entry.spelling.empty() ||
        by_source == module.struct_names_by_source_id_.end() ||
        by_source->second != id ||
        by_spelling == module.struct_names_by_spelling_.end() ||
        by_spelling->second != id)
      report(result, VerificationRule::ModuleNameTable, {}, id,
             "struct-name order, identity, spelling, and indexes must agree");
  }
  if (module.struct_names_by_source_id_.size() != module.struct_names_.size() ||
      module.struct_names_by_spelling_.size() != module.struct_names_.size())
    report(result, VerificationRule::ModuleNameTable, {}, ModuleEntity{},
           "struct-name index sizes must match the ordered table");

  for (std::size_t index = 0; index < module.struct_decls_.size(); ++index) {
    const StructDeclId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& decl = module.struct_decls_[index];
    const auto cached = module.struct_decls_by_name_.find(decl.name);
    if (!decl.name.valid() || decl.name.epoch != module.epoch_ ||
        decl.name.slot >= module.struct_names_.size() ||
        cached == module.struct_decls_by_name_.end() || cached->second != id)
      report(result, VerificationRule::StructDeclaration, {}, id,
             "struct declaration name and cache must resolve exactly");
    if (decl.is_opaque && (!decl.fields.empty() || decl.is_packed))
      report(result, VerificationRule::StructDeclaration, {}, id,
             "opaque struct declarations cannot carry fields or packed layout");
    for (const auto& field : decl.fields) {
      if (!is_well_formed(field.type) || field.type.kind == TypeKind::Void) {
        report(result, VerificationRule::StructDeclaration, {}, id,
               "struct field type must be well formed");
        continue;
      }
      if (field.type.struct_name_id != c4c::kInvalidStructName) {
        const auto named = module.struct_names_by_source_id_.find(
            field.type.struct_name_id);
        if (named == module.struct_names_by_source_id_.end() ||
            field.referenced_name != named->second ||
            named->second.slot >= module.struct_names_.size() ||
            module.struct_names_[named->second.slot].spelling !=
                field.type.spelling)
          report(result, VerificationRule::StructDeclaration, {}, id,
                 "named struct field type must resolve with matching spelling");
      } else if (field.referenced_name.valid()) {
        report(result, VerificationRule::StructDeclaration, {}, id,
               "unnamed struct field type cannot carry a named reference");
      }
    }
  }
  if (module.struct_decls_by_name_.size() != module.struct_decls_.size())
    report(result, VerificationRule::StructDeclaration, {}, ModuleEntity{},
           "struct declaration cache size must match source order");

  std::unordered_map<ConstantId, std::size_t> constant_references;
  for (std::size_t index = 0; index < module.constants_.size(); ++index) {
    const ConstantId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& constant = module.constants_[index];
    if (!id.valid() || !is_well_formed(constant.type) ||
        constant.type.kind == TypeKind::Void ||
        constant.payload.valueless_by_exception()) {
      report(result, VerificationRule::ConstantDefinition, {}, id,
             "constant identity, type, and payload must be well formed");
      continue;
    }
    if (std::holds_alternative<IntegerConstant>(constant.payload) !=
            integer_type(constant.type) ||
        std::holds_alternative<FloatingConstant>(constant.payload) !=
            floating_type(constant.type))
      report(result, VerificationRule::ConstantDefinition, {}, id,
             "constant payload alternative must match its exact type domain");
  }

  for (std::size_t index = 0; index < module.string_data_.size(); ++index) {
    const StringDataId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& data = module.string_data_[index];
    const auto named = module.string_data_by_name_.find(data.pool_name);
    if (!id.valid() || data.pool_name.empty() || data.byte_length < -1 ||
        named == module.string_data_by_name_.end() || named->second != id)
      report(result, VerificationRule::StringDataStorage, {}, id,
             "string data identity, name, length, and name index must agree");
  }
  if (module.string_data_by_name_.size() != module.string_data_.size())
    report(result, VerificationRule::StringDataStorage, {}, ModuleEntity{},
           "string data name index size must match ordered storage");
  for (const auto& entry : module.string_data_by_name_) {
    if (entry.first.empty() || entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.string_data_.size() ||
        (entry.second.slot < module.string_data_.size() &&
         module.string_data_[entry.second.slot].pool_name != entry.first))
      report(result, VerificationRule::StringDataStorage, {}, entry.second,
             "string data name index contains a foreign or conflicting row");
  }

  std::size_t linked_external_count = 0;
  for (std::size_t index = 0; index < module.external_decls_.size(); ++index) {
    const ExternalDeclId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& declaration = module.external_decls_[index];
    const auto named =
        module.external_decls_by_name_.find(declaration.source_name);
    bool return_type_resolves = true;
    if (declaration.return_type.struct_name_id != c4c::kInvalidStructName) {
      const auto struct_name = module.struct_names_by_source_id_.find(
          declaration.return_type.struct_name_id);
      return_type_resolves =
          struct_name != module.struct_names_by_source_id_.end() &&
          struct_name->second.slot < module.struct_names_.size() &&
          module.struct_names_[struct_name->second.slot].spelling ==
              declaration.return_type.spelling &&
          module.struct_decls_by_name_.count(struct_name->second) != 0;
    }
    if (!id.valid() || declaration.source_name.empty() ||
        !is_well_formed(declaration.return_type) ||
        !return_type_resolves ||
        !known(declaration.return_extension) ||
        (declaration.return_extension != ReturnExtension::None &&
         !integer_type(declaration.return_type)) ||
        named == module.external_decls_by_name_.end() || named->second != id)
      report(result, VerificationRule::ExternalDeclaration, {}, id,
             "external declaration name, type, extension, and name index must agree");

    if (const auto* link_name =
            std::get_if<LinkNameId>(&declaration.identity)) {
      ++linked_external_count;
      const auto linked =
          module.external_decls_by_link_name_.find(*link_name);
      if (!link_name->valid() || link_name->epoch != module.epoch_ ||
          link_name->slot >= module.link_names_.size() ||
          (link_name->slot < module.link_names_.size() &&
           module.link_names_[link_name->slot].spelling !=
               declaration.source_name) ||
          linked == module.external_decls_by_link_name_.end() ||
          linked->second != id)
        report(result, VerificationRule::ExternalDeclaration, {}, id,
               "link-backed external identity must resolve exactly");
    } else if (const auto* fallback =
                   std::get_if<FallbackExternalName>(&declaration.identity)) {
      if (fallback->name.empty() || fallback->name != declaration.source_name)
        report(result, VerificationRule::ExternalDeclaration, {}, id,
               "fallback external identity must retain its exact source name");
    } else {
      report(result, VerificationRule::ExternalDeclaration, {}, id,
             "external identity has no known alternative");
    }
  }
  if (module.external_decls_by_name_.size() !=
      module.external_decls_.size())
    report(result, VerificationRule::ExternalDeclaration, {}, ModuleEntity{},
           "external name index size must match ordered declarations");
  if (module.external_decls_by_link_name_.size() != linked_external_count)
    report(result, VerificationRule::ExternalDeclaration, {}, ModuleEntity{},
           "external link index size must match link-backed declarations");
  for (const auto& entry : module.external_decls_by_name_) {
    if (entry.first.empty() || entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.external_decls_.size() ||
        (entry.second.slot < module.external_decls_.size() &&
         module.external_decls_[entry.second.slot].source_name != entry.first))
      report(result, VerificationRule::ExternalDeclaration, {}, entry.second,
             "external name index contains a foreign or conflicting row");
  }
  for (const auto& entry : module.external_decls_by_link_name_) {
    if (!entry.first.valid() || entry.first.epoch != module.epoch_ ||
        entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.external_decls_.size() ||
        (entry.second.slot < module.external_decls_.size() &&
         (!std::holds_alternative<LinkNameId>(
              module.external_decls_[entry.second.slot].identity) ||
          std::get<LinkNameId>(
              module.external_decls_[entry.second.slot].identity) !=
              entry.first)))
      report(result, VerificationRule::ExternalDeclaration, {}, entry.second,
             "external link index contains a foreign or conflicting row");
  }

  std::size_t linked_global_count = 0;
  for (std::size_t index = 0; index < module.globals_.size(); ++index) {
    const GlobalObjectId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& global = module.globals_[index];
    const auto named = module.globals_by_name_.find(global.source_name);
    bool type_resolves = true;
    if (global.object_type.struct_name_id != c4c::kInvalidStructName) {
      const auto struct_name = module.struct_names_by_source_id_.find(
          global.object_type.struct_name_id);
      type_resolves =
          struct_name != module.struct_names_by_source_id_.end() &&
          struct_name->second.slot < module.struct_names_.size() &&
          module.struct_names_[struct_name->second.slot].spelling ==
              global.object_type.spelling &&
          module.struct_decls_by_name_.count(struct_name->second) != 0;
    }
    const auto nested_va_list_facts_resolve =
        [&](const std::optional<VaListTypeFacts>& facts) {
          if (!facts || facts->is_pointer_object) return true;
          const auto struct_name =
              module.struct_names_by_source_id_.find(facts->struct_name_id);
          if (struct_name == module.struct_names_by_source_id_.end() ||
              !struct_name->second.valid() ||
              struct_name->second.epoch != module.epoch_ ||
              struct_name->second.slot >= module.struct_names_.size() ||
              module.struct_names_[struct_name->second.slot].spelling !=
                  "%struct.__va_list_tag_")
            return false;
          const auto declaration =
              module.struct_decls_by_name_.find(struct_name->second);
          if (declaration == module.struct_decls_by_name_.end() ||
              !declaration->second.valid() ||
              declaration->second.epoch != module.epoch_ ||
              declaration->second.slot >= module.struct_decls_.size())
            return false;
          const auto& resolved =
              module.struct_decls_[declaration->second.slot];
          if (resolved.name != struct_name->second || resolved.is_packed ||
              resolved.is_opaque)
            return false;
          const auto is_i32 = [](const StructField& field) {
            const auto& type = field.type;
            return type.kind == TypeKind::Integer && type.bit_width == 32 &&
                   type.spelling == "i32" &&
                   type.struct_name_id == c4c::kInvalidStructName &&
                   !type.structured_spec && !type.array_facts &&
                   !type.pointer_facts && !type.vector_facts &&
                   !type.complex_facts && !type.va_list_facts &&
                   !field.referenced_name.valid();
          };
          const auto is_ptr = [](const StructField& field) {
            const auto& type = field.type;
            return type.kind == TypeKind::Pointer && type.bit_width == 0 &&
                   type.spelling == "ptr" &&
                   type.struct_name_id == c4c::kInvalidStructName &&
                   !type.structured_spec && !type.array_facts &&
                   !type.pointer_facts && !type.vector_facts &&
                   !type.complex_facts && !type.va_list_facts &&
                   !field.referenced_name.valid();
          };
          const auto& fields = resolved.fields;
          if (facts->storage_size == 24 &&
              facts->storage_alignment == 16)
            return fields.size() == 4 && is_i32(fields[0]) &&
                   is_i32(fields[1]) && is_ptr(fields[2]) &&
                   is_ptr(fields[3]);
          if (facts->storage_size == 32 && facts->storage_alignment == 8)
            return fields.size() == 5 && is_ptr(fields[0]) &&
                   is_ptr(fields[1]) && is_ptr(fields[2]) &&
                   is_i32(fields[3]) && is_i32(fields[4]);
          return false;
        };
    type_resolves =
        type_resolves &&
        nested_va_list_facts_resolve(global.object_type.va_list_facts) &&
        (!global.object_type.pointer_facts ||
         nested_va_list_facts_resolve(
             global.object_type.pointer_facts->pointee_va_list_facts)) &&
        (!global.object_type.array_facts ||
         nested_va_list_facts_resolve(
             global.object_type.array_facts->element_va_list_facts));
    const bool valid_alignment =
        global.alignment >= 0 &&
        (global.alignment == 0 ||
         (global.alignment & (global.alignment - 1)) == 0);
    if (!id.valid() || global.source_name.empty() ||
        !is_well_formed(global.object_type) ||
        global.object_type.kind == TypeKind::Void || !type_resolves ||
        !valid_alignment || !known(global.visibility) ||
        (global.is_weak && global.is_internal) ||
        (global.is_extern_declaration == global.initializer.has_value()) ||
        named == module.globals_by_name_.end() || named->second != id)
      report(result, VerificationRule::GlobalObject, {}, id,
             "global identity, linkage classification, definition/extern initializer shape, type, alignment, and name index must agree");

    if (global.initializer) {
      if (global.initializer->opaque_payload.empty())
        report(result, VerificationRule::GlobalObject, {}, id,
               "global definition initializer payload must be present exactly");
      for (const LinkNameId function_link :
           global.initializer->function_links) {
        if (!function_link.valid() || function_link.epoch != module.epoch_ ||
            function_link.slot >= module.link_names_.size())
          report(result, VerificationRule::GlobalObject, {}, function_link,
                 "global initializer function link must resolve in the module link-name domain");
      }
    }

    if (const auto* link_name = std::get_if<LinkNameId>(&global.identity)) {
      ++linked_global_count;
      const auto linked = module.globals_by_link_name_.find(*link_name);
      if (!link_name->valid() || link_name->epoch != module.epoch_ ||
          link_name->slot >= module.link_names_.size() ||
          (link_name->slot < module.link_names_.size() &&
           module.link_names_[link_name->slot].spelling != global.source_name) ||
          linked == module.globals_by_link_name_.end() || linked->second != id)
        report(result, VerificationRule::GlobalObject, {}, id,
               "link-backed global identity must resolve exactly");
    } else if (const auto* fallback =
                   std::get_if<FallbackGlobalName>(&global.identity)) {
      if (fallback->name.empty() || fallback->name != global.source_name)
        report(result, VerificationRule::GlobalObject, {}, id,
               "fallback global identity must retain its exact source name");
    } else {
      report(result, VerificationRule::GlobalObject, {}, id,
             "global identity has no known alternative");
    }
  }
  if (module.globals_by_name_.size() != module.globals_.size())
    report(result, VerificationRule::GlobalObject, {}, ModuleEntity{},
           "global name index size must match ordered storage");
  if (module.globals_by_link_name_.size() != linked_global_count)
    report(result, VerificationRule::GlobalObject, {}, ModuleEntity{},
           "global link index size must match link-backed rows");
  for (const auto& entry : module.globals_by_name_) {
    if (entry.first.empty() || entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.globals_.size() ||
        (entry.second.slot < module.globals_.size() &&
         module.globals_[entry.second.slot].source_name != entry.first))
      report(result, VerificationRule::GlobalObject, {}, entry.second,
             "global name index contains a foreign or conflicting row");
  }
  for (const auto& entry : module.globals_by_link_name_) {
    if (!entry.first.valid() || entry.first.epoch != module.epoch_ ||
        entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.globals_.size() ||
        (entry.second.slot < module.globals_.size() &&
         (!std::holds_alternative<LinkNameId>(
              module.globals_[entry.second.slot].identity) ||
          std::get<LinkNameId>(module.globals_[entry.second.slot].identity) !=
              entry.first)))
      report(result, VerificationRule::GlobalObject, {}, entry.second,
             "global link index contains a foreign or conflicting row");
  }

  for (std::size_t index = 0; index < module.specializations_.size(); ++index) {
    const SpecializationId id{module.epoch_, static_cast<SlotIndex>(index)};
    const auto& specialization = module.specializations_[index];
    const detail::ModuleData::SpecializationSemanticKey semantic_key{
        specialization.spec_key, specialization.template_origin};
    const auto semantic =
        module.specializations_by_semantic_key_.find(semantic_key);
    const auto linked = module.specializations_by_link_name_.find(
        specialization.mangled_link_name);
    if (!id.valid() || specialization.spec_key.empty() ||
        specialization.template_origin.empty() ||
        specialization.mangled_name.empty() ||
        !specialization.mangled_link_name.valid() ||
        specialization.mangled_link_name.epoch != module.epoch_ ||
        specialization.mangled_link_name.slot >= module.link_names_.size() ||
        (specialization.mangled_link_name.slot < module.link_names_.size() &&
         module.link_names_[specialization.mangled_link_name.slot].spelling !=
             specialization.mangled_name) ||
        semantic == module.specializations_by_semantic_key_.end() ||
        semantic->second != id ||
        linked == module.specializations_by_link_name_.end() ||
        linked->second != id)
      report(result, VerificationRule::SpecializationMetadata, {}, id,
             "specialization order, fields, link identity, and indexes must agree");
  }
  if (module.specializations_by_semantic_key_.size() !=
          module.specializations_.size() ||
      module.specializations_by_link_name_.size() !=
          module.specializations_.size())
    report(result, VerificationRule::SpecializationMetadata, {}, ModuleEntity{},
           "specialization semantic and link indexes must match ordered storage");
  for (const auto& entry : module.specializations_by_semantic_key_) {
    if (entry.first.spec_key.empty() || entry.first.template_origin.empty() ||
        entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.specializations_.size() ||
        (entry.second.slot < module.specializations_.size() &&
         (module.specializations_[entry.second.slot].spec_key !=
              entry.first.spec_key ||
          module.specializations_[entry.second.slot].template_origin !=
              entry.first.template_origin)))
      report(result, VerificationRule::SpecializationMetadata, {}, entry.second,
             "specialization semantic index contains a foreign or conflicting row");
  }
  for (const auto& entry : module.specializations_by_link_name_) {
    if (!entry.first.valid() || entry.first.epoch != module.epoch_ ||
        entry.first.slot >= module.link_names_.size() ||
        entry.second.epoch != module.epoch_ ||
        entry.second.slot >= module.specializations_.size() ||
        (entry.second.slot < module.specializations_.size() &&
         module.specializations_[entry.second.slot].mangled_link_name !=
             entry.first))
      report(result, VerificationRule::SpecializationMetadata, {}, entry.second,
             "specialization link index contains a foreign or conflicting row");
  }

  const auto function_counts = counts(module.function_order_.ids_);
  std::unordered_set<FunctionId> live_functions;
  std::unordered_map<std::string, FunctionId> live_names;

  for (std::size_t slot_index = 0; slot_index < module.functions_.slots_.size();
       ++slot_index) {
    const auto& slot = module.functions_.slots_[slot_index];
    if (!slot.value) continue;
    const FunctionId function_id{module.epoch_,
                                 static_cast<SlotIndex>(slot_index),
                                 slot.generation};
    live_functions.insert(function_id);
    const auto count_it = function_counts.find(function_id);
    if (!function_id.valid() || !module.functions_.contains(module.epoch_, function_id))
      report(result, VerificationRule::FunctionStorageAndOrder, function_id,
             function_id, "live function does not resolve by exact ID");
    if (count_it == function_counts.end() || count_it->second != 1)
      report(result, VerificationRule::FunctionStorageAndOrder, function_id,
             function_id, "live function must appear exactly once in module order");

    const auto& function = *slot.value;
    if (!is_well_formed(function.signature_.return_type))
      report(result, VerificationRule::BoundedAlternative, function_id,
             function_id, "function return type is malformed");
    for (const auto& type : function.signature_.parameter_types)
      if (!is_well_formed(type))
        report(result, VerificationRule::BoundedAlternative, function_id,
               function_id, "function parameter type is malformed");

    if (function.is_internal_ && !function.can_elide_if_unreferenced_)
      report(result, VerificationRule::FunctionMetadata, function_id,
             function_id,
             "internal function must remain eligible for unreferenced elision");
    if (function.is_declaration_ &&
        (function.is_internal_ || function.can_elide_if_unreferenced_))
      report(result, VerificationRule::FunctionMetadata, function_id,
             function_id,
             "declaration cannot carry definition-only linkage/elision metadata");

    if (function.link_name_.empty())
      report(result, VerificationRule::LinkNameIndex, function_id, function_id,
             "function link name must be nonempty");
    const auto inserted_name = live_names.emplace(function.link_name_, function_id);
    if (!inserted_name.second)
      report(result, VerificationRule::LinkNameIndex, function_id,
             LinkNameEntity{function.link_name_},
             "live function link names must be unique");

    const auto block_counts = counts(function.block_order_.ids_);
    std::unordered_set<BlockId> live_blocks;
    for (std::size_t block_slot = 0; block_slot < function.blocks_.slots_.size();
         ++block_slot) {
      const auto& block_storage = function.blocks_.slots_[block_slot];
      if (!block_storage.value) continue;
      const BlockId block_id{function_id, static_cast<SlotIndex>(block_slot),
                             block_storage.generation};
      live_blocks.insert(block_id);
      const auto count_it = block_counts.find(block_id);
      if (!block_id.valid() || !function.blocks_.contains(function_id, block_id))
        report(result, VerificationRule::BlockStorageAndOrder, function_id,
               block_id, "live block does not resolve by exact owner and ID");
      if (count_it == block_counts.end() || count_it->second != 1)
        report(result, VerificationRule::BlockStorageAndOrder, function_id,
               block_id, "live block must appear exactly once in block order");
    }
    for (const auto block_id : function.block_order_.ids_) {
      if (block_id.owner != function_id ||
          !function.blocks_.contains(function_id, block_id))
        report(result, VerificationRule::BlockStorageAndOrder, function_id,
               block_id, "block order contains a foreign or unresolved ID");
    }

    std::unordered_map<InstId, std::size_t> instruction_membership;
    for (const auto block_id : function.block_order_.ids_) {
      const auto resolved_block = function.blocks_.get(function_id, block_id);
      if (!resolved_block) continue;
      for (const auto instruction :
           resolved_block.value().get().instruction_order_.ids_) {
        ++instruction_membership[instruction];
        if (instruction.owner != function_id ||
            !function.insts_.contains(function_id, instruction))
          report(result, VerificationRule::InstructionStorageAndOrder,
                 function_id, instruction,
                 "instruction order contains a foreign or unresolved ID");
      }
    }

    for (std::size_t inst_slot = 0; inst_slot < function.insts_.slots_.size();
         ++inst_slot) {
      const auto& inst_storage = function.insts_.slots_[inst_slot];
      if (!inst_storage.value) continue;
      const InstId inst_id{function_id, static_cast<SlotIndex>(inst_slot),
                           inst_storage.generation};
      if (!inst_id.valid() || !function.insts_.contains(function_id, inst_id))
        report(result, VerificationRule::InstructionStorageAndOrder,
               function_id, inst_id,
               "live instruction does not resolve by exact owner and ID");
      if (instruction_membership[inst_id] != 1)
        report(result, VerificationRule::InstructionStorageAndOrder,
               function_id, inst_id,
               "live instruction must appear once in exactly one block order");
      const auto& instruction = *inst_storage.value;
      if (instruction.payload.valueless_by_exception() ||
          !opcode_matches_payload(instruction)) {
        report(result, VerificationRule::BoundedAlternative, function_id,
               inst_id,
               "instruction opcode and closed payload alternative disagree");
      }
      for (const auto operand : instruction.operands)
        if (operand.owner != function_id ||
            !function.values_.contains(function_id, operand))
          report(result, VerificationRule::ValueDefinition, function_id,
                 operand, "instruction operand does not resolve in its owner");
      if (const auto* store = std::get_if<StoreNode>(&instruction.payload)) {
        const bool destination_resolves =
            store->destination.valid() &&
            store->destination.epoch == module.epoch_ &&
            store->destination.slot < module.globals_.size();
        const ValueDef* operand = nullptr;
        if (instruction.operands.size() == 1) {
          const auto resolved =
              function.values_.get(function_id, instruction.operands[0]);
          if (resolved) operand = &resolved.value().get();
        }
        if (instruction.operands.size() != 1 ||
            !instruction.results.empty() || !destination_resolves ||
            !is_well_formed(store->stored_type) ||
            !integer_type(store->stored_type) || !operand ||
            (operand && operand->type != store->stored_type) ||
            (destination_resolves &&
             module.globals_[store->destination.slot].object_type !=
                 store->stored_type))
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "store must have one typed integer value use, no results, and one exact global destination");
      }
      if (const auto* load = std::get_if<LoadNode>(&instruction.payload)) {
        const bool source_resolves =
            load->source.valid() && load->source.epoch == module.epoch_ &&
            load->source.slot < module.globals_.size();
        const ValueDef* result_value = nullptr;
        if (instruction.results.size() == 1) {
          const auto resolved =
              function.values_.get(function_id, instruction.results[0]);
          if (resolved) result_value = &resolved.value().get();
        }
        if (!instruction.operands.empty() || instruction.results.size() != 1 ||
            !source_resolves || !is_well_formed(load->loaded_type) ||
            !integer_type(load->loaded_type) || !result_value ||
            (result_value && result_value->type != load->loaded_type) ||
            (result_value &&
             (!result_value->source_id ||
              result_value->source_id->owner != function_id)) ||
            (source_resolves &&
             module.globals_[load->source.slot].object_type !=
                 load->loaded_type))
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "load must have no operands, one source-backed typed integer result, and one exact global source");
      }
      if (const auto* gep =
              std::get_if<GetElementPtrNode>(&instruction.payload)) {
        const bool base_resolves =
            gep->base.valid() && gep->base.epoch == module.epoch_ &&
            gep->base.slot < module.globals_.size();
        bool indices_resolve = !instruction.operands.empty();
        for (const auto index : instruction.operands) {
          const auto value = function.values_.get(function_id, index);
          indices_resolve =
              indices_resolve && index.owner == function_id && value &&
              integer_type(value.value().get().type);
        }
        const ValueDef* result_value = nullptr;
        if (instruction.results.size() == 1) {
          const auto resolved =
              function.values_.get(function_id, instruction.results[0]);
          if (resolved) result_value = &resolved.value().get();
        }
        if (!indices_resolve || instruction.results.size() != 1 ||
            !base_resolves || !is_well_formed(gep->element_type) ||
            gep->element_type.kind != TypeKind::Array || !result_value ||
            (result_value &&
             result_value->type != Type{TypeKind::Pointer}) ||
            (result_value &&
             (!result_value->source_id ||
              result_value->source_id->owner != function_id)) ||
            (base_resolves &&
             module.globals_[gep->base.slot].object_type !=
                 gep->element_type))
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "getelementptr must have one exact global array base, nonempty ordered integer indices, and one source-backed pointer result");
      }
      if (const auto* call = std::get_if<CallNode>(&instruction.payload)) {
        const auto callee =
            module.functions_.get(module.epoch_, call->callee);
        const bool exact_signature =
            callee && !callee.value().get().signature_.is_variadic;
        bool exact_arguments = exact_signature &&
            instruction.operands.size() ==
                callee.value().get().signature_.parameter_types.size();
        if (exact_arguments) {
          for (std::size_t index = 0; index < instruction.operands.size(); ++index) {
            const auto argument = function.values_.get(function_id, instruction.operands[index]);
            exact_arguments = argument &&
                argument.value().get().type ==
                    callee.value().get().signature_.parameter_types[index];
            if (!exact_arguments) break;
          }
        }
        const bool exact_result = exact_signature &&
            ((callee.value().get().signature_.return_type.kind == TypeKind::Void &&
              instruction.results.empty()) ||
             (callee.value().get().signature_.return_type.kind != TypeKind::Void &&
              instruction.results.size() == 1 &&
              [&] {
                const auto value = function.values_.get(function_id, instruction.results[0]);
                return value && value.value().get().type ==
                    callee.value().get().signature_.return_type &&
                    value.value().get().source_id.has_value();
              }()));
        if (!exact_arguments || !exact_result)
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "call must target one module-owned nonvariadic function with exact ordered operands and result arity");
      }
      if (const auto* binary = std::get_if<BinaryNode>(&instruction.payload)) {
        const Type f64{TypeKind::F64, 64, "double"};
        const Type i32{TypeKind::Integer, 32, "i32"};
        const bool fadd = binary->opcode == BinaryOpcode::FAdd && binary->type == f64;
        const bool add = binary->opcode == BinaryOpcode::Add && binary->type == i32;
        bool exact = (fadd || add) && instruction.operands.size() == 2 &&
            instruction.results.size() == 1;
        if (exact) {
          for (const auto operand_id : instruction.operands) {
            const auto operand = function.values_.get(function_id, operand_id);
            exact = operand && operand.value().get().type == binary->type;
            if (!exact) break;
          }
        }
        if (exact) {
          const auto lhs = function.values_.get(function_id, instruction.operands[0]);
          const auto* lhs_def = lhs
              ? std::get_if<InstResultDef>(&lhs.value().get().definition)
              : nullptr;
          if (!lhs_def) {
            exact = false;
          } else {
            const auto producer =
                function.insts_.get(function_id, lhs_def->instruction);
            exact = producer && (fadd
                ? std::holds_alternative<CallNode>(producer.value().get().payload)
                : std::holds_alternative<LoadNode>(producer.value().get().payload));
        }
        if (exact && add) {
          const auto rhs = function.values_.get(function_id, instruction.operands[1]);
          const auto* rhs_constant = rhs
              ? std::get_if<ConstantDef>(&rhs.value().get().definition)
              : nullptr;
          const auto* integer = rhs_constant && rhs_constant->constant.valid() &&
                  rhs_constant->constant.epoch == module.epoch_ &&
                  rhs_constant->constant.slot < module.constants_.size()
              ? std::get_if<IntegerConstant>(
                    &module.constants_[rhs_constant->constant.slot].payload)
              : nullptr;
          exact = integer && integer->value == 1;
          }
        }
        if (exact) {
          const auto result_value =
              function.values_.get(function_id, instruction.results[0]);
          exact = result_value && result_value.value().get().type == binary->type &&
              result_value.value().get().source_id.has_value() &&
              result_value.value().get().source_id->owner == function_id;
        }
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "binary must retain an exact admitted typed operand shape and source-backed result");
      }
      if (const auto* call = std::get_if<IntrinsicCallNode>(&instruction.payload)) {
        const bool count_flag = call->kind == IntrinsicKind::Cttz ||
                                call->kind == IntrinsicKind::Ctlz;
        bool exact = call->callee_link_name.epoch == module.epoch_ &&
            call->callee_link_name.slot < module.link_names_.size() &&
            integer_type(call->type) && instruction.operands.size() == (count_flag ? 2U : 1U) &&
            call->zero_count_is_undef.has_value() == count_flag &&
            instruction.results.size() == 1;
        for (std::size_t index = 0; exact && index < instruction.operands.size(); ++index) {
          const auto value = function.values_.get(function_id, instruction.operands[index]);
          exact = value && value.value().get().type ==
              (index == 0 ? call->type : Type{TypeKind::Integer, 1, "i1"});
        }
        if (exact) {
          const auto value = function.values_.get(function_id, instruction.results[0]);
          exact = value && value.value().get().type == call->type &&
              value.value().get().source_id.has_value() &&
              value.value().get().source_id->owner == function_id;
        }
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id,
                           inst_id, "intrinsic call must retain an exact native integer signature, LinkNameId, operands, and source-backed result");
      }
      if (const auto* cast = std::get_if<CastNode>(&instruction.payload)) {
        const Type i64{TypeKind::Integer, 64, "i64"};
        const Type i32{TypeKind::Integer, 32, "i32"};
        bool exact = cast->kind == CastKind::Trunc && cast->from_type == i64 &&
            cast->to_type == i32 && instruction.operands.size() == 1 &&
            instruction.results.size() == 1;
        if (exact) {
          const auto operand = function.values_.get(function_id, instruction.operands[0]);
          exact = operand && operand.value().get().type == i64;
          const auto* def = exact ? std::get_if<InstResultDef>(&operand.value().get().definition) : nullptr;
          if (!def) exact = false;
          if (exact) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            exact = producer && std::holds_alternative<IntrinsicCallNode>(producer.value().get().payload);
          }
        }
        if (exact) { const auto result_value = function.values_.get(function_id, instruction.results[0]); exact = result_value && result_value.value().get().type == i32 && result_value.value().get().source_id.has_value() && result_value.value().get().source_id->owner == function_id; }
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "cast must retain the exact i64 intrinsic-result to i32 Trunc receipt");
      }
      for (std::size_t result_index = 0;
           result_index < instruction.results.size(); ++result_index) {
        const auto value_id = instruction.results[result_index];
        const auto value = function.values_.get(function_id, value_id);
        if (!value) {
          report(result, VerificationRule::ValueDefinition, function_id,
                 value_id, "instruction result does not resolve in its owner");
          continue;
        }
        const auto* definition = std::get_if<InstResultDef>(
            &value.value().get().definition);
        if (!definition || definition->instruction != inst_id ||
            definition->result_index != result_index)
          report(result, VerificationRule::ValueDefinition, function_id,
                 value_id, "instruction result has an incoherent definition");
      }
    }

    if (function.parameters_.size() !=
        function.signature_.parameter_types.size())
      report(result, VerificationRule::ParameterDefinition, function_id,
             function_id,
             "parameter order size must match signature parameter count");
    std::unordered_set<ValueId> parameters;
    for (std::size_t ordinal = 0; ordinal < function.parameters_.size();
         ++ordinal) {
      const auto parameter_id = function.parameters_[ordinal];
      if (!parameters.insert(parameter_id).second)
        report(result, VerificationRule::ParameterDefinition, function_id,
               parameter_id, "parameter must appear once in signature order");
      const auto parameter = function.values_.get(function_id, parameter_id);
      if (!parameter) {
        report(result, VerificationRule::ParameterDefinition, function_id,
               parameter_id, "parameter value does not resolve in its owner");
        continue;
      }
      const auto& value = parameter.value().get();
      const auto* definition = std::get_if<ParameterDef>(&value.definition);
      if (parameter_id.owner != function_id ||
          parameter_id.kind != ValueKind::Parameter ||
          value.kind != ValueKind::Parameter || !definition ||
          definition->ordinal != ordinal ||
          ordinal >= function.signature_.parameter_types.size() ||
          value.type != function.signature_.parameter_types[ordinal])
        report(result, VerificationRule::ParameterDefinition, function_id,
               parameter_id,
               "parameter kind, ordinal, or type does not match its signature");
    }

    const auto value_counts = counts(function.value_order_.ids_);

    for (std::size_t value_slot = 0; value_slot < function.values_.slots_.size();
         ++value_slot) {
      const auto& value_storage = function.values_.slots_[value_slot];
      if (!value_storage.value) continue;
      const auto& value = *value_storage.value;
      const ValueId value_id{function_id, value.kind,
                             static_cast<SlotIndex>(value_slot),
                             value_storage.generation};
      if (!value_id.valid() || !known(value.kind) ||
          !function.values_.contains(function_id, value_id))
        report(result, VerificationRule::ValueDefinition, function_id,
               value_id, "live value does not resolve by exact owner/kind/ID");
      const auto ordered = value_counts.find(value_id);
      if (ordered == value_counts.end() || ordered->second != 1)
        report(result, VerificationRule::ValueDefinition, function_id,
               value_id, "live value must appear exactly once in value order");
      if (!is_well_formed(value.type))
        report(result, VerificationRule::BoundedAlternative, function_id,
               value_id, "value type is malformed");
      if (value.definition.valueless_by_exception()) {
        report(result, VerificationRule::BoundedAlternative, function_id,
               value_id, "value definition has no known variant alternative");
      } else if (const auto* parameter =
                     std::get_if<ParameterDef>(&value.definition)) {
        if (value.kind != ValueKind::Parameter ||
            parameter->ordinal >= function.parameters_.size() ||
            function.parameters_[parameter->ordinal] != value_id)
          report(result, VerificationRule::ValueDefinition, function_id,
                 value_id, "parameter definition does not resolve through order");
      } else if (const auto* inst_result =
                     std::get_if<InstResultDef>(&value.definition)) {
        const auto instruction =
            function.insts_.get(function_id, inst_result->instruction);
        if (value.kind != ValueKind::Ordinary || !instruction ||
            inst_result->result_index >= instruction.value().get().results.size() ||
            instruction.value().get().results[inst_result->result_index] != value_id)
          report(result, VerificationRule::ValueDefinition, function_id,
                 value_id, "instruction-result definition is incoherent");
      } else if (std::holds_alternative<UnresolvedDef>(value.definition)) {
        report(result, VerificationRule::ValueDefinition, function_id,
               value_id, "reserved ordinary value has no definition");
      } else if (const auto* constant =
                     std::get_if<ConstantDef>(&value.definition)) {
        if (value.kind != ValueKind::Ordinary || !constant->constant.valid() ||
            constant->constant.epoch != module.epoch_ ||
            constant->constant.slot >= module.constants_.size() ||
            (constant->constant.slot < module.constants_.size() &&
             module.constants_[constant->constant.slot].type != value.type)) {
          report(result, VerificationRule::ValueDefinition, function_id,
                 value_id,
                 "constant value definition must resolve with its exact type");
        } else {
          ++constant_references[constant->constant];
        }
      }
      if (value.source_id) {
        const auto indexed =
            function.values_by_source_id_.find(value.source_id->value);
        if (!value.source_id->valid() || value.source_id->owner != function_id ||
            indexed == function.values_by_source_id_.end() ||
            indexed->second != value_id)
          report(result, VerificationRule::SourceValueIndex, function_id,
                 *value.source_id,
                 "source value identity must resolve to the exact ordinary value");
      }
    }

    for (const auto value_id : function.value_order_.ids_)
      if (value_id.owner != function_id ||
          !function.values_.contains(function_id, value_id))
        report(result, VerificationRule::ValueDefinition, function_id,
               value_id, "value order contains a foreign or unresolved ID");


    std::size_t source_value_count = 0;
    for (const auto& slot : function.values_.slots_)
      if (slot.value && slot.value->source_id) ++source_value_count;
    if (source_value_count != function.values_by_source_id_.size())
      report(result, VerificationRule::SourceValueIndex, function_id,
             function_id,
             "source value index size must match source-backed live values");
    for (const auto& entry : function.values_by_source_id_) {
      const auto value = function.values_.get(function_id, entry.second);
      if (!value || entry.second.kind != ValueKind::Ordinary ||
          !value.value().get().source_id ||
          value.value().get().source_id->owner != function_id ||
          value.value().get().source_id->value != entry.first)
        report(result, VerificationRule::SourceValueIndex, function_id,
               SourceValueId{function_id, entry.first},
               "source value index contains a foreign or conflicting definition");
    }

    if (function.is_declaration_) {
      if (!live_blocks.empty())
        report(result, VerificationRule::FunctionShape, function_id,
               function_id, "declaration must not contain blocks");
    } else if (live_blocks.empty()) {
      report(result, VerificationRule::FunctionShape, function_id, function_id,
             "definition must contain at least one block");
    }

    for (const auto block_id : live_blocks) {
      const auto block = function.blocks_.get(function_id, block_id);
      if (!block) continue;
      const auto& terminator = block.value().get().terminator_;
      if (!terminator) {
        report(result, VerificationRule::FunctionShape, function_id, block_id,
               "definition block must have exactly one terminator");
        continue;
      }
      if (terminator->valueless_by_exception()) {
        report(result, VerificationRule::BoundedAlternative, function_id,
               block_id, "terminator has no known variant alternative");
        continue;
      }
      std::visit(
          [&](const auto& term) {
            using Term = std::decay_t<decltype(term)>;
            if constexpr (std::is_same_v<Term, JumpTerm>) {
              if (term.target.owner != function_id ||
                  !function.blocks_.contains(function_id, term.target))
                report(result, VerificationRule::Terminator, function_id,
                       block_id, "jump target is foreign or unresolved");
            } else if constexpr (std::is_same_v<Term, CondJumpTerm>) {
              const auto condition =
                  function.values_.get(function_id, term.condition);
              if (term.condition.owner != function_id || !condition ||
                  condition.value().get().type != Type{TypeKind::I1})
                report(result, VerificationRule::Terminator, function_id,
                       block_id,
                       "conditional jump condition must resolve to local I1");
              if (term.true_target.owner != function_id ||
                  !function.blocks_.contains(function_id, term.true_target) ||
                  term.false_target.owner != function_id ||
                  !function.blocks_.contains(function_id, term.false_target))
                report(result, VerificationRule::Terminator, function_id,
                       block_id,
                       "conditional jump targets must resolve in their owner");
            } else if constexpr (std::is_same_v<Term, ReturnTerm>) {
              const bool returns_void =
                  function.signature_.return_type.kind == TypeKind::Void;
              if (returns_void != !term.value) {
                report(result, VerificationRule::Terminator, function_id,
                       block_id, "return arity does not match signature");
              } else if (term.value) {
                const auto value = function.values_.get(function_id, *term.value);
                if (term.value->owner != function_id || !value ||
                    value.value().get().type != function.signature_.return_type)
                  report(result, VerificationRule::Terminator, function_id,
                         block_id,
                         "return value must resolve locally with signature type");
              }
            }
          },
          *terminator);
    }
  }

  for (const auto function_id : module.function_order_.ids_) {
    if (function_id.epoch != module.epoch_ ||
        !module.functions_.contains(module.epoch_, function_id))
      report(result, VerificationRule::FunctionStorageAndOrder, function_id,
             function_id, "module order contains a foreign or unresolved ID");
  }

  for (std::size_t index = 0; index < module.constants_.size(); ++index) {
    const ConstantId id{module.epoch_, static_cast<SlotIndex>(index)};
    if (constant_references[id] != 1)
      report(result, VerificationRule::ConstantDefinition, {}, id,
             "each constant must define exactly one ordinary value");
  }

  if (module.functions_by_link_name_.size() != live_names.size())
    report(result, VerificationRule::LinkNameIndex, {}, ModuleEntity{},
           "link-name index size must exactly match unique live functions");
  for (const auto& entry : module.functions_by_link_name_) {
    const auto live = live_names.find(entry.first);
    if (entry.first.empty() || live == live_names.end() ||
        live->second != entry.second || entry.second.epoch != module.epoch_ ||
        !module.functions_.contains(module.epoch_, entry.second))
      report(result, VerificationRule::LinkNameIndex, entry.second,
             LinkNameEntity{entry.first},
             "link-name index entry does not exactly match live function state");
  }
  for (const auto& entry : live_names) {
    const auto indexed = module.functions_by_link_name_.find(entry.first);
    if (indexed == module.functions_by_link_name_.end() ||
        indexed->second != entry.second)
      report(result, VerificationRule::LinkNameIndex, entry.second,
             LinkNameEntity{entry.first},
             "live function is absent or mismatched in link-name index");
  }

  return result;
}

VerificationResult FoundationVerifier::verify(const RawBir& raw,
                                               VerifyProfile profile) {
  return verify(*raw.data_, profile);
}

}  // namespace c4c::backend::bir
