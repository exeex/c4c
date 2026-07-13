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

bool opcode_matches_payload(const detail::InstData& instruction) noexcept {
  switch (instruction.opcode) {
    case Opcode::InlineAsm:
      return std::holds_alternative<InlineAsmNode>(instruction.payload);
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
