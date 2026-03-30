#include "ir_validate.hpp"

#include <unordered_set>

namespace c4c::backend {

namespace {

bool fail(std::string* error, std::string message) {
  if (error != nullptr) {
    *error = std::move(message);
  }
  return false;
}

bool validate_function_signature(const BackendFunctionSignature& signature,
                                 std::string* error,
                                 std::string_view context) {
  if (signature.linkage.empty()) {
    return fail(error, std::string(context) + ": signature linkage must not be empty");
  }
  if (signature.return_type.empty()) {
    return fail(error, std::string(context) + ": signature return type must not be empty");
  }
  if (signature.name.empty()) {
    return fail(error, std::string(context) + ": signature name must not be empty");
  }
  for (std::size_t index = 0; index < signature.params.size(); ++index) {
    if (signature.params[index].type_str.empty()) {
      return fail(error,
                  std::string(context) + ": parameter " + std::to_string(index) +
                      " type must not be empty");
    }
  }
  return true;
}

bool validate_global(const BackendGlobal& global,
                     std::string* error,
                     std::string_view context) {
  if (global.name.empty()) {
    return fail(error, std::string(context) + ": global name must not be empty");
  }
  if (global.qualifier.empty()) {
    return fail(error, std::string(context) + ": global qualifier must not be empty");
  }
  if (global.llvm_type.empty()) {
    return fail(error, std::string(context) + ": global type must not be empty");
  }
  if (!global.is_extern_decl && global.init_text.empty()) {
    return fail(error, std::string(context) + ": defined globals must have an initializer");
  }
  if (global.is_extern_decl && !global.init_text.empty()) {
    return fail(error, std::string(context) + ": extern globals must not have an initializer");
  }
  return true;
}

bool validate_inst(const BackendInst& inst, std::string* error, std::string_view context) {
  if (const auto* bin = std::get_if<BackendBinaryInst>(&inst)) {
    if (bin->result.empty()) {
      return fail(error, std::string(context) + ": binary result must not be empty");
    }
    if (bin->type_str.empty()) {
      return fail(error, std::string(context) + ": binary type must not be empty");
    }
    if (bin->lhs.empty() || bin->rhs.empty()) {
      return fail(error, std::string(context) + ": binary operands must not be empty");
    }
    return true;
  }

  if (const auto* call = std::get_if<BackendCallInst>(&inst)) {
    if (call->return_type.empty()) {
      return fail(error, std::string(context) + ": call return type must not be empty");
    }
    if (call->callee.empty()) {
      return fail(error, std::string(context) + ": call callee must not be empty");
    }
    if (call->param_types.size() != call->args.size()) {
      return fail(error,
                  std::string(context) + ": call param type count must match arg count");
    }
    return true;
  }

  const auto* load = std::get_if<BackendLoadInst>(&inst);
  if (load == nullptr) {
    return fail(error, std::string(context) + ": unknown instruction variant");
  }
  if (load->result.empty()) {
    return fail(error, std::string(context) + ": load result must not be empty");
  }
  if (load->type_str.empty()) {
    return fail(error, std::string(context) + ": load type must not be empty");
  }
  if (load->address.base_symbol.empty()) {
    return fail(error, std::string(context) + ": load base symbol must not be empty");
  }
  return true;
}

bool validate_block(const BackendBlock& block, std::string* error, std::string_view context) {
  if (block.label.empty()) {
    return fail(error, std::string(context) + ": block label must not be empty");
  }
  if (block.terminator.type_str.empty()) {
    return fail(error, std::string(context) + ": block terminator type must not be empty");
  }
  if (!block.terminator.value.has_value() && block.terminator.type_str != "void") {
    return fail(error,
                std::string(context) + ": void terminators must use type 'void'");
  }
  if (block.terminator.value.has_value() && block.terminator.type_str == "void") {
    return fail(error,
                std::string(context) + ": non-void terminators must not use type 'void'");
  }
  for (std::size_t index = 0; index < block.insts.size(); ++index) {
    if (!validate_inst(block.insts[index],
                       error,
                       std::string(context) + ": instruction " + std::to_string(index))) {
      return false;
    }
  }
  return true;
}

bool validate_function(const BackendFunction& function,
                       std::string* error,
                       std::string_view context) {
  if (!validate_function_signature(function.signature, error, context)) {
    return false;
  }
  if (function.is_declaration) {
    if (!function.blocks.empty()) {
      return fail(error, std::string(context) + ": declarations must not have blocks");
    }
    return true;
  }
  if (function.blocks.empty()) {
    return fail(error, std::string(context) + ": definitions must have at least one block");
  }

  std::unordered_set<std::string> labels;
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    const auto& block = function.blocks[index];
    if (!labels.insert(block.label).second) {
      return fail(error,
                  std::string(context) + ": duplicate block label '" + block.label + "'");
    }
    if (!validate_block(block,
                        error,
                        std::string(context) + ": block " + std::to_string(index))) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool validate_backend_ir(const BackendModule& module, std::string* error) {
  std::unordered_set<std::string> globals;
  for (std::size_t index = 0; index < module.globals.size(); ++index) {
    const auto& global = module.globals[index];
    if (!validate_global(global, error, std::string("global ") + std::to_string(index))) {
      return false;
    }
    if (!globals.insert(global.name).second) {
      return fail(error, "duplicate global '" + global.name + "'");
    }
  }

  for (std::size_t index = 0; index < module.functions.size(); ++index) {
    if (!validate_function(module.functions[index],
                           error,
                           std::string("function ") + std::to_string(index))) {
      return false;
    }
  }
  return true;
}

}  // namespace c4c::backend
