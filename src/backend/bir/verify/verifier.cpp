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
      return std::holds_alternative<StoreNode>(instruction.payload) ||
             std::holds_alternative<LocalStoreAuthorityNode>(instruction.payload);
    case Opcode::Load:
      return std::holds_alternative<LoadNode>(instruction.payload) ||
             std::holds_alternative<LocalLoadAuthorityNode>(instruction.payload);
    case Opcode::GetElementPtr:
      return std::holds_alternative<GetElementPtrNode>(instruction.payload) ||
             std::holds_alternative<LocalArrayGepAuthorityNode>(instruction.payload);
    case Opcode::StackSaveAuthority:
      return std::holds_alternative<StackSaveAuthorityNode>(instruction.payload);
    case Opcode::StackRestoreAuthority:
      return std::holds_alternative<StackRestoreAuthorityNode>(instruction.payload);
    case Opcode::Abs:
      return std::holds_alternative<AbsNode>(instruction.payload);
    case Opcode::Call:
      return std::holds_alternative<CallNode>(instruction.payload) ||
             std::holds_alternative<IntrinsicCallNode>(instruction.payload);
    case Opcode::Binary:
      return std::holds_alternative<BinaryNode>(instruction.payload);
    case Opcode::Compare:
      return std::holds_alternative<CompareNode>(instruction.payload);
    case Opcode::Select:
      return std::holds_alternative<SelectNode>(instruction.payload);
    case Opcode::SelectedMemcpy:
      return std::holds_alternative<SelectedMemcpyNode>(instruction.payload);
    case Opcode::Amd64SysVOverflowAggregateMemcpy:
      return std::holds_alternative<Amd64SysVOverflowAggregateMemcpyNode>(instruction.payload);
    case Opcode::Cast:
      return std::holds_alternative<CastNode>(instruction.payload);
    case Opcode::Phi:
      return std::holds_alternative<PhiNode>(instruction.payload);
    case Opcode::AllocaAuthority:
      return std::holds_alternative<AllocaAuthorityNode>(instruction.payload);
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
    const bool special = std::holds_alternative<SpecialConstant>(constant.payload);
    if ((!special && std::holds_alternative<IntegerConstant>(constant.payload) !=
             integer_type(constant.type)) ||
        (!special && std::holds_alternative<FloatingConstant>(constant.payload) !=
             floating_type(constant.type)) ||
        (std::holds_alternative<LabelAddressConstant>(constant.payload) &&
             constant.type.kind != TypeKind::Pointer) ||
        (std::holds_alternative<SpecialConstant>(constant.payload) &&
             [&] {
               const auto kind = std::get<SpecialConstant>(constant.payload).kind;
               if (kind == SpecialConstantKind::Null) return constant.type.kind != TypeKind::Pointer;
               if (kind == SpecialConstantKind::True || kind == SpecialConstantKind::False)
                 return constant.type != Type{TypeKind::I1, 1, "i1"};
               return static_cast<unsigned>(kind) > static_cast<unsigned>(SpecialConstantKind::False);
             }()))
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
    std::unordered_map<InstId, BlockId> instruction_blocks;
    for (const auto block_id : function.block_order_.ids_) {
      const auto resolved_block = function.blocks_.get(function_id, block_id);
      if (!resolved_block) continue;
      for (const auto instruction :
           resolved_block.value().get().instruction_order_.ids_) {
        ++instruction_membership[instruction];
        instruction_blocks.emplace(instruction, block_id);
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
      const auto containing_block = instruction_blocks.find(inst_id);
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
      if (const auto* phi = std::get_if<PhiNode>(&instruction.payload)) {
        bool exact = is_well_formed(phi->type) && phi->type.kind != TypeKind::Void &&
            instruction.results.size() == 1 &&
            instruction.operands.size() == phi->incoming.size() && !phi->incoming.empty();
        const auto result_value = exact
            ? function.values_.get(function_id, instruction.results[0])
            : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(ResolveError::OutOfRange);
        exact = exact && result_value && result_value.value().get().type == phi->type &&
            result_value.value().get().source_id.has_value() &&
            result_value.value().get().source_id->owner == function_id;
        std::unordered_set<std::uint64_t> seen;
        for (std::size_t index = 0; exact && index < phi->incoming.size(); ++index) {
          const auto& incoming = phi->incoming[index];
          const auto value = function.values_.get(function_id, incoming.value);
          const auto predecessor = function.blocks_.get(function_id, incoming.edge.predecessor);
          exact = value && value.value().get().type == phi->type && predecessor &&
              containing_block != instruction_blocks.end() &&
              incoming.edge.destination == containing_block->second && instruction.operands[index] == incoming.value;
          if (!exact) break;
          const auto term = predecessor.value().get().terminator_;
          std::vector<BlockId> successors;
          if (term) successors = std::visit([](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, JumpTerm>) return std::vector<BlockId>{item.target};
            else if constexpr (std::is_same_v<T, CondJumpTerm>) return std::vector<BlockId>{item.true_target, item.false_target};
            else if constexpr (std::is_same_v<T, IndirectJumpTerm>) return item.targets;
            else if constexpr (std::is_same_v<T, SwitchTerm>) { std::vector<BlockId> r{item.default_target}; r.insert(r.end(), item.case_targets.begin(), item.case_targets.end()); return r; }
            else return std::vector<BlockId>{};
          }, *term);
          const auto occurrence_count = static_cast<std::size_t>(std::count(
              successors.begin(), successors.end(), containing_block->second));
          exact = incoming.edge.occurrence < occurrence_count;
          const auto key = (static_cast<std::uint64_t>(incoming.edge.predecessor.slot) << 32) | incoming.edge.occurrence;
          exact = exact && seen.insert(key).second;
        }
        if (exact) {
          std::size_t expected = 0;
          for (const auto predecessor_id : function.block_order_.ids_) {
            const auto predecessor = function.blocks_.get(function_id, predecessor_id);
            if (!predecessor || !predecessor.value().get().terminator_) continue;
            const auto successors = std::visit([](const auto& item) {
              using T = std::decay_t<decltype(item)>;
              if constexpr (std::is_same_v<T, JumpTerm>) return std::vector<BlockId>{item.target};
              else if constexpr (std::is_same_v<T, CondJumpTerm>) return std::vector<BlockId>{item.true_target, item.false_target};
              else if constexpr (std::is_same_v<T, IndirectJumpTerm>) return item.targets;
              else if constexpr (std::is_same_v<T, SwitchTerm>) { std::vector<BlockId> r{item.default_target}; r.insert(r.end(), item.case_targets.begin(), item.case_targets.end()); return r; }
              else return std::vector<BlockId>{};
            }, *predecessor.value().get().terminator_);
            expected += static_cast<std::size_t>(std::count(successors.begin(), successors.end(), containing_block->second));
          }
          exact = expected == phi->incoming.size();
        }
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "phi must bind each typed incoming to one exact current CFG edge occurrence");
      }
      if (const auto* alloca = std::get_if<AllocaAuthorityNode>(&instruction.payload)) {
        const auto result_value = instruction.results.size() == 1
            ? function.values_.get(function_id, instruction.results[0])
            : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(ResolveError::OutOfRange);
        const bool exact = alloca->result.valid() &&
            alloca->pointer_definition == alloca->result && alloca->result.owner == function_id &&
            alloca->object.valid() && alloca->object.owner == function_id && alloca->owner.valid() &&
            alloca->owner.epoch == module.epoch_ && alloca->owner.slot < module.link_names_.size() &&
            alloca->pointer_type == Type{TypeKind::Pointer} &&
            is_well_formed(alloca->pointee_type) && alloca->pointee_type.kind != TypeKind::Void &&
            alloca->live && result_value && result_value.value().get().type == alloca->pointer_type &&
            result_value.value().get().source_id == alloca->result;
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "alloca authority must retain one live current-function typed pointer/object binding");
      }
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
      if (const auto* store = std::get_if<LocalStoreAuthorityNode>(&instruction.payload)) {
        const ValueDef* pointer = nullptr;
        if (instruction.operands.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.operands[0]);
          if (resolved) pointer = &resolved.value().get();
        }
        const auto* pointer_result = pointer ? std::get_if<InstResultDef>(&pointer->definition) : nullptr;
        const auto pointer_inst = pointer_result ? function.insts_.get(function_id, pointer_result->instruction)
            : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
        const auto* alloca = pointer_inst ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload) : nullptr;
        const bool owner = store->owner.valid() && store->owner.epoch == module.epoch_ && store->owner.slot < module.link_names_.size();
        const bool exact = instruction.results.empty() && store->pointer_definition.valid() && store->object.valid() &&
            store->pointer_definition.owner == function_id && store->object.owner == function_id && owner &&
            store->pointer_type == Type{TypeKind::Pointer} && is_well_formed(store->stored_type) &&
            integer_type(store->stored_type) && store->live && pointer &&
            pointer->source_id == store->pointer_definition && pointer->type == store->pointer_type && alloca &&
            alloca->pointer_definition == store->pointer_definition && alloca->object.owner == store->object.owner &&
            alloca->object.value == store->object.value && alloca->owner == store->owner &&
            alloca->pointee_type == store->stored_type && alloca->live;
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "local store must retain one live typed current-function pointer/object authority and native immediate");
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
      if (const auto* load = std::get_if<LocalLoadAuthorityNode>(&instruction.payload)) {
        const ValueDef* pointer = nullptr;
        const ValueDef* result_value = nullptr;
        if (instruction.operands.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.operands[0]);
          if (resolved) pointer = &resolved.value().get();
        }
        if (instruction.results.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.results[0]);
          if (resolved) result_value = &resolved.value().get();
        }
        const bool owner_resolves = load->owner.valid() && load->owner.epoch == module.epoch_ &&
            load->owner.slot < module.link_names_.size();
        const auto* pointer_result = pointer
            ? std::get_if<InstResultDef>(&pointer->definition) : nullptr;
        const auto pointer_inst = pointer_result
            ? function.insts_.get(function_id, pointer_result->instruction)
            : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
        const auto* alloca = pointer_inst
            ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload) : nullptr;
        const bool exact = load->result.valid() && load->pointer_definition.valid() &&
            load->object.valid() && load->result.owner == function_id &&
            load->pointer_definition.owner == function_id && load->object.owner == function_id &&
            owner_resolves && load->pointer_type == Type{TypeKind::Pointer} &&
            is_well_formed(load->loaded_type) &&
            (integer_type(load->loaded_type) || floating_type(load->loaded_type)) && load->live &&
            pointer && pointer->source_id == load->pointer_definition &&
            pointer->type == load->pointer_type && result_value &&
            result_value->source_id == load->result && result_value->type == load->loaded_type &&
            alloca && alloca->result == load->pointer_definition &&
            alloca->pointer_definition == load->pointer_definition &&
            alloca->object.owner == load->object.owner && alloca->object.value == load->object.value &&
            alloca->owner == load->owner && alloca->pointer_type == load->pointer_type &&
            alloca->pointee_type == load->loaded_type && alloca->live;
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "local load must retain one live typed current-function pointer/object authority and exact result");
      }
      if (const auto* memcpy = std::get_if<SelectedMemcpyNode>(&instruction.payload)) {
        const bool owner_resolves = memcpy->destination_object_owner.valid() &&
            memcpy->destination_object_owner == memcpy->source_object_owner &&
            memcpy->destination_object_owner.epoch == module.epoch_ &&
            memcpy->destination_object_owner.slot < module.link_names_.size();
        const bool exact = instruction.operands.empty() && instruction.results.empty() &&
            memcpy->destination.valid() && memcpy->source.valid() &&
            memcpy->destination.owner == function_id && memcpy->source.owner == function_id &&
            memcpy->destination.value != memcpy->source.value &&
            memcpy->destination_object.valid() &&
            memcpy->source_object.valid() && memcpy->destination_object.owner == function_id &&
            memcpy->source_object.owner == function_id &&
            memcpy->destination_object.value != memcpy->source_object.value &&
            owner_resolves && memcpy->pointer_type == Type{TypeKind::Pointer} &&
            memcpy->size_bytes > 0 && memcpy->destination_live_at_site &&
            memcpy->source_live_at_site;
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "selected memcpy must retain exact current-function pointer, object, owner, i64-positive-size, and live-site authority");
      }
      if (const auto* memcpy =
              std::get_if<Amd64SysVOverflowAggregateMemcpyNode>(&instruction.payload)) {
        const bool owner_resolves = memcpy->owner.valid() &&
            memcpy->owner.epoch == module.epoch_ &&
            memcpy->owner.slot < module.link_names_.size();
        const bool exact = instruction.operands.empty() && instruction.results.empty() &&
            memcpy->va_list_pointer.valid() && memcpy->va_list_object.valid() &&
            memcpy->overflow_field_address.valid() && memcpy->overflow_pointer_load.valid() &&
            memcpy->destination.valid() && memcpy->destination_object.valid() &&
            memcpy->final_load.valid() && memcpy->va_list_pointer.owner == function_id &&
            memcpy->va_list_object.owner == function_id &&
            memcpy->overflow_field_address.owner == function_id &&
            memcpy->overflow_pointer_load.owner == function_id &&
            memcpy->destination.owner == function_id &&
            memcpy->destination_object.owner == function_id &&
            memcpy->final_load.owner == function_id &&
            memcpy->va_list_pointer.value != memcpy->destination.value &&
            memcpy->va_list_object.value != memcpy->destination_object.value &&
            owner_resolves && memcpy->payload_type.kind == TypeKind::Struct &&
            memcpy->size_bytes > 0 && memcpy->va_list_live && memcpy->destination_live;
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "AMD64 SysV overflow aggregate memcpy must retain the exact typed local/derived-storage receipt");
      }
      if (const auto* gep =
              std::get_if<GetElementPtrNode>(&instruction.payload)) {
        bool base_resolves = false;
        bool base_matches_element_type = false;
        if (const auto* global =
                std::get_if<GlobalObjectId>(&gep->base.authority)) {
          base_resolves = global->valid() && global->epoch == module.epoch_ &&
              global->slot < module.globals_.size();
          base_matches_element_type = base_resolves &&
              module.globals_[global->slot].object_type == gep->element_type;
        } else if (const auto* label =
                       std::get_if<LabelAddressGepBase>(&gep->base.authority)) {
          const auto value = function.values_.get(function_id, label->value);
          if (value && label->value.owner == function_id) {
            const auto& definition = value.value().get();
            if (definition.kind == ValueKind::Ordinary &&
                definition.type == Type{TypeKind::Pointer}) {
              if (const auto* constant =
                      std::get_if<ConstantDef>(&definition.definition);
                  constant && constant->constant.valid() &&
                  constant->constant.epoch == module.epoch_ &&
                  constant->constant.slot < module.constants_.size()) {
                if (const auto* label_constant =
                        std::get_if<LabelAddressConstant>(
                            &module.constants_[constant->constant.slot].payload)) {
                  base_resolves = label_constant->target.owner == function_id &&
                      function.blocks_.contains(function_id, label_constant->target);
                  base_matches_element_type = base_resolves;
                }
              }
            }
          }
        } else if (const auto* parameter =
                       std::get_if<DirectPointerBodyParameterGepBase>(&gep->base.authority)) {
          const auto parameters = function.parameters_;
          if (parameter->source_value_id != 0 && parameter->owner.valid() &&
              parameter->owner.epoch == module.epoch_ &&
              parameter->owner.slot < module.link_names_.size() &&
              parameter->pointer_type == Type{TypeKind::Pointer} &&
              parameter->parameter_index < parameters.size()) {
            const auto value = function.values_.get(function_id,
                                                    parameters[parameter->parameter_index]);
            base_resolves = value && value.value().get().kind == ValueKind::Parameter &&
                value.value().get().type == parameter->pointer_type;
            base_matches_element_type = base_resolves;
          }
        }
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
        const bool direct_body_parameter =
            std::holds_alternative<DirectPointerBodyParameterGepBase>(gep->base.authority);
        if (!indices_resolve || instruction.results.size() != 1 ||
            !base_resolves || !is_well_formed(gep->element_type) ||
            (!direct_body_parameter && gep->element_type.kind != TypeKind::Array) || !result_value ||
            (result_value &&
             result_value->type != Type{TypeKind::Pointer}) ||
            (result_value &&
             (!result_value->source_id ||
              result_value->source_id->owner != function_id)) ||
            !base_matches_element_type)
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "getelementptr must have one exact global-array or current-function label-address base, nonempty ordered integer indices, and one source-backed pointer result");
      }
      if (const auto* gep =
              std::get_if<LocalArrayGepAuthorityNode>(&instruction.payload)) {
        const ValueDef* pointer = nullptr;
        const ValueDef* result_value = nullptr;
        if (instruction.operands.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.operands[0]);
          if (resolved) pointer = &resolved.value().get();
        }
        if (instruction.results.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.results[0]);
          if (resolved) result_value = &resolved.value().get();
        }
        const auto* pointer_result = pointer
            ? std::get_if<InstResultDef>(&pointer->definition) : nullptr;
        const auto pointer_inst = pointer_result
            ? function.insts_.get(function_id, pointer_result->instruction)
            : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
        const auto* alloca = pointer_inst
            ? std::get_if<AllocaAuthorityNode>(&pointer_inst.value().get().payload) : nullptr;
        const bool owner_resolves = gep->owner.valid() && gep->owner.epoch == module.epoch_ &&
            gep->owner.slot < module.link_names_.size();
        const bool exact = gep->result.valid() && gep->pointer_definition.valid() &&
            gep->object.valid() && gep->result.owner == function_id &&
            gep->pointer_definition.owner == function_id && gep->object.owner == function_id &&
            owner_resolves && gep->pointer_type == Type{TypeKind::Pointer} &&
            is_well_formed(gep->pointee_type) && gep->pointee_type.kind == TypeKind::Array &&
            is_well_formed(gep->element_type) && gep->element_type.kind != TypeKind::Array &&
            gep->live && pointer && pointer->source_id == gep->pointer_definition &&
            pointer->type == gep->pointer_type && result_value &&
            result_value->source_id == gep->result &&
            result_value->type == Type{TypeKind::Pointer} && alloca &&
            alloca->result == gep->pointer_definition &&
            alloca->pointer_definition == gep->pointer_definition &&
            alloca->object.owner == gep->object.owner && alloca->object.value == gep->object.value &&
            alloca->owner == gep->owner && alloca->pointer_type == gep->pointer_type &&
            alloca->pointee_type == gep->pointee_type && alloca->live;
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "local-array GEP must retain one live typed current-function array pointer/object authority, immediate index, and exact result");
      }
      if (const auto* stack_save =
              std::get_if<StackSaveAuthorityNode>(&instruction.payload)) {
        const ValueDef* result_value = nullptr;
        if (instruction.results.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.results[0]);
          if (resolved) result_value = &resolved.value().get();
        }
        const bool owner_resolves = stack_save->owner.valid() &&
            stack_save->owner.epoch == module.epoch_ &&
            stack_save->owner.slot < module.link_names_.size();
        const bool exact = stack_save->result.valid() &&
            stack_save->pointer_definition.valid() && stack_save->object.valid() &&
            stack_save->result == stack_save->pointer_definition &&
            stack_save->result.owner == function_id &&
            stack_save->object.owner == function_id && owner_resolves &&
            stack_save->pointer_type == Type{TypeKind::Pointer} &&
            stack_save->pointee_type == Type{TypeKind::Pointer} && stack_save->live &&
            instruction.operands.empty() && result_value &&
            result_value->source_id == stack_save->result &&
            result_value->type == Type{TypeKind::Pointer};
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "stack save must retain one live typed current-function saved-pointer/object authority and exact result");
      }
      if (const auto* stack_restore =
              std::get_if<StackRestoreAuthorityNode>(&instruction.payload)) {
        const ValueDef* saved_value = nullptr;
        const StackSaveAuthorityNode* stack_save = nullptr;
        if (instruction.operands.size() == 1) {
          const auto resolved = function.values_.get(function_id, instruction.operands.front());
          if (resolved) {
            saved_value = &resolved.value().get();
            if (const auto* definition = std::get_if<InstResultDef>(&saved_value->definition)) {
              const auto producer = function.insts_.get(function_id, definition->instruction);
              if (producer)
                stack_save = std::get_if<StackSaveAuthorityNode>(&producer.value().get().payload);
            }
          }
        }
        const bool owner_resolves = stack_restore->owner.valid() &&
            stack_restore->owner.epoch == module.epoch_ &&
            stack_restore->owner.slot < module.link_names_.size();
        const bool exact = stack_restore->saved_pointer_definition.valid() &&
            stack_restore->object.valid() &&
            stack_restore->saved_pointer_definition.owner == function_id &&
            stack_restore->object.owner == function_id && owner_resolves &&
            stack_restore->pointer_type == Type{TypeKind::Pointer} &&
            stack_restore->pointee_type == Type{TypeKind::Pointer} && stack_restore->live &&
            saved_value && saved_value->source_id == stack_restore->saved_pointer_definition &&
            saved_value->type == Type{TypeKind::Pointer} && stack_save &&
            stack_save->result == stack_restore->saved_pointer_definition &&
            stack_save->pointer_definition == stack_restore->saved_pointer_definition &&
            stack_save->object.owner == stack_restore->object.owner &&
            stack_save->object.value == stack_restore->object.value &&
            stack_save->owner == stack_restore->owner &&
            stack_save->pointer_type == stack_restore->pointer_type &&
            stack_save->pointee_type == stack_restore->pointee_type && stack_save->live;
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "stack restore must consume the matching live typed VLA stack-save checkpoint authority");
      }
      if (const auto* abs = std::get_if<AbsNode>(&instruction.payload)) {
        const Type i32{TypeKind::Integer, 32, "i32"};
        bool exact = abs->type == i32 && instruction.operands.size() == 1 &&
            instruction.results.size() == 1;
        const auto operand = exact
            ? function.values_.get(function_id, instruction.operands[0])
            : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(
                  ResolveError::OutOfRange);
        exact = exact && operand && operand.value().get().type == i32;
        const auto* operand_def = exact
            ? std::get_if<InstResultDef>(&operand.value().get().definition) : nullptr;
        const auto producer = operand_def
            ? function.insts_.get(function_id, operand_def->instruction)
            : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(
                  ResolveError::OutOfRange);
        exact = exact && producer &&
            std::holds_alternative<LoadNode>(producer.value().get().payload);
        const auto result_value = exact
            ? function.values_.get(function_id, instruction.results[0])
            : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(
                  ResolveError::OutOfRange);
        exact = exact && result_value && result_value.value().get().type == i32 &&
            result_value.value().get().source_id.has_value() &&
            result_value.value().get().source_id->owner == function_id;
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "abs must retain one selected-load i32 operand and one source-backed i32 result");
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
        const auto* direct_scalar = call->direct_scalar_argument
            ? &*call->direct_scalar_argument : nullptr;
        const bool exact_direct_scalar = !direct_scalar ||
            (exact_signature && direct_scalar->argument_index < instruction.operands.size() &&
             direct_scalar->argument_index <
                 callee.value().get().signature_.parameter_types.size() &&
             direct_scalar->source_value_id != 0 && direct_scalar->owner.valid() &&
             direct_scalar->owner.epoch == module.epoch_ &&
             direct_scalar->owner.slot < module.link_names_.size() &&
             module.link_names_[direct_scalar->owner.slot].spelling == function.link_name_ &&
             direct_scalar->parameter_index < function.parameters_.size() &&
             instruction.operands[direct_scalar->argument_index] ==
                 function.parameters_[direct_scalar->parameter_index] &&
             direct_scalar->scalar_type ==
                 callee.value().get().signature_
                     .parameter_types[direct_scalar->argument_index]);
        if (!exact_arguments || !exact_result || !exact_direct_scalar)
          report(result, VerificationRule::ValueDefinition, function_id,
                 inst_id,
                 "call must target one module-owned nonvariadic function with exact ordered operands, result arity, and retained direct-scalar argument authority");
      }
      if (const auto* binary = std::get_if<BinaryNode>(&instruction.payload)) {
        const Type f64{TypeKind::F64, 64, "double"};
        const Type i32{TypeKind::Integer, 32, "i32"};
        const Type i64{TypeKind::Integer, 64, "i64"};
        const bool fadd = binary->opcode == BinaryOpcode::FAdd && binary->type == f64;
        const bool fmul = binary->opcode == BinaryOpcode::FMul && binary->type == f64;
        const bool float_fmul = binary->opcode == BinaryOpcode::FMul &&
            binary->type == Type{TypeKind::F32, 32, "float"};
        const bool fneg = binary->opcode == BinaryOpcode::FNeg &&
            floating_type(binary->type);
        const bool add = binary->opcode == BinaryOpcode::Add && binary->type == i32;
        const bool sext_add = binary->opcode == BinaryOpcode::Add && binary->type == i64;
        const bool mul = binary->opcode == BinaryOpcode::Mul && binary->type == i32;
        bool exact = (fadd || fmul || float_fmul || add || sext_add || mul)
            ? instruction.operands.size() == 2 && instruction.results.size() == 1
            : fneg && instruction.operands.size() == 1 &&
            instruction.results.size() == 1;
        const auto* direct_scalar = binary->direct_scalar_lhs ? &*binary->direct_scalar_lhs : nullptr;
        const auto* direct_scalar_rhs = binary->direct_scalar_rhs ? &*binary->direct_scalar_rhs : nullptr;
        const bool direct_scalar_add = exact && add && direct_scalar &&
            direct_scalar->source_value_id != 0 && direct_scalar->scalar_type == i32 &&
            direct_scalar->owner.valid() && direct_scalar->owner.epoch == module.epoch_ &&
            direct_scalar->owner.slot < module.link_names_.size() &&
            module.link_names_[direct_scalar->owner.slot].spelling == function.link_name_ &&
            direct_scalar->parameter_index < function.parameters_.size() &&
            instruction.operands[0] == function.parameters_[direct_scalar->parameter_index];
        const bool direct_scalar_fneg = exact && fneg && direct_scalar &&
            direct_scalar->source_value_id != 0 &&
            direct_scalar->scalar_type == binary->type &&
            direct_scalar->owner.valid() && direct_scalar->owner.epoch == module.epoch_ &&
            direct_scalar->owner.slot < module.link_names_.size() &&
            module.link_names_[direct_scalar->owner.slot].spelling == function.link_name_ &&
            direct_scalar->parameter_index < function.parameters_.size() &&
            instruction.operands[0] == function.parameters_[direct_scalar->parameter_index];
        const bool direct_scalar_rhs_add = exact && add && direct_scalar_rhs &&
            direct_scalar_rhs->source_value_id != 0 && direct_scalar_rhs->scalar_type == i32 &&
            direct_scalar_rhs->owner.valid() && direct_scalar_rhs->owner.epoch == module.epoch_ &&
            direct_scalar_rhs->owner.slot < module.link_names_.size() &&
            module.link_names_[direct_scalar_rhs->owner.slot].spelling == function.link_name_ &&
            direct_scalar_rhs->parameter_index < function.parameters_.size() &&
            instruction.operands[1] == function.parameters_[direct_scalar_rhs->parameter_index];
        if (exact && !direct_scalar_add && !direct_scalar_fneg &&
            !direct_scalar_rhs_add) {
          for (const auto operand_id : instruction.operands) {
            const auto operand = function.values_.get(function_id, operand_id);
            exact = operand && operand.value().get().type == binary->type;
            if (!exact) break;
          }
        }
        if (exact && !direct_scalar_add && !direct_scalar_fneg &&
            !direct_scalar_rhs_add) {
          const auto lhs = function.values_.get(function_id, instruction.operands[0]);
          const auto* lhs_def = lhs
              ? std::get_if<InstResultDef>(&lhs.value().get().definition)
              : nullptr;
          if (!lhs_def) {
            exact = false;
          } else {
            const auto producer =
                function.insts_.get(function_id, lhs_def->instruction);
            const bool cttz_add = add && producer && [&] {
              const auto* intrinsic = std::get_if<IntrinsicCallNode>(
                  &producer.value().get().payload);
              return intrinsic && intrinsic->kind == IntrinsicKind::Cttz &&
                  intrinsic->type == i32;
            }();
            const bool ctlz_add = add && producer && [&] {
              const auto* intrinsic = std::get_if<IntrinsicCallNode>(
                  &producer.value().get().payload);
              return intrinsic && intrinsic->kind == IntrinsicKind::Ctlz &&
                  intrinsic->type == i32 &&
                  intrinsic->zero_count_is_undef == std::optional<bool>{true};
            }();
            const bool ctpop_add = add && producer && [&] {
              const auto* intrinsic = std::get_if<IntrinsicCallNode>(
                  &producer.value().get().payload);
              return intrinsic && intrinsic->kind == IntrinsicKind::Ctpop &&
                  intrinsic->type == i32 && !intrinsic->zero_count_is_undef.has_value();
            }();
            exact = producer && (fadd
                ? std::holds_alternative<CallNode>(producer.value().get().payload)
                : fmul ? [&] {
                    const auto* producer_binary = std::get_if<BinaryNode>(
                        &producer.value().get().payload);
                    if (producer_binary)
                      return producer_binary->opcode == BinaryOpcode::FAdd &&
                          producer_binary->type == f64;
                    const auto* producer_cast = std::get_if<CastNode>(
                        &producer.value().get().payload);
                    return producer_cast &&
                        ((producer_cast->kind == CastKind::FPExt &&
                          producer_cast->from_type == Type{TypeKind::F32, 32, "float"}) ||
                         (producer_cast->kind == CastKind::SIToFP &&
                          producer_cast->from_type == Type{TypeKind::Integer, 32, "i32"}) ||
                         (producer_cast->kind == CastKind::UIToFP &&
                          producer_cast->from_type == Type{TypeKind::Integer, 32, "i32"})) &&
                        producer_cast->to_type == f64;
                  }()
                : float_fmul ? [&] {
                    const auto* cast = std::get_if<CastNode>(&producer.value().get().payload);
                    return cast && cast->kind == CastKind::FPTrunc &&
                        cast->from_type == Type{TypeKind::F64, 64, "double"} &&
                        cast->to_type == Type{TypeKind::F32, 32, "float"};
                  }()
                : add ? (std::holds_alternative<LoadNode>(producer.value().get().payload) ||
                         std::holds_alternative<AbsNode>(producer.value().get().payload) ||
                         cttz_add || ctlz_add || ctpop_add || [&] {
                           const auto* producer_cast = std::get_if<CastNode>(
                               &producer.value().get().payload);
                           return producer_cast && (producer_cast->kind == CastKind::FPToSI ||
                               producer_cast->kind == CastKind::FPToUI ||
                               producer_cast->kind == CastKind::Trunc) &&
                               ((producer_cast->from_type == f64 && producer_cast->to_type == i32) ||
                                (producer_cast->from_type == Type{TypeKind::Integer, 64, "i64"} &&
                                 producer_cast->to_type == i32));
                         }())
                      : sext_add ? [&] {
                          const auto* intrinsic = std::get_if<IntrinsicCallNode>(
                              &producer.value().get().payload);
                          if (intrinsic && intrinsic->kind == IntrinsicKind::Cttz &&
                              intrinsic->type == i64)
                            return true;
                          const auto* cast = std::get_if<CastNode>(
                              &producer.value().get().payload);
                          return cast && cast->kind == CastKind::SExt &&
                              cast->from_type == i32 && cast->to_type == i64;
                        }()
                      : [&] {
                          const auto* producer_binary = std::get_if<BinaryNode>(
                              &producer.value().get().payload);
                          return producer_binary &&
                              producer_binary->opcode == BinaryOpcode::Add &&
                              producer_binary->type == i32;
                        }());
        }
        if (exact && (add || sext_add || mul)) {
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
          exact = integer && integer->value == ((add || sext_add) ? 1 : 2);
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
      if (const auto* compare = std::get_if<CompareNode>(&instruction.payload)) {
        const Type i1{TypeKind::I1, 1, "i1"};
        const Type i32{TypeKind::Integer, 32, "i32"};
        const Type i64{TypeKind::Integer, 64, "i64"};
        const Type f64{TypeKind::F64, 64, "double"};
        const bool slt = compare->predicate == ComparePredicate::Slt && compare->type == i32;
        const bool olt = compare->predicate == ComparePredicate::OLt && compare->type == f64;
        const bool ffs_eq_zero = compare->predicate == ComparePredicate::Eq &&
            (compare->type == i32 || compare->type == i64);
        const bool truthiness_ne = compare->predicate == ComparePredicate::Ne &&
            integer_type(compare->type) && compare->direct_scalar_truthiness_lhs.has_value();
        const bool pointer_truthiness_ne = compare->predicate == ComparePredicate::Ne &&
            compare->type == i64 && compare->direct_pointer_truthiness.has_value();
        bool exact = (slt || olt || ffs_eq_zero || truthiness_ne || pointer_truthiness_ne) && instruction.operands.size() == 2 &&
            instruction.results.size() == 1;
        if (exact) {
          const auto lhs = function.values_.get(function_id, instruction.operands[0]);
          const auto rhs = function.values_.get(function_id, instruction.operands[1]);
          exact = lhs && rhs && lhs.value().get().type == compare->type &&
              rhs.value().get().type == compare->type;
          const auto* lhs_def = exact
              ? std::get_if<InstResultDef>(&lhs.value().get().definition) : nullptr;
          const auto lhs_producer = lhs_def
              ? function.insts_.get(function_id, lhs_def->instruction)
              : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
          const auto* lhs_parameter = exact
              ? std::get_if<ParameterDef>(&lhs.value().get().definition) : nullptr;
          const auto* lhs_cast = lhs_producer
              ? std::get_if<CastNode>(&lhs_producer.value().get().payload)
              : nullptr;
          const auto* ptrtoint_operand = lhs_cast &&
                  lhs_cast->kind == CastKind::PtrToInt &&
                  lhs_producer.value().get().operands.size() == 1
              ? &lhs_producer.value().get().operands[0]
              : nullptr;
          const auto ptrtoint_source = ptrtoint_operand
              ? function.values_.get(function_id, *ptrtoint_operand)
              : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(ResolveError::OutOfRange);
          const auto* ptrtoint_parameter = ptrtoint_source
              ? std::get_if<ParameterDef>(&ptrtoint_source.value().get().definition)
              : nullptr;
          exact = exact && (truthiness_ne || pointer_truthiness_ne || ffs_eq_zero || (lhs_producer && (slt
              ? std::holds_alternative<LoadNode>(lhs_producer.value().get().payload)
              : [&] {
                  const auto* binary = std::get_if<BinaryNode>(&lhs_producer.value().get().payload);
                  return binary && binary->opcode == BinaryOpcode::FMul && binary->type == f64;
                }())));
          const auto* rhs_def = exact
              ? std::get_if<ConstantDef>(&rhs.value().get().definition) : nullptr;
          const auto* integer = rhs_def && rhs_def->constant.valid() &&
                  rhs_def->constant.epoch == module.epoch_ &&
                  rhs_def->constant.slot < module.constants_.size()
              ? std::get_if<IntegerConstant>(&module.constants_[rhs_def->constant.slot].payload)
              : nullptr;
          exact = exact && (!slt || (integer && integer->value == 7)) &&
              (!ffs_eq_zero || (integer && integer->value == 0)) &&
              (!truthiness_ne || (integer && integer->value == 0 && lhs_parameter &&
                  compare->direct_scalar_truthiness_lhs->source_value_id != 0 &&
                  compare->direct_scalar_truthiness_lhs->parameter_index == lhs_parameter->ordinal &&
                  compare->direct_scalar_truthiness_lhs->scalar_type == compare->type &&
                  compare->direct_scalar_truthiness_lhs->owner.valid() &&
                  compare->direct_scalar_truthiness_lhs->owner.epoch == module.epoch_ &&
                  compare->direct_scalar_truthiness_lhs->owner.slot < module.link_names_.size() &&
                  module.link_names_[compare->direct_scalar_truthiness_lhs->owner.slot].spelling ==
                      function.link_name_ &&
                  instruction.operands[0] ==
                      function.parameters_[compare->direct_scalar_truthiness_lhs->parameter_index])) &&
              (!pointer_truthiness_ne || (integer && integer->value == 0 &&
                  lhs_cast && lhs_cast->kind == CastKind::PtrToInt &&
                  lhs_cast->from_type == Type{TypeKind::Pointer} &&
                  lhs_cast->to_type == compare->type && ptrtoint_parameter &&
                  compare->direct_pointer_truthiness->source_value_id != 0 &&
                  compare->direct_pointer_truthiness->parameter_index == ptrtoint_parameter->ordinal &&
                  compare->direct_pointer_truthiness->pointer_type == Type{TypeKind::Pointer} &&
                  compare->direct_pointer_truthiness->owner.valid() &&
                  compare->direct_pointer_truthiness->owner.epoch == module.epoch_ &&
                  compare->direct_pointer_truthiness->owner.slot < module.link_names_.size() &&
                  module.link_names_[compare->direct_pointer_truthiness->owner.slot].spelling ==
                      function.link_name_ &&
                  *ptrtoint_operand ==
                      function.parameters_[compare->direct_pointer_truthiness->parameter_index]));
          const auto result_value = exact
              ? function.values_.get(function_id, instruction.results[0])
              : Result<std::reference_wrapper<const ValueDef>, ResolveError>::failure(ResolveError::OutOfRange);
          exact = exact && result_value && result_value.value().get().type == i1 &&
              result_value.value().get().source_id.has_value() &&
              result_value.value().get().source_id->owner == function_id;
        }
        if (!exact)
          report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                 "compare must retain an exact admitted typed operand shape and source-backed i1 result");
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
        const Type f64{TypeKind::F64, 64, "double"};
        const Type f32{TypeKind::F32, 32, "float"};
        const bool intrinsic_trunc = cast->kind == CastKind::Trunc &&
            cast->from_type == i64 && cast->to_type == i32;
        const bool scalar_sext = cast->kind == CastKind::SExt &&
            cast->from_type == i32 && cast->to_type == i64;
        const bool scalar_fptrunc = cast->kind == CastKind::FPTrunc &&
            cast->from_type == f64 && cast->to_type == f32;
        const bool scalar_fpext = cast->kind == CastKind::FPExt &&
            cast->from_type == f32 && cast->to_type == f64;
        const bool scalar_sitofp = cast->kind == CastKind::SIToFP &&
            cast->from_type == i32 && cast->to_type == f64;
        const bool scalar_uitofp = cast->kind == CastKind::UIToFP &&
            cast->from_type == i32 && cast->to_type == f64;
        const bool scalar_fptosi = cast->kind == CastKind::FPToSI &&
            cast->from_type == f64 && cast->to_type == i32;
        const bool scalar_fptoui = cast->kind == CastKind::FPToUI &&
            cast->from_type == f64 && cast->to_type == i32;
        const bool pointer_truthiness_ptrtoint =
            cast->kind == CastKind::PtrToInt &&
            cast->from_type == Type{TypeKind::Pointer} && cast->to_type == i64;
        bool exact = (intrinsic_trunc || scalar_sext || scalar_fptrunc || scalar_fpext || scalar_sitofp || scalar_uitofp || scalar_fptosi || scalar_fptoui || pointer_truthiness_ptrtoint) &&
            instruction.operands.size() == 1 && instruction.results.size() == 1;
        if (exact) {
          const auto operand = function.values_.get(function_id, instruction.operands[0]);
          exact = operand && operand.value().get().type == cast->from_type;
          const auto* def = exact ? std::get_if<InstResultDef>(&operand.value().get().definition) : nullptr;
          const auto* parameter = exact ? std::get_if<ParameterDef>(&operand.value().get().definition) : nullptr;
          if (!def && !parameter) exact = false;
          if (exact && pointer_truthiness_ptrtoint) exact = parameter != nullptr;
          if (exact && !pointer_truthiness_ptrtoint && !def) exact = false;
          if (exact && intrinsic_trunc) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            exact = producer && (std::holds_alternative<IntrinsicCallNode>(producer.value().get().payload) ||
                std::holds_alternative<SelectNode>(producer.value().get().payload));
          }
          if (exact && scalar_fptrunc) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            const auto* binary = producer
                ? std::get_if<BinaryNode>(&producer.value().get().payload)
                : nullptr;
            exact = binary && binary->opcode == BinaryOpcode::FMul && binary->type == f64;
          }
          if (exact && scalar_fpext) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            const auto* producer_cast = producer
                ? std::get_if<CastNode>(&producer.value().get().payload) : nullptr;
            exact = producer_cast && producer_cast->kind == CastKind::FPTrunc &&
                producer_cast->from_type == f64 && producer_cast->to_type == f32;
          }
          if (exact && (scalar_sitofp || scalar_uitofp)) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            const auto* binary = producer
                ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
            exact = binary && binary->opcode == BinaryOpcode::Add && binary->type == i32;
          }
          if (exact && (scalar_fptosi || scalar_fptoui)) {
            const auto producer = function.insts_.get(function_id, def->instruction);
            const auto* binary = producer
                ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
            exact = binary && binary->opcode == BinaryOpcode::FAdd && binary->type == f64;
          }
        }
        if (exact) { const auto result_value = function.values_.get(function_id, instruction.results[0]); exact = result_value && result_value.value().get().type == cast->to_type && result_value.value().get().source_id.has_value() && result_value.value().get().source_id->owner == function_id; }
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "cast must retain an admitted typed current-function scalar receipt");
      }
      if (const auto* select = std::get_if<SelectNode>(&instruction.payload)) {
        const Type i32{TypeKind::Integer, 32, "i32"};
        const Type i64{TypeKind::Integer, 64, "i64"};
        bool exact = (select->type == i64 || select->type == i32) &&
            ((select->type == i64 && instruction.operands.empty()) || instruction.operands.size() == 1 ||
             instruction.operands.size() == 2) &&
            instruction.results.size() == 1;
        if (exact && instruction.operands.size() == 2) {
          const auto condition = function.values_.get(function_id, instruction.operands[0]);
          const auto* definition = condition
              ? std::get_if<InstResultDef>(&condition.value().get().definition) : nullptr;
          const auto producer = definition ? function.insts_.get(function_id, definition->instruction)
                                           : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
          const auto* compare = producer ? std::get_if<CompareNode>(&producer.value().get().payload) : nullptr;
          const auto false_value = function.values_.get(function_id, instruction.operands[1]);
          const auto* false_definition = false_value
              ? std::get_if<InstResultDef>(&false_value.value().get().definition) : nullptr;
          const auto false_producer = false_definition
              ? function.insts_.get(function_id, false_definition->instruction)
              : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
          const auto* binary = false_producer ? std::get_if<BinaryNode>(&false_producer.value().get().payload) : nullptr;
          exact = condition && condition.value().get().type == Type{TypeKind::I1, 1, "i1"} && compare &&
              compare->predicate == ComparePredicate::Eq && compare->type == select->type && false_value &&
              false_value.value().get().type == select->type && binary &&
              binary->opcode == BinaryOpcode::Add && binary->type == select->type;
        }
        if (exact && instruction.operands.size() == 1) {
          const auto false_value = function.values_.get(function_id, instruction.operands[0]);
          const auto* definition = false_value
              ? std::get_if<InstResultDef>(&false_value.value().get().definition) : nullptr;
          const auto producer = definition ? function.insts_.get(function_id, definition->instruction)
                                           : Result<std::reference_wrapper<const detail::InstData>, ResolveError>::failure(ResolveError::OutOfRange);
          const auto* binary = producer ? std::get_if<BinaryNode>(&producer.value().get().payload) : nullptr;
          exact = false_value && false_value.value().get().type == select->type && binary &&
              binary->opcode == BinaryOpcode::Add && binary->type == select->type;
        }
        if (exact) {
          const auto value = function.values_.get(function_id, instruction.results[0]);
          exact = value && value.value().get().type == select->type &&
              value.value().get().source_id.has_value() &&
              value.value().get().source_id->owner == function_id;
        }
        if (!exact) report(result, VerificationRule::ValueDefinition, function_id, inst_id,
                           "select must retain the admitted typed wide ffs result receipt");
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
          if (const auto* label = std::get_if<LabelAddressConstant>(
                  &module.constants_[constant->constant.slot].payload);
              label && (label->target.owner != function_id ||
                        !function.blocks_.contains(function_id, label->target)))
            report(result, VerificationRule::ConstantDefinition, function_id,
                   value_id, "label-address constant target must resolve locally");
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
            } else if constexpr (std::is_same_v<Term, IndirectJumpTerm>) {
              const auto address = function.values_.get(function_id, term.address);
              if (term.address.owner != function_id || !address ||
                  address.value().get().type.kind != TypeKind::Pointer)
                report(result, VerificationRule::Terminator, function_id,
                       block_id,
                       "indirect jump address must resolve to a local pointer");
              if (term.targets.empty())
                report(result, VerificationRule::Terminator, function_id,
                       block_id, "indirect jump must have at least one target");
              std::unordered_set<std::uint32_t> targets;
              for (const auto target : term.targets) {
                if (target.owner != function_id ||
                    !function.blocks_.contains(function_id, target) ||
                    !targets.insert(target.slot).second)
                  report(result, VerificationRule::Terminator, function_id,
                         block_id,
                         "indirect jump targets must be unique and resolve in their owner");
              }
            } else if constexpr (std::is_same_v<Term, SwitchTerm>) {
              const auto selector = function.values_.get(function_id, term.selector);
              if (term.selector.owner != function_id || !selector ||
                  !integer_type(selector.value().get().type))
                report(result, VerificationRule::Terminator, function_id,
                       block_id,
                       "switch selector must resolve to a local integer value");
              const auto check_target = [&](BlockId target) {
                return target.owner == function_id &&
                       function.blocks_.contains(function_id, target);
              };
              if (!check_target(term.default_target))
                report(result, VerificationRule::Terminator, function_id,
                       block_id,
                       "switch default target must resolve in its owner");
              for (const auto target : term.case_targets)
                if (!check_target(target))
                  report(result, VerificationRule::Terminator, function_id,
                         block_id,
                         "switch case targets must resolve in their owner");
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
