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
  if (global.llvm_type.empty()) {
    return fail(error, std::string(context) + ": global type must not be empty");
  }
  if (global.initializer.kind == BackendGlobalInitializer::Kind::Declaration &&
      global.linkage != "external " && global.linkage != "extern_weak ") {
    return fail(error, std::string(context) + ": defined globals must have an initializer");
  }
  if (global.initializer.kind == BackendGlobalInitializer::Kind::RawText &&
      global.initializer.raw_text.empty()) {
    return fail(error, std::string(context) + ": raw global initializers must not be empty");
  }
  return true;
}

bool validate_string_constant(const BackendStringConstant& string_constant,
                              std::string* error,
                              std::string_view context) {
  if (string_constant.name.empty()) {
    return fail(error, std::string(context) + ": string constant name must not be empty");
  }
  if (string_constant.byte_length != string_constant.raw_bytes.size() + 1) {
    return fail(error,
                std::string(context) +
                    ": string constant byte length must match raw bytes plus trailing nul");
  }
  return true;
}

bool validate_inst(const BackendInst& inst, std::string* error, std::string_view context) {
  if (const auto* phi = std::get_if<BackendPhiInst>(&inst)) {
    if (phi->result.empty()) {
      return fail(error, std::string(context) + ": phi result must not be empty");
    }
    if (backend_phi_value_type(*phi) == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": phi type must not be empty");
    }
    if (phi->incoming.empty()) {
      return fail(error, std::string(context) + ": phi must have at least one incoming edge");
    }
    for (std::size_t index = 0; index < phi->incoming.size(); ++index) {
      if (phi->incoming[index].value.empty()) {
        return fail(error,
                    std::string(context) + ": phi incoming " + std::to_string(index) +
                        " value must not be empty");
      }
      if (phi->incoming[index].label.empty()) {
        return fail(error,
                    std::string(context) + ": phi incoming " + std::to_string(index) +
                        " label must not be empty");
      }
    }
    return true;
  }

  if (const auto* bin = std::get_if<BackendBinaryInst>(&inst)) {
    if (bin->result.empty()) {
      return fail(error, std::string(context) + ": binary result must not be empty");
    }
    if (backend_binary_value_type(*bin) == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": binary type must not be empty");
    }
    if (bin->lhs.empty() || bin->rhs.empty()) {
      return fail(error, std::string(context) + ": binary operands must not be empty");
    }
    return true;
  }

  if (const auto* cmp = std::get_if<BackendCompareInst>(&inst)) {
    if (cmp->result.empty()) {
      return fail(error, std::string(context) + ": compare result must not be empty");
    }
    if (backend_compare_operand_type(*cmp) == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": compare type must not be empty");
    }
    if (cmp->lhs.empty() || cmp->rhs.empty()) {
      return fail(error, std::string(context) + ": compare operands must not be empty");
    }
    return true;
  }

  if (const auto* call = std::get_if<BackendCallInst>(&inst)) {
    if (call->return_type.empty()) {
      return fail(error, std::string(context) + ": call return type must not be empty");
    }
    switch (call->callee.kind) {
      case BackendCallCalleeKind::DirectGlobal:
      case BackendCallCalleeKind::DirectIntrinsic:
        if (call->callee.symbol_name.empty()) {
          return fail(error, std::string(context) + ": direct call symbol must not be empty");
        }
        break;
      case BackendCallCalleeKind::Indirect:
        if (call->callee.operand.empty()) {
          return fail(error, std::string(context) + ": indirect call operand must not be empty");
        }
        break;
    }
    if (call->param_types.size() != call->args.size()) {
      return fail(error,
                  std::string(context) + ": call param type count must match arg count");
    }
    return true;
  }

  const auto* load = std::get_if<BackendLoadInst>(&inst);
  if (load != nullptr) {
    if (load->result.empty()) {
      return fail(error, std::string(context) + ": load result must not be empty");
    }
    const auto load_value_type = backend_load_value_type(*load);
    if (load_value_type == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": load type must not be empty");
    }
    const auto load_memory_type = backend_load_memory_type(*load);
    if (load_memory_type == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": load memory type must not be empty");
    }
    if (load->extension == BackendLoadExtension::None &&
        load_memory_type != load_value_type) {
      return fail(error,
                  std::string(context) +
                      ": widened loads must declare an extension kind");
    }
    if (load->extension != BackendLoadExtension::None &&
        load_memory_type == load_value_type) {
      return fail(error,
                  std::string(context) +
                      ": extended loads must declare a memory type");
    }
    if (load->address.base_symbol.empty()) {
      return fail(error, std::string(context) + ": load base symbol must not be empty");
    }
    return true;
  }

  const auto* store = std::get_if<BackendStoreInst>(&inst);
  if (store != nullptr) {
    if (backend_store_value_type(*store) == BackendScalarType::Unknown) {
      return fail(error, std::string(context) + ": store type must not be empty");
    }
    if (store->value.empty()) {
      return fail(error, std::string(context) + ": store value must not be empty");
    }
    if (store->address.base_symbol.empty()) {
      return fail(error, std::string(context) + ": store base symbol must not be empty");
    }
    return true;
  }

  const auto* ptrdiff = std::get_if<BackendPtrDiffEqInst>(&inst);
  if (ptrdiff == nullptr) {
    return fail(error, std::string(context) + ": unknown instruction variant");
  }
  if (ptrdiff->result.empty()) {
    return fail(error, std::string(context) + ": ptrdiff result must not be empty");
  }
  if (backend_ptrdiff_result_type(*ptrdiff) == BackendScalarType::Unknown) {
    return fail(error, std::string(context) + ": ptrdiff type must not be empty");
  }
  if (ptrdiff->lhs_address.base_symbol.empty() ||
      ptrdiff->rhs_address.base_symbol.empty()) {
    return fail(error, std::string(context) + ": ptrdiff base symbols must not be empty");
  }
  if (ptrdiff->element_size <= 0) {
    return fail(error, std::string(context) + ": ptrdiff element size must be positive");
  }
  const auto element_type = backend_ptrdiff_element_type(*ptrdiff);
  if (element_type != BackendScalarType::Unknown &&
      backend_scalar_type_size_bytes(element_type) !=
          static_cast<std::size_t>(ptrdiff->element_size)) {
    return fail(error,
                std::string(context) +
                    ": ptrdiff element type must match element size");
  }
  return true;
}

bool validate_block(const BackendBlock& block, std::string* error, std::string_view context) {
  if (block.label.empty()) {
    return fail(error, std::string(context) + ": block label must not be empty");
  }
  switch (block.terminator.kind) {
    case BackendTerminatorKind::Return:
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
      break;
    case BackendTerminatorKind::Branch:
      if (block.terminator.target_label.empty()) {
        return fail(error, std::string(context) + ": branch target label must not be empty");
      }
      break;
    case BackendTerminatorKind::CondBranch:
      if (block.terminator.cond_name.empty()) {
        return fail(error, std::string(context) + ": conditional branch value must not be empty");
      }
      if (block.terminator.true_label.empty() || block.terminator.false_label.empty()) {
        return fail(error,
                    std::string(context) +
                        ": conditional branch target labels must not be empty");
      }
      break;
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

  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    const auto& terminator = function.blocks[index].terminator;
    for (const auto& inst : function.blocks[index].insts) {
      const auto* phi = std::get_if<BackendPhiInst>(&inst);
      if (phi == nullptr) {
        continue;
      }
      for (const auto& incoming : phi->incoming) {
        if (labels.find(incoming.label) == labels.end()) {
          return fail(error,
                      std::string(context) + ": block " + std::to_string(index) +
                          " phi references unknown label '" + incoming.label + "'");
        }
      }
    }
    if (terminator.kind == BackendTerminatorKind::Branch &&
        labels.find(terminator.target_label) == labels.end()) {
      return fail(error,
                  std::string(context) + ": block " + std::to_string(index) +
                      " branches to unknown label '" + terminator.target_label + "'");
    }
    if (terminator.kind == BackendTerminatorKind::CondBranch) {
      if (labels.find(terminator.true_label) == labels.end()) {
        return fail(error,
                    std::string(context) + ": block " + std::to_string(index) +
                        " branches to unknown true label '" + terminator.true_label + "'");
      }
      if (labels.find(terminator.false_label) == labels.end()) {
        return fail(error,
                    std::string(context) + ": block " + std::to_string(index) +
                        " branches to unknown false label '" + terminator.false_label + "'");
      }
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
  for (std::size_t index = 0; index < module.string_constants.size(); ++index) {
    const auto& string_constant = module.string_constants[index];
    if (!validate_string_constant(string_constant,
                                  error,
                                  std::string("string constant ") +
                                      std::to_string(index))) {
      return false;
    }
    if (!globals.insert(string_constant.name).second) {
      return fail(error, "duplicate global '" + string_constant.name + "'");
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
