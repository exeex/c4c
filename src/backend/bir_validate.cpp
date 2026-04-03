#include "bir_validate.hpp"

namespace c4c::backend::bir {

namespace {

bool fail(std::string* error, std::string message) {
  if (error != nullptr) {
    *error = std::move(message);
  }
  return false;
}

bool validate_return(const Function& function,
                     const Block& block,
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
      if (!validate_return(function, block, error)) {
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
