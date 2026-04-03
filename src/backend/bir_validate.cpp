#include "bir_validate.hpp"

#include <algorithm>
#include <string_view>
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

}  // namespace

bool validate(const Module& module, std::string* error) {
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
        if (!validate_binary(function, inst, &defined_names, error)) {
          return false;
        }
      }
      if (!validate_return(function, block, defined_names, error)) {
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
