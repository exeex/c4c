#include "bir.hpp"

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <vector>

namespace c4c::backend::bir {

namespace {

bool fail(std::string* error, std::string message) {
  if (error != nullptr) {
    *error = std::move(message);
  }
  return false;
}

bool validate_link_name_id(const Module& module,
                           LinkNameId id,
                           std::string_view context,
                           std::string* error);

const Function* find_function(const Module& module,
                              std::string_view function_name,
                              LinkNameId function_name_id);
const Global* find_global_by_name(const Module& module, std::string_view global_name);
const Function* find_function_by_name(const Module& module, std::string_view function_name);

bool validate_named_value(const Value& value,
                          std::string_view context,
                          std::string* error) {
  if (value.kind == Value::Kind::Named && value.name.empty()) {
    return fail(error, std::string(context) + " must not use an empty name");
  }
  return true;
}

bool validate_binary(const Function& function,
                     const BinaryInst& inst,
                     std::vector<std::string>* defined_names,
                     std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir binary result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (!validate_named_value(inst.lhs, "bir binary lhs in @" + function.name, error) ||
      !validate_named_value(inst.rhs, "bir binary rhs in @" + function.name, error)) {
    return false;
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_select(const Function& function,
                     const SelectInst& inst,
                     std::vector<std::string>* defined_names,
                     std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir select result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (!validate_named_value(inst.lhs, "bir select compare lhs in @" + function.name, error) ||
      !validate_named_value(inst.rhs, "bir select compare rhs in @" + function.name, error) ||
      !validate_named_value(inst.true_value,
                            "bir select true value in @" + function.name,
                            error) ||
      !validate_named_value(inst.false_value,
                            "bir select false value in @" + function.name,
                            error)) {
    return false;
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_cast(const Function& function,
                   const CastInst& inst,
                   std::vector<std::string>* defined_names,
                   std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir cast result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (!validate_named_value(inst.operand, "bir cast operand in @" + function.name, error)) {
    return false;
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_phi(const Function& function,
                  const std::vector<std::string>& block_labels,
                  const PhiInst& inst,
                  std::vector<std::string>* defined_names,
                  std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir phi result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (inst.incomings.empty()) {
    return fail(error, "bir phi in @" + function.name + " must have at least one incoming");
  }
  std::vector<std::string_view> incoming_labels;
  incoming_labels.reserve(inst.incomings.size());
  for (const auto& incoming : inst.incomings) {
    if (incoming.label.empty()) {
      return fail(error, "bir phi in @" + function.name +
                             " must not use an empty incoming label");
    }
    if (std::find(block_labels.begin(), block_labels.end(), incoming.label) == block_labels.end()) {
      return fail(error, "bir phi in @" + function.name +
                             " must reference blocks in the same function");
    }
    if (std::find(incoming_labels.begin(), incoming_labels.end(), incoming.label) !=
        incoming_labels.end()) {
      return fail(error, "bir phi in @" + function.name +
                             " must not repeat an incoming label");
    }
    if (!validate_named_value(incoming.value, "bir phi incoming value in @" + function.name, error)) {
      return false;
    }
    incoming_labels.push_back(incoming.label);
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_call(const Module& module,
                   const Function& function,
                   const CallInst& inst,
                   std::vector<std::string>* defined_names,
                   std::string* error) {
  if (inst.is_indirect) {
    if (!inst.callee_value.has_value()) {
      return fail(error, "bir indirect call in @" + function.name +
                             " must carry a callee value");
    }
  } else if (inst.callee.empty()) {
    return fail(error, "bir call in @" + function.name + " must name a callee");
  }
  if (!validate_link_name_id(
          module, inst.callee_link_name_id, "bir call in @" + function.name, error)) {
    return false;
  }
  if (!inst.is_indirect && inst.callee_link_name_id != kInvalidLinkName &&
      find_function(module, inst.callee, inst.callee_link_name_id) == nullptr) {
    return fail(error, "bir call in @" + function.name +
                           " must reference a declared function by LinkNameId");
  }
  if (!inst.is_indirect && inst.callee_link_name_id != kInvalidLinkName) {
    const auto* named_function = find_function_by_name(module, inst.callee);
    if (named_function != nullptr && named_function->link_name_id != kInvalidLinkName &&
        named_function->link_name_id != inst.callee_link_name_id) {
      return fail(error, "bir call in @" + function.name +
                             " must not pair LinkNameId with a different declared function name");
    }
  }
  if (inst.result.has_value()) {
    if (inst.result->kind != Value::Kind::Named || inst.result->name.empty()) {
      return fail(error, "bir call result in @" + function.name +
                             " must use a non-empty named value");
    }
    defined_names->push_back(inst.result->name);
  }
  for (const auto& arg : inst.args) {
    if (!validate_named_value(arg, "bir call arg in @" + function.name, error)) {
      return false;
    }
  }
  if (inst.callee_value.has_value() &&
      !validate_named_value(*inst.callee_value,
                            "bir call callee value in @" + function.name,
                            error)) {
    return false;
  }
  return true;
}

bool validate_local_slot_names(const Function& function, std::string* error) {
  std::vector<std::string_view> slot_names;
  slot_names.reserve(function.local_slots.size());
  for (const auto& slot : function.local_slots) {
    if (slot.name.empty()) {
      return fail(error, "bir local slot in @" + function.name +
                             " must use a non-empty name");
    }
    if (slot.size_bytes == 0) {
      return fail(error, "bir local slot in @" + function.name +
                             " must use a non-zero size");
    }
    if (std::find(slot_names.begin(), slot_names.end(), slot.name) != slot_names.end()) {
      return fail(error, "bir local slot in @" + function.name +
                             " must not reuse an existing slot name");
    }
    slot_names.push_back(slot.name);
  }
  return true;
}

bool validate_link_name_id(const Module& module,
                           LinkNameId id,
                           std::string_view context,
                           std::string* error) {
  if (id == kInvalidLinkName) {
    return true;
  }
  if (module.names.link_names.spelling(id).empty()) {
    return fail(error, std::string(context) + " must reference a known LinkNameId");
  }
  return true;
}

const Global* find_global(const Module& module,
                          std::string_view global_name,
                          LinkNameId global_name_id = kInvalidLinkName) {
  if (global_name_id != kInvalidLinkName) {
    const auto it = std::find_if(
        module.globals.begin(),
        module.globals.end(),
        [&](const Global& global) { return global.link_name_id == global_name_id; });
    return it == module.globals.end() ? nullptr : &*it;
  }
  return find_global_by_name(module, global_name);
}

const Function* find_function(const Module& module,
                              std::string_view function_name,
                              LinkNameId function_name_id = kInvalidLinkName) {
  if (function_name_id != kInvalidLinkName) {
    const auto it = std::find_if(
        module.functions.begin(),
        module.functions.end(),
        [&](const Function& function) { return function.link_name_id == function_name_id; });
    return it == module.functions.end() ? nullptr : &*it;
  }
  return find_function_by_name(module, function_name);
}

const Global* find_global_by_name(const Module& module, std::string_view global_name) {
  const auto it = std::find_if(module.globals.begin(),
                               module.globals.end(),
                               [&](const Global& global) { return global.name == global_name; });
  return it == module.globals.end() ? nullptr : &*it;
}

const Function* find_function_by_name(const Module& module, std::string_view function_name) {
  const auto it = std::find_if(
      module.functions.begin(),
      module.functions.end(),
      [&](const Function& function) { return function.name == function_name; });
  return it == module.functions.end() ? nullptr : &*it;
}

bool validate_global_link_name_matches_visible_name(const Module& module,
                                                    std::string_view global_name,
                                                    LinkNameId global_name_id,
                                                    std::string_view context,
                                                    std::string* error) {
  if (global_name_id == kInvalidLinkName) {
    return true;
  }
  const auto* named_global = find_global_by_name(module, global_name);
  if (named_global != nullptr && named_global->link_name_id != kInvalidLinkName &&
      named_global->link_name_id != global_name_id) {
    return fail(error, std::string(context) +
                           " must not pair LinkNameId with a different declared global name");
  }
  return true;
}

bool validate_initializer_symbol_link_name(const Module& module,
                                           const Global& owner,
                                           std::string* error) {
  if (owner.initializer_symbol_name_id == kInvalidLinkName) {
    return true;
  }
  if (!owner.initializer_symbol_name.has_value()) {
    return fail(error, "bir global initializer symbol @" + owner.name +
                           " must not carry LinkNameId without a symbol name");
  }

  const auto symbol_name = std::string_view(*owner.initializer_symbol_name);
  if (const auto* named_global = find_global_by_name(module, symbol_name);
      named_global != nullptr && named_global->link_name_id != kInvalidLinkName &&
      named_global->link_name_id != owner.initializer_symbol_name_id) {
    return fail(error, "bir global initializer symbol @" + owner.name +
                           " must not pair LinkNameId with a different declared global name");
  }
  if (const auto* named_function = find_function_by_name(module, symbol_name);
      named_function != nullptr && named_function->link_name_id != kInvalidLinkName &&
      named_function->link_name_id != owner.initializer_symbol_name_id) {
    return fail(error, "bir global initializer symbol @" + owner.name +
                           " must not pair LinkNameId with a different declared function name");
  }
  if (find_global(module, symbol_name, owner.initializer_symbol_name_id) == nullptr &&
      find_function(module, symbol_name, owner.initializer_symbol_name_id) == nullptr) {
    return fail(error, "bir global initializer symbol @" + owner.name +
                           " must reference a declared global or function by LinkNameId");
  }
  return true;
}

const LocalSlot* find_local_slot(const Function& function, std::string_view slot_name) {
  const auto it = std::find_if(function.local_slots.begin(),
                               function.local_slots.end(),
                               [&](const LocalSlot& slot) { return slot.name == slot_name; });
  return it == function.local_slots.end() ? nullptr : &*it;
}

bool validate_load_local(const Function& function,
                         const LoadLocalInst& inst,
                         std::vector<std::string>* defined_names,
                         std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir local load result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (find_local_slot(function, inst.slot_name) == nullptr) {
    return fail(error, "bir local load in @" + function.name +
                           " must reference a declared local slot");
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_load_global(const Module& module,
                          const Function& function,
                          const LoadGlobalInst& inst,
                          std::vector<std::string>* defined_names,
                          std::string* error) {
  if (inst.result.kind != Value::Kind::Named || inst.result.name.empty()) {
    return fail(error, "bir global load result in @" + function.name +
                           " must use a non-empty named value");
  }
  if (!validate_link_name_id(
          module, inst.global_name_id, "bir global load in @" + function.name, error)) {
    return false;
  }
  if (find_global(module, inst.global_name, inst.global_name_id) == nullptr) {
    return fail(error, "bir global load in @" + function.name +
                           " must reference a declared global");
  }
  if (!validate_global_link_name_matches_visible_name(
          module,
          inst.global_name,
          inst.global_name_id,
          "bir global load in @" + function.name,
          error)) {
    return false;
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_store_local(const Function& function,
                          const StoreLocalInst& inst,
                          const std::vector<std::string>& defined_names,
                          std::string* error) {
  if (find_local_slot(function, inst.slot_name) == nullptr) {
    return fail(error, "bir local store in @" + function.name +
                           " must reference a declared local slot");
  }
  if (!validate_named_value(inst.value, "bir local store value in @" + function.name, error)) {
    return false;
  }
  if (inst.value.kind == Value::Kind::Named &&
      std::find(defined_names.begin(), defined_names.end(), inst.value.name) ==
          defined_names.end()) {
    return fail(error, "bir local store in @" + function.name +
                           " must reference a value defined earlier in the block");
  }
  return true;
}

bool validate_store_global(const Module& module,
                           const Function& function,
                           const StoreGlobalInst& inst,
                           const std::vector<std::string>& defined_names,
                           std::string* error) {
  if (!validate_link_name_id(
          module, inst.global_name_id, "bir global store in @" + function.name, error)) {
    return false;
  }
  if (find_global(module, inst.global_name, inst.global_name_id) == nullptr) {
    return fail(error, "bir global store in @" + function.name +
                           " must reference a declared global");
  }
  if (!validate_global_link_name_matches_visible_name(
          module,
          inst.global_name,
          inst.global_name_id,
          "bir global store in @" + function.name,
          error)) {
    return false;
  }
  if (!validate_named_value(inst.value, "bir global store value in @" + function.name, error)) {
    return false;
  }
  if (inst.value.kind == Value::Kind::Named &&
      std::find(defined_names.begin(), defined_names.end(), inst.value.name) ==
          defined_names.end()) {
    return fail(error, "bir global store in @" + function.name +
                           " must reference a value defined earlier in the block");
  }
  return true;
}

bool validate_params(const Function& function,
                     std::vector<std::string>* defined_names,
                     std::string* error) {
  for (const auto& param : function.params) {
    if (param.name.empty()) {
      return fail(error, "bir param in @" + function.name +
                             " must use a non-empty name");
    }
    if (std::find(defined_names->begin(), defined_names->end(), param.name) !=
        defined_names->end()) {
      return fail(error, "bir param in @" + function.name +
                             " must not reuse an existing value name");
    }
    defined_names->push_back(param.name);
  }
  return true;
}

bool validate_return(const Function& function,
                     const Block& block,
                     const std::vector<std::string>& defined_names,
                     std::string* error) {
  (void)function;
  if (block.terminator.kind != TerminatorKind::Return ||
      !block.terminator.value.has_value()) {
    return true;
  }
  if (!validate_named_value(*block.terminator.value, "bir return value", error)) {
    return false;
  }
  if (block.terminator.value->kind == Value::Kind::Named &&
      std::find(defined_names.begin(), defined_names.end(),
                block.terminator.value->name) == defined_names.end()) {
    return fail(error, "bir return must reference a value defined earlier in the block");
  }
  return true;
}

bool validate_terminator(const Function& function,
                         const Block& block,
                         const std::vector<std::string>& block_labels,
                         const std::vector<std::string>& defined_names,
                         std::string* error) {
  switch (block.terminator.kind) {
    case TerminatorKind::Return:
      return validate_return(function, block, defined_names, error);
    case TerminatorKind::Branch:
      if (block.terminator.target_label.empty()) {
        return fail(error, "bir branch in @" + function.name + " must name a target");
      }
      if (std::find(block_labels.begin(), block_labels.end(), block.terminator.target_label) ==
          block_labels.end()) {
        return fail(error, "bir branch in @" + function.name +
                               " must target a block in the same function");
      }
      return true;
    case TerminatorKind::CondBranch:
      if (!validate_named_value(block.terminator.condition,
                                "bir cond_br condition in @" + function.name,
                                error)) {
        return false;
      }
      if (block.terminator.condition.kind == Value::Kind::Named &&
          std::find(defined_names.begin(), defined_names.end(),
                    block.terminator.condition.name) == defined_names.end()) {
        return fail(error, "bir cond_br in @" + function.name +
                               " must reference a value defined earlier in the block");
      }
      if (block.terminator.true_label.empty() || block.terminator.false_label.empty()) {
        return fail(error, "bir cond_br in @" + function.name +
                               " must name both successor labels");
      }
      if (std::find(block_labels.begin(), block_labels.end(), block.terminator.true_label) ==
              block_labels.end() ||
          std::find(block_labels.begin(), block_labels.end(), block.terminator.false_label) ==
              block_labels.end()) {
        return fail(error, "bir cond_br in @" + function.name +
                               " must target blocks in the same function");
      }
      return true;
  }
  return false;
}

}  // namespace

bool validate(const Module& module, std::string* error) {
  std::vector<std::string_view> global_names;
  std::vector<LinkNameId> global_name_ids;
  global_names.reserve(module.globals.size());
  global_name_ids.reserve(module.globals.size());
  for (const auto& global : module.globals) {
    if (global.name.empty()) {
      return fail(error, "bir global must have a non-empty name");
    }
    if (!validate_link_name_id(module, global.link_name_id, "bir global @" + global.name, error)) {
      return false;
    }
    if (std::find(global_names.begin(), global_names.end(), global.name) != global_names.end()) {
      return fail(error, "bir globals must not reuse an existing name");
    }
    if (global.link_name_id != kInvalidLinkName) {
      if (std::find(global_name_ids.begin(), global_name_ids.end(), global.link_name_id) !=
          global_name_ids.end()) {
        return fail(error, "bir globals must not reuse an existing LinkNameId");
      }
      global_name_ids.push_back(global.link_name_id);
    }
    if (!global.is_extern && !global.initializer.has_value() &&
        !global.initializer_symbol_name.has_value() && global.initializer_elements.empty()) {
      return fail(error, "bir defined global @" + global.name + " must have an initializer");
    }
    if (global.initializer_symbol_name.has_value() && global.initializer_symbol_name->empty()) {
      return fail(error, "bir global initializer symbol @" + global.name +
                             " must not use an empty name");
    }
    if (!validate_link_name_id(module,
                               global.initializer_symbol_name_id,
                               "bir global initializer symbol @" + global.name,
                               error)) {
      return false;
    }
    if (!validate_initializer_symbol_link_name(module, global, error)) {
      return false;
    }
    if (global.initializer.has_value()) {
      if (global.initializer->kind == Value::Kind::Named &&
          global.initializer->name.empty()) {
        return fail(error, "bir global initializer @" + global.name +
                               " must not use an empty name");
      }
      if (global.initializer->kind == Value::Kind::Named &&
          global.initializer->type != TypeKind::Ptr) {
        return fail(error, "bir global initializer @" + global.name +
                               " may only use named pointer initializers");
      }
      if (global.initializer->kind != Value::Kind::Immediate &&
          global.initializer->kind != Value::Kind::Named) {
        return fail(error, "bir global initializer @" + global.name +
                               " must be an immediate or named pointer");
      }
    }
    for (const auto& element : global.initializer_elements) {
      if (element.kind == Value::Kind::Named && element.name.empty()) {
        return fail(error, "bir global initializer element @" + global.name +
                               " must not use an empty name");
      }
      if (element.kind == Value::Kind::Named && element.type != TypeKind::Ptr) {
        return fail(error, "bir global initializer element @" + global.name +
                               " may only use named pointer initializers");
      }
      if (element.kind != Value::Kind::Immediate && element.kind != Value::Kind::Named) {
        return fail(error, "bir global initializer element @" + global.name +
                               " must be an immediate or named pointer");
      }
    }
    global_names.push_back(global.name);
  }

  std::vector<LinkNameId> function_name_ids;
  function_name_ids.reserve(module.functions.size());
  for (std::size_t function_index = 0; function_index < module.functions.size();
       ++function_index) {
    const auto& function = module.functions[function_index];
    if (function.name.empty()) {
      return fail(error, "bir function " + std::to_string(function_index) +
                             " must have a name");
    }
    if (!validate_link_name_id(
            module, function.link_name_id, "bir function @" + function.name, error)) {
      return false;
    }
    if (function.link_name_id != kInvalidLinkName) {
      if (std::find(function_name_ids.begin(), function_name_ids.end(), function.link_name_id) !=
          function_name_ids.end()) {
        return fail(error, "bir functions must not reuse an existing LinkNameId");
      }
      function_name_ids.push_back(function.link_name_id);
    }
    if (function.is_declaration) {
      if (!function.blocks.empty()) {
        return fail(error, "bir declaration @" + function.name +
                               " must not define blocks");
      }
      continue;
    }
    if (function.blocks.empty()) {
      return fail(error, "bir function @" + function.name +
                             " must contain at least one block");
    }
    if (!validate_local_slot_names(function, error)) {
      return false;
    }

    std::vector<std::string> block_labels;
    block_labels.reserve(function.blocks.size());
    for (const auto& block : function.blocks) {
      if (block.label.empty()) {
        return fail(error, "bir block in @" + function.name + " must have a label");
      }
      if (std::find(block_labels.begin(), block_labels.end(), block.label) != block_labels.end()) {
        return fail(error, "bir function @" + function.name +
                               " must not reuse a block label");
      }
      block_labels.push_back(block.label);
    }

    for (const auto& block : function.blocks) {
      std::vector<std::string> defined_names;
      if (!validate_params(function, &defined_names, error)) {
        return false;
      }

      for (const auto& inst : block.insts) {
        const bool ok = std::visit(
            [&](const auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, BinaryInst>) {
                return validate_binary(function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, SelectInst>) {
                return validate_select(function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, CastInst>) {
                return validate_cast(function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, PhiInst>) {
                return validate_phi(function, block_labels, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, CallInst>) {
                return validate_call(module, function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, LoadLocalInst>) {
                return validate_load_local(function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
                return validate_load_global(module, function, lowered, &defined_names, error);
              } else if constexpr (std::is_same_v<T, StoreGlobalInst>) {
                return validate_store_global(module, function, lowered, defined_names, error);
              } else if constexpr (std::is_same_v<T, StoreLocalInst>) {
                return validate_store_local(function, lowered, defined_names, error);
              }
            },
            inst);
        if (!ok) {
          return false;
        }
      }

      if (!validate_terminator(function, block, block_labels, defined_names, error)) {
        return false;
      }
    }
  }

  if (error != nullptr) {
    error->clear();
  }
  return true;
}

}  // namespace c4c::backend::bir
