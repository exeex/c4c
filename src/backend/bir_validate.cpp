#include "bir_validate.hpp"

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

bool validate_value_type(TypeKind type,
                         std::string_view context,
                         std::string* error) {
  if (type == TypeKind::Void) {
    return fail(error, std::string(context) + " must not use void-typed values");
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
  if (!validate_value_type(inst.result.type,
                           "bir binary result in @" + function.name,
                           error)) {
    return false;
  }
  if (!validate_value_type(inst.lhs.type,
                           "bir binary lhs in @" + function.name,
                           error) ||
      !validate_value_type(inst.rhs.type,
                           "bir binary rhs in @" + function.name,
                           error)) {
    return false;
  }
  if (inst.result.type != inst.lhs.type || inst.result.type != inst.rhs.type) {
    return fail(error, "bir binary operands in @" + function.name +
                           " must agree on one scalar type");
  }
  if (inst.lhs.kind == Value::Kind::Named && inst.lhs.name.empty()) {
    return fail(error, "bir binary lhs in @" + function.name +
                           " must not use an empty name");
  }
  if (inst.rhs.kind == Value::Kind::Named && inst.rhs.name.empty()) {
    return fail(error, "bir binary rhs in @" + function.name +
                           " must not use an empty name");
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
  if (!validate_value_type(inst.result.type,
                           "bir select result in @" + function.name,
                           error) ||
      !validate_value_type(inst.lhs.type,
                           "bir select compare lhs in @" + function.name,
                           error) ||
      !validate_value_type(inst.rhs.type,
                           "bir select compare rhs in @" + function.name,
                           error) ||
      !validate_value_type(inst.true_value.type,
                           "bir select true value in @" + function.name,
                           error) ||
      !validate_value_type(inst.false_value.type,
                           "bir select false value in @" + function.name,
                           error)) {
    return false;
  }
  if (inst.result.type != inst.lhs.type || inst.result.type != inst.rhs.type ||
      inst.result.type != inst.true_value.type ||
      inst.result.type != inst.false_value.type) {
    return fail(error, "bir select operands in @" + function.name +
                           " must agree on one scalar type");
  }
  if (inst.lhs.kind == Value::Kind::Named && inst.lhs.name.empty()) {
    return fail(error, "bir select compare lhs in @" + function.name +
                           " must not use an empty name");
  }
  if (inst.rhs.kind == Value::Kind::Named && inst.rhs.name.empty()) {
    return fail(error, "bir select compare rhs in @" + function.name +
                           " must not use an empty name");
  }
  if (inst.true_value.kind == Value::Kind::Named && inst.true_value.name.empty()) {
    return fail(error, "bir select true value in @" + function.name +
                           " must not use an empty name");
  }
  if (inst.false_value.kind == Value::Kind::Named && inst.false_value.name.empty()) {
    return fail(error, "bir select false value in @" + function.name +
                           " must not use an empty name");
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
  if (!validate_value_type(inst.result.type,
                           "bir cast result in @" + function.name,
                           error)) {
    return false;
  }
  if (!validate_value_type(inst.operand.type,
                           "bir cast operand in @" + function.name,
                           error)) {
    return false;
  }
  if (inst.operand.kind == Value::Kind::Named && inst.operand.name.empty()) {
    return fail(error, "bir cast operand in @" + function.name +
                           " must not use an empty name");
  }
  if (inst.result.type == inst.operand.type) {
    return fail(error, "bir cast in @" + function.name +
                           " must change the type");
  }
  const auto result_width = inst.result.type;
  const auto operand_width = inst.operand.type;
  if (inst.opcode == CastOpcode::Trunc && result_width >= operand_width) {
    return fail(error, "bir trunc in @" + function.name +
                           " must narrow the type");
  }
  if ((inst.opcode == CastOpcode::SExt || inst.opcode == CastOpcode::ZExt) &&
      result_width <= operand_width) {
    return fail(error, "bir sext/zext in @" + function.name +
                           " must widen the type");
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_call(const Function& function,
                   const CallInst& inst,
                   std::vector<std::string>* defined_names,
                   std::string* error) {
  if (inst.callee.empty()) {
    return fail(error, "bir call in @" + function.name + " must name a callee");
  }
  if (inst.return_type_name.empty()) {
    return fail(error, "bir call in @" + function.name + " must name a return type");
  }
  if (inst.result.has_value()) {
    if (inst.result->kind != Value::Kind::Named || inst.result->name.empty()) {
      return fail(error, "bir call result in @" + function.name +
                             " must use a non-empty named value");
    }
    if (!validate_value_type(inst.result->type,
                             "bir call result in @" + function.name,
                             error)) {
      return false;
    }
    if (inst.result->type == TypeKind::Void) {
      return fail(error, "bir call result in @" + function.name +
                             " must not be void-typed");
    }
  }
  for (const auto& arg : inst.args) {
    if (!validate_value_type(arg.type, "bir call arg in @" + function.name, error)) {
      return false;
    }
    if (arg.kind == Value::Kind::Named && arg.name.empty()) {
      return fail(error, "bir call arg in @" + function.name +
                             " must not use an empty name");
    }
  }
  if (inst.result.has_value()) {
    defined_names->push_back(inst.result->name);
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
    if (!validate_value_type(slot.type,
                             "bir local slot in @" + function.name,
                             error)) {
      return false;
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

const Global* find_global(const Module& module, std::string_view global_name) {
  const auto it = std::find_if(module.globals.begin(),
                               module.globals.end(),
                               [&](const Global& global) { return global.name == global_name; });
  return it == module.globals.end() ? nullptr : &*it;
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
  if (!validate_value_type(inst.result.type,
                           "bir local load result in @" + function.name,
                           error)) {
    return false;
  }
  const auto* slot = find_local_slot(function, inst.slot_name);
  if (slot == nullptr) {
    return fail(error, "bir local load in @" + function.name +
                           " must reference a declared local slot");
  }
  if (slot->type != inst.result.type) {
    return fail(error, "bir local load in @" + function.name +
                           " must agree with the referenced local slot type");
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
  if (!validate_value_type(inst.result.type,
                           "bir global load result in @" + function.name,
                           error)) {
    return false;
  }
  const auto* global = find_global(module, inst.global_name);
  if (global == nullptr) {
    return fail(error, "bir global load in @" + function.name +
                           " must reference a declared global");
  }
  if (global->type != inst.result.type) {
    return fail(error, "bir global load in @" + function.name +
                           " must agree with the referenced global type");
  }
  defined_names->push_back(inst.result.name);
  return true;
}

bool validate_store_local(const Function& function,
                          const StoreLocalInst& inst,
                          const std::vector<std::string>& defined_names,
                          std::string* error) {
  const auto* slot = find_local_slot(function, inst.slot_name);
  if (slot == nullptr) {
    return fail(error, "bir local store in @" + function.name +
                           " must reference a declared local slot");
  }
  if (!validate_value_type(inst.value.type,
                           "bir local store value in @" + function.name,
                           error)) {
    return false;
  }
  if (slot->type != inst.value.type) {
    return fail(error, "bir local store in @" + function.name +
                           " must agree with the referenced local slot type");
  }
  if (inst.value.kind == Value::Kind::Named) {
    if (inst.value.name.empty()) {
      return fail(error, "bir local store value in @" + function.name +
                             " must not use an empty name");
    }
    if (std::find(defined_names.begin(), defined_names.end(), inst.value.name) ==
        defined_names.end()) {
      return fail(error, "bir local store in @" + function.name +
                             " must reference a value defined earlier in the block");
    }
  }
  return true;
}

bool validate_store_global(const Module& module,
                           const Function& function,
                           const StoreGlobalInst& inst,
                           const std::vector<std::string>& defined_names,
                           std::string* error) {
  const auto* global = find_global(module, inst.global_name);
  if (global == nullptr) {
    return fail(error, "bir global store in @" + function.name +
                           " must reference a declared global");
  }
  if (!validate_value_type(inst.value.type,
                           "bir global store value in @" + function.name,
                           error)) {
    return false;
  }
  if (global->type != inst.value.type) {
    return fail(error, "bir global store in @" + function.name +
                           " must agree with the referenced global type");
  }
  if (inst.value.kind == Value::Kind::Named) {
    if (inst.value.name.empty()) {
      return fail(error, "bir global store value in @" + function.name +
                             " must not use an empty name");
    }
    if (std::find(defined_names.begin(), defined_names.end(), inst.value.name) ==
        defined_names.end()) {
      return fail(error, "bir global store in @" + function.name +
                             " must reference a value defined earlier in the block");
    }
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
    if (!validate_value_type(param.type,
                             "bir param in @" + function.name,
                             error)) {
      return false;
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
  if (block.terminator.kind != TerminatorKind::Return) {
    return true;
  }

  const auto& returned = block.terminator.value;
  if (function.return_type == TypeKind::Void) {
    if (returned.has_value()) {
      return fail(error, "bir return in @" + function.name +
                             " must not carry a value for void functions");
    }
    return true;
  }

  if (!returned.has_value()) {
    return fail(error, "bir return in @" + function.name +
                           " must produce a value for non-void functions");
  }
  if (returned->type != function.return_type) {
    return fail(error, "bir return in @" + function.name +
                           " must match the function return type");
  }
  if (returned->kind == Value::Kind::Named && returned->name.empty()) {
    return fail(error, "bir named return value in @" + function.name +
                           " must not use an empty name");
  }
  if (returned->kind == Value::Kind::Named &&
      std::find(defined_names.begin(), defined_names.end(), returned->name) ==
          defined_names.end()) {
    return fail(error, "bir return in @" + function.name +
                           " must reference a value defined earlier in the block");
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
      if (!validate_value_type(block.terminator.condition.type,
                               "bir cond_br condition in @" + function.name,
                               error)) {
        return false;
      }
      if (block.terminator.condition.kind == Value::Kind::Named) {
        if (block.terminator.condition.name.empty()) {
          return fail(error, "bir cond_br condition in @" + function.name +
                                 " must not use an empty name");
        }
        if (std::find(defined_names.begin(), defined_names.end(),
                      block.terminator.condition.name) == defined_names.end()) {
          return fail(error, "bir cond_br in @" + function.name +
                                 " must reference a value defined earlier in the block");
        }
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
  global_names.reserve(module.globals.size());
  for (const auto& global : module.globals) {
    if (global.name.empty()) {
      return fail(error, "bir global must have a non-empty name");
    }
    if (!validate_value_type(global.type, "bir global", error)) {
      return false;
    }
    if (std::find(global_names.begin(), global_names.end(), global.name) != global_names.end()) {
      return fail(error, "bir globals must not reuse an existing name");
    }
    if (!global.is_extern && !global.initializer.has_value()) {
      return fail(error, "bir defined global @" + global.name + " must have an initializer");
    }
    if (global.initializer.has_value()) {
      if (!validate_value_type(global.initializer->type,
                               "bir global initializer",
                               error)) {
        return false;
      }
      if (global.initializer->kind != Value::Kind::Immediate ||
          global.initializer->type != global.type) {
        return fail(error, "bir global initializer @" + global.name +
                               " must be an immediate matching the global type");
      }
    }
    global_names.push_back(global.name);
  }
  for (std::size_t function_index = 0; function_index < module.functions.size(); ++function_index) {
    const auto& function = module.functions[function_index];
    if (function.name.empty()) {
      return fail(error, "bir function " + std::to_string(function_index) +
                             " must have a name");
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
      if (std::find(block_labels.begin(), block_labels.end(), block.label) != block_labels.end()) {
        return fail(error, "bir function @" + function.name +
                               " must not reuse a block label");
      }
      block_labels.push_back(block.label);
    }
    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      if (block.label.empty()) {
        return fail(error, "bir block " + std::to_string(block_index) +
                               " in @" + function.name + " must have a label");
      }
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
              } else if constexpr (std::is_same_v<T, CallInst>) {
                return validate_call(function, lowered, &defined_names, error);
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
