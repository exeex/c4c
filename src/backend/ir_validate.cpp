#include "ir_validate.hpp"

#include <unordered_map>
#include <unordered_set>

namespace c4c::backend {

namespace {

bool fail(std::string* error, std::string message) {
  if (error != nullptr) {
    *error = std::move(message);
  }
  return false;
}

bool is_local_address_symbol(std::string_view symbol) {
  return !symbol.empty() && symbol.front() == '%';
}

std::string_view address_base_kind_label(BackendAddressBaseKind kind) {
  switch (kind) {
    case BackendAddressBaseKind::Global:
      return "global";
    case BackendAddressBaseKind::StringConstant:
      return "string constant";
    case BackendAddressBaseKind::LocalSlot:
      return "local slot";
    case BackendAddressBaseKind::Unknown:
      break;
  }
  return "object";
}

std::string_view referenced_element_type_label(BackendAddressBaseKind kind) {
  switch (kind) {
    case BackendAddressBaseKind::Global:
      return "global element type";
    case BackendAddressBaseKind::StringConstant:
      return "string-constant element type";
    case BackendAddressBaseKind::LocalSlot:
      return "local-slot element type";
    case BackendAddressBaseKind::Unknown:
      break;
  }
  return "element type";
}

bool validate_function_signature(const BackendFunctionSignature& signature,
                                 std::string* error,
                                 std::string_view context) {
  if (backend_function_linkage(signature) == BackendFunctionLinkage::Unknown) {
    return fail(error, std::string(context) + ": signature linkage must not be empty");
  }
  if (backend_signature_return_type_kind(signature) == BackendValueTypeKind::Unknown) {
    return fail(error, std::string(context) + ": signature return type must not be empty");
  }
  if (signature.name.empty()) {
    return fail(error, std::string(context) + ": signature name must not be empty");
  }
  for (std::size_t index = 0; index < signature.params.size(); ++index) {
    if (backend_param_type_kind(signature.params[index]) == BackendValueTypeKind::Unknown) {
      return fail(error,
                  std::string(context) + ": parameter " + std::to_string(index) +
                      " type must not be empty");
    }
  }
  return true;
}

using FunctionSignatureMap = std::unordered_map<std::string, const BackendFunctionSignature*>;

bool validate_direct_call_contract(const BackendCallInst& call,
                                   const FunctionSignatureMap& function_signatures,
                                   std::string* error,
                                   std::string_view context) {
  if (call.callee.kind != BackendCallCalleeKind::DirectGlobal) {
    return true;
  }

  const auto signature_it = function_signatures.find(call.callee.symbol_name);
  if (signature_it == function_signatures.end()) {
    return fail(error,
                std::string(context) +
                    ": direct call must reference a declared or defined function");
  }

  const auto& signature = *signature_it->second;
  if (backend_call_return_type_kind(call) != backend_signature_return_type_kind(signature) ||
      backend_call_return_scalar_type(call) != backend_signature_return_scalar_type(signature)) {
    return fail(error,
                std::string(context) +
                    ": direct call return type must match callee signature return type");
  }

  if (call.args.size() < signature.params.size()) {
    return fail(error,
                std::string(context) +
                    ": direct call must provide all fixed callee parameters");
  }
  if (!signature.is_vararg && call.args.size() != signature.params.size()) {
    return fail(error,
                std::string(context) +
                    ": direct call arg count must match callee signature");
  }

  for (std::size_t index = 0; index < signature.params.size(); ++index) {
    if (backend_call_param_type_kind(call, index) !=
            backend_param_type_kind(signature.params[index]) ||
        backend_call_param_scalar_type(call, index) !=
            backend_param_scalar_type(signature.params[index])) {
      return fail(error,
                  std::string(context) + ": direct call param type " +
                      std::to_string(index) + " must match callee signature");
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
  if (render_backend_global_type(global).empty()) {
    return fail(error, std::string(context) + ": global type must not be empty");
  }
  if (global.initializer.kind == BackendGlobalInitializer::Kind::Declaration &&
      backend_global_linkage(global) != BackendGlobalLinkage::External &&
      backend_global_linkage(global) != BackendGlobalLinkage::ExternWeak) {
    return fail(error, std::string(context) + ": defined globals must have an initializer");
  }
  if (global.initializer.kind == BackendGlobalInitializer::Kind::RawText &&
      global.initializer.raw_text.empty()) {
    return fail(error, std::string(context) + ": raw global initializers must not be empty");
  }
  if (const auto array_type = backend_global_array_type(global); array_type.has_value()) {
    if (array_type->element_type_kind != BackendValueTypeKind::Scalar ||
        backend_scalar_type_size_bytes(array_type->element_scalar_type) == 0) {
      return fail(error,
                  std::string(context) +
                      ": structured global arrays must use a sized scalar element type");
    }
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

struct ReferencedBoundsInfo {
  BackendAddressBaseKind kind = BackendAddressBaseKind::Global;
  std::size_t total_size_bytes = 0;
  BackendScalarType element_type = BackendScalarType::Unknown;
  std::size_t element_size_bytes = 0;
};

std::optional<ReferencedBoundsInfo> global_bounds_info(const BackendGlobal& global) {
  if (const auto array_type = backend_global_array_type(global); array_type.has_value()) {
    if (array_type->element_type_kind != BackendValueTypeKind::Scalar) {
      return std::nullopt;
    }
    const auto element_size = backend_scalar_type_size_bytes(array_type->element_scalar_type);
    if (element_size == 0) {
      return std::nullopt;
    }
    return ReferencedBoundsInfo{
        BackendAddressBaseKind::Global,
        array_type->element_count * element_size,
        array_type->element_scalar_type,
        element_size,
    };
  }

  if (backend_global_value_type_kind(global) != BackendValueTypeKind::Scalar) {
    return std::nullopt;
  }

  const auto element_size = backend_scalar_type_size_bytes(backend_global_scalar_type(global));
  if (element_size == 0) {
    return std::nullopt;
  }
  return ReferencedBoundsInfo{
      BackendAddressBaseKind::Global,
      element_size,
      backend_global_scalar_type(global),
      element_size,
  };
}

std::optional<ReferencedBoundsInfo> string_constant_bounds_info(
    const BackendStringConstant& string_constant) {
  if (string_constant.byte_length == 0) {
    return std::nullopt;
  }
  return ReferencedBoundsInfo{
      BackendAddressBaseKind::StringConstant,
      string_constant.byte_length,
      BackendScalarType::I8,
      1,
  };
}

bool validate_address_range(const BackendAddress& address,
                            std::size_t access_size_bytes,
                            const std::unordered_map<std::string, ReferencedBoundsInfo>&
                                referenced_bounds,
                            std::string* error,
                            std::string_view context) {
  const auto bounds_it = referenced_bounds.find(address.base_symbol);
  if (bounds_it == referenced_bounds.end()) {
    return true;
  }
  const auto byte_offset = static_cast<std::size_t>(address.byte_offset);
  if (byte_offset >= bounds_it->second.total_size_bytes ||
      access_size_bytes > bounds_it->second.total_size_bytes - byte_offset) {
    return fail(error, std::string(context) + ": address exceeds referenced global bounds");
  }
  if (bounds_it->second.element_size_bytes > 1 &&
      byte_offset % bounds_it->second.element_size_bytes != 0) {
    const auto alignment_context =
        bounds_it->second.kind == BackendAddressBaseKind::LocalSlot
            ? ": address byte offset must align to local access size"
            : ": address byte offset must align to global element size";
    return fail(error, std::string(context) + alignment_context);
  }
  return true;
}

bool validate_address_scalar_type(
    const BackendAddress& address,
    BackendScalarType access_scalar_type,
    const std::unordered_map<std::string, ReferencedBoundsInfo>& referenced_bounds,
    std::string* error,
    std::string_view context) {
  const auto bounds_it = referenced_bounds.find(address.base_symbol);
  if (bounds_it == referenced_bounds.end()) {
    return true;
  }
  if (bounds_it->second.element_type == BackendScalarType::Unknown) {
    return true;
  }
  if (access_scalar_type != bounds_it->second.element_type) {
    return fail(error,
                std::string(context) +
                    ": type must match referenced " +
                    std::string(referenced_element_type_label(bounds_it->second.kind)));
  }
  return true;
}

bool validate_address_base_symbol(
    const BackendAddress& address,
    const std::unordered_map<std::string, BackendAddressBaseKind>& referenced_object_kinds,
    std::string* error,
    std::string_view context) {
  const auto object_it = referenced_object_kinds.find(address.base_symbol);
  if (object_it == referenced_object_kinds.end()) {
    return fail(error,
                std::string(context) +
                    ": base symbol must reference a known global, string constant, or local slot");
  }
  if (address.kind != BackendAddressBaseKind::Unknown && address.kind != object_it->second) {
    return fail(error,
                std::string(context) + ": address kind " +
                    std::string(address_base_kind_label(address.kind)) +
                    " must match referenced " +
                    std::string(address_base_kind_label(object_it->second)));
  }
  return true;
}

bool validate_address_offset(const BackendAddress& address,
                             std::string* error,
                             std::string_view context) {
  if (address.byte_offset < 0) {
    return fail(error, std::string(context) + ": address byte offset must not be negative");
  }
  return true;
}

bool validate_local_address_alignment(
    const BackendAddress& address,
    std::size_t access_size_bytes,
    const std::unordered_map<std::string, BackendAddressBaseKind>& referenced_object_kinds,
    const std::unordered_map<std::string, ReferencedBoundsInfo>& referenced_bounds,
    std::string* error,
    std::string_view context) {
  const auto object_it = referenced_object_kinds.find(address.base_symbol);
  const bool is_local_slot =
      (object_it != referenced_object_kinds.end() &&
       object_it->second == BackendAddressBaseKind::LocalSlot) ||
      address.kind == BackendAddressBaseKind::LocalSlot;
  if (!is_local_slot ||
      referenced_bounds.find(address.base_symbol) != referenced_bounds.end() ||
      access_size_bytes <= 1) {
    return true;
  }
  if (static_cast<std::size_t>(address.byte_offset) % access_size_bytes != 0) {
    return fail(error, std::string(context) + ": address byte offset must align to local access size");
  }
  return true;
}

bool validate_local_slot(const BackendLocalSlot& local_slot,
                         std::string* error,
                         std::string_view context) {
  if (local_slot.name.empty()) {
    return fail(error, std::string(context) + ": local slot name must not be empty");
  }
  if (!is_local_address_symbol(local_slot.name)) {
    return fail(error, std::string(context) + ": local slot name must start with '%'");
  }
  if (local_slot.size_bytes == 0) {
    return fail(error, std::string(context) + ": local slot size must be positive");
  }
  if (local_slot.element_type == BackendScalarType::Unknown) {
    return fail(error, std::string(context) + ": local slot element type must not be empty");
  }
  if (local_slot.element_size_bytes == 0) {
    return fail(error, std::string(context) + ": local slot element size must be positive");
  }
  if (local_slot.size_bytes % local_slot.element_size_bytes != 0) {
    return fail(error,
                std::string(context) +
                    ": local slot size must be a multiple of the element size");
  }
  if (local_slot.element_type != BackendScalarType::Unknown &&
      backend_scalar_type_size_bytes(local_slot.element_type) !=
          local_slot.element_size_bytes) {
    return fail(error,
                std::string(context) +
                    ": local slot element type must match the element size");
  }
  return true;
}

bool validate_inst(const BackendInst& inst,
                   const FunctionSignatureMap& function_signatures,
                   const std::unordered_map<std::string, BackendAddressBaseKind>&
                       referenced_object_kinds,
                   const std::unordered_set<std::string>& referenced_string_constants,
                   const std::unordered_map<std::string, ReferencedBoundsInfo>&
                       referenced_bounds,
                   std::string* error,
                   std::string_view context) {
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
    if (backend_call_return_type_kind(*call) == BackendValueTypeKind::Unknown) {
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
    for (std::size_t index = 0; index < call->args.size(); ++index) {
      if (backend_call_param_type_kind(*call, index) == BackendValueTypeKind::Unknown) {
        return fail(error,
                    std::string(context) + ": call param type " +
                        std::to_string(index) + " must not be empty");
      }
    }
    if (!validate_direct_call_contract(*call, function_signatures, error, context)) {
      return false;
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
    if (load->extension != BackendLoadExtension::None &&
        backend_scalar_type_size_bytes(load_memory_type) >=
            backend_scalar_type_size_bytes(load_value_type)) {
      return fail(error,
                  std::string(context) +
                      ": extended loads must widen from memory type to value type");
    }
    if (load->address.base_symbol.empty()) {
      return fail(error, std::string(context) + ": load base symbol must not be empty");
    }
    if (!validate_address_base_symbol(load->address,
                                      referenced_object_kinds,
                                      error,
                                      std::string(context) + ": load address")) {
      return false;
    }
    if (!validate_address_offset(load->address, error, std::string(context) + ": load address")) {
      return false;
    }
    if (!validate_local_address_alignment(load->address,
                                          backend_scalar_type_size_bytes(load_memory_type),
                                          referenced_object_kinds,
                                          referenced_bounds,
                                          error,
                                          std::string(context) + ": load address")) {
      return false;
    }
    if (!validate_address_range(load->address,
                                backend_scalar_type_size_bytes(load_memory_type),
                                referenced_bounds,
                                error,
                                std::string(context) + ": load address")) {
      return false;
    }
    if (!validate_address_scalar_type(load->address,
                                      load_memory_type,
                                      referenced_bounds,
                                      error,
                                      std::string(context) + ": load memory type")) {
      return false;
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
    if (!validate_address_base_symbol(store->address,
                                      referenced_object_kinds,
                                      error,
                                      std::string(context) + ": store address")) {
      return false;
    }
    if (!validate_address_offset(store->address, error, std::string(context) + ": store address")) {
      return false;
    }
    if (!validate_local_address_alignment(store->address,
                                          backend_scalar_type_size_bytes(
                                              backend_store_value_type(*store)),
                                          referenced_object_kinds,
                                          referenced_bounds,
                                          error,
                                          std::string(context) + ": store address")) {
      return false;
    }
    if (referenced_string_constants.find(store->address.base_symbol) !=
        referenced_string_constants.end()) {
      return fail(error,
                  std::string(context) +
                      ": stores must not target referenced string constants");
    }
    if (!validate_address_range(store->address,
                                backend_scalar_type_size_bytes(backend_store_value_type(*store)),
                                referenced_bounds,
                                error,
                                std::string(context) + ": store address")) {
      return false;
    }
    if (!validate_address_scalar_type(store->address,
                                      backend_store_value_type(*store),
                                      referenced_bounds,
                                      error,
                                      std::string(context) + ": store type")) {
      return false;
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
  if (!validate_address_base_symbol(ptrdiff->lhs_address,
                                    referenced_object_kinds,
                                    error,
                                    std::string(context) + ": ptrdiff lhs address")) {
    return false;
  }
  if (!validate_address_offset(ptrdiff->lhs_address,
                               error,
                               std::string(context) + ": ptrdiff lhs address")) {
    return false;
  }
  if (!validate_address_base_symbol(ptrdiff->rhs_address,
                                    referenced_object_kinds,
                                    error,
                                    std::string(context) + ": ptrdiff rhs address")) {
    return false;
  }
  if (!validate_address_offset(ptrdiff->rhs_address,
                               error,
                               std::string(context) + ": ptrdiff rhs address")) {
    return false;
  }
  if (ptrdiff->element_size <= 0) {
    return fail(error, std::string(context) + ": ptrdiff element size must be positive");
  }
    if (!validate_local_address_alignment(ptrdiff->lhs_address,
                                          static_cast<std::size_t>(ptrdiff->element_size),
                                          referenced_object_kinds,
                                          referenced_bounds,
                                          error,
                                          std::string(context) + ": ptrdiff lhs address")) {
    return false;
  }
    if (!validate_local_address_alignment(ptrdiff->rhs_address,
                                          static_cast<std::size_t>(ptrdiff->element_size),
                                          referenced_object_kinds,
                                          referenced_bounds,
                                          error,
                                          std::string(context) + ": ptrdiff rhs address")) {
    return false;
  }
  const auto element_type = backend_ptrdiff_element_type(*ptrdiff);
  if (element_type != BackendScalarType::Unknown &&
      backend_scalar_type_size_bytes(element_type) !=
          static_cast<std::size_t>(ptrdiff->element_size)) {
    return fail(error,
                std::string(context) +
                    ": ptrdiff element type must match element size");
  }
  const auto lhs_kind_it = referenced_object_kinds.find(ptrdiff->lhs_address.base_symbol);
  const auto rhs_kind_it = referenced_object_kinds.find(ptrdiff->rhs_address.base_symbol);
  const bool lhs_is_referenced_object =
      lhs_kind_it != referenced_object_kinds.end() &&
      lhs_kind_it->second != BackendAddressBaseKind::LocalSlot;
  const bool rhs_is_referenced_object =
      rhs_kind_it != referenced_object_kinds.end() &&
      rhs_kind_it->second != BackendAddressBaseKind::LocalSlot;
  const bool lhs_is_local_address =
      lhs_kind_it != referenced_object_kinds.end() &&
      lhs_kind_it->second == BackendAddressBaseKind::LocalSlot;
  const bool rhs_is_local_address =
      rhs_kind_it != referenced_object_kinds.end() &&
      rhs_kind_it->second == BackendAddressBaseKind::LocalSlot;
  if (lhs_is_referenced_object != rhs_is_referenced_object) {
    return fail(error,
                std::string(context) +
                    ": ptrdiff addresses must both reference the same global when either side is global-backed");
  }
  if (lhs_is_referenced_object && rhs_is_referenced_object &&
      ptrdiff->lhs_address.base_symbol != ptrdiff->rhs_address.base_symbol) {
    return fail(error,
                std::string(context) +
                    ": ptrdiff addresses must reference the same global");
  }
  if (lhs_is_local_address && rhs_is_local_address &&
      ptrdiff->lhs_address.base_symbol != ptrdiff->rhs_address.base_symbol) {
    return fail(error,
                std::string(context) +
                    ": ptrdiff addresses must reference the same local slot");
  }
  const auto lhs_bounds_it = referenced_bounds.find(ptrdiff->lhs_address.base_symbol);
  const bool same_structured_base =
      ptrdiff->lhs_address.base_symbol == ptrdiff->rhs_address.base_symbol &&
      lhs_bounds_it != referenced_bounds.end();
  if ((lhs_is_referenced_object && rhs_is_referenced_object && same_structured_base) ||
      (!lhs_is_referenced_object && !rhs_is_referenced_object && same_structured_base)) {
    if (!validate_address_range(ptrdiff->lhs_address,
                                1,
                                referenced_bounds,
                                error,
                                std::string(context) + ": ptrdiff lhs address") ||
        !validate_address_range(ptrdiff->rhs_address,
                                1,
                                referenced_bounds,
                                error,
                                std::string(context) + ": ptrdiff rhs address")) {
      return false;
    }
    if (lhs_bounds_it->second.element_type != BackendScalarType::Unknown &&
        element_type != BackendScalarType::Unknown &&
        lhs_bounds_it->second.element_type != element_type) {
      return fail(error,
                  std::string(context) +
                      ": ptrdiff element type must match referenced global element type");
    }
    if (ptrdiff->element_size !=
        static_cast<std::int64_t>(lhs_bounds_it->second.element_size_bytes)) {
      return fail(error,
                  std::string(context) +
                      ": ptrdiff element size must match referenced global element size");
    }
    if (ptrdiff->lhs_address.byte_offset % ptrdiff->element_size != 0 ||
        ptrdiff->rhs_address.byte_offset % ptrdiff->element_size != 0) {
      return fail(error,
                  std::string(context) +
                      ": ptrdiff addresses must align to the referenced element size");
    }
    const auto actual_diff =
        (ptrdiff->lhs_address.byte_offset - ptrdiff->rhs_address.byte_offset) /
        ptrdiff->element_size;
    if (actual_diff != ptrdiff->expected_diff) {
      return fail(error,
                  std::string(context) +
                      ": ptrdiff expected diff must match the referenced addresses");
    }
  }
  return true;
}

bool validate_block(const BackendBlock& block,
                    const FunctionSignatureMap& function_signatures,
                    const std::unordered_map<std::string, BackendAddressBaseKind>&
                        referenced_object_kinds,
                    const std::unordered_set<std::string>& referenced_string_constants,
                    const std::unordered_map<std::string, ReferencedBoundsInfo>&
                        referenced_bounds,
                    std::string* error,
                    std::string_view context) {
  if (block.label.empty()) {
    return fail(error, std::string(context) + ": block label must not be empty");
  }
  switch (block.terminator.kind) {
    case BackendTerminatorKind::Return:
      if (render_backend_return_type(block.terminator).empty()) {
        return fail(error, std::string(context) + ": block terminator type must not be empty");
      }
      if (!block.terminator.value.has_value() && !backend_return_is_void(block.terminator)) {
        return fail(error,
                    std::string(context) + ": void terminators must use type 'void'");
      }
      if (block.terminator.value.has_value() && backend_return_is_void(block.terminator)) {
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
                       function_signatures,
                       referenced_object_kinds,
                       referenced_string_constants,
                       referenced_bounds,
                       error,
                       std::string(context) + ": instruction " + std::to_string(index))) {
      return false;
    }
  }
  return true;
}

bool validate_function(const BackendFunction& function,
                       const FunctionSignatureMap& function_signatures,
                       const std::unordered_map<std::string, BackendAddressBaseKind>&
                           referenced_object_kinds,
                       const std::unordered_set<std::string>& referenced_string_constants,
                       const std::unordered_map<std::string, ReferencedBoundsInfo>&
                           referenced_bounds,
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

  auto function_bounds = referenced_bounds;
  auto function_object_kinds = referenced_object_kinds;
  for (std::size_t index = 0; index < function.local_slots.size(); ++index) {
    const auto& local_slot = function.local_slots[index];
    if (!validate_local_slot(local_slot,
                             error,
                             std::string(context) + ": local slot " + std::to_string(index))) {
      return false;
    }
    if (!function_bounds
             .emplace(local_slot.name,
                      ReferencedBoundsInfo{BackendAddressBaseKind::LocalSlot,
                                           local_slot.size_bytes,
                                           local_slot.element_type,
                                           local_slot.element_size_bytes})
             .second) {
      return fail(error,
                  std::string(context) + ": duplicate referenced object '" + local_slot.name +
                      "'");
    }
    function_object_kinds.emplace(local_slot.name, BackendAddressBaseKind::LocalSlot);
  }

  std::unordered_set<std::string> labels;
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    const auto& block = function.blocks[index];
    if (!labels.insert(block.label).second) {
      return fail(error,
                  std::string(context) + ": duplicate block label '" + block.label + "'");
    }
    if (!validate_block(block,
                        function_signatures,
                        function_object_kinds,
                        referenced_string_constants,
                        function_bounds,
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
  FunctionSignatureMap function_signatures;
  std::unordered_map<std::string, BackendAddressBaseKind> referenced_object_kinds;
  std::unordered_set<std::string> referenced_string_constants;
  std::unordered_map<std::string, ReferencedBoundsInfo> referenced_bounds;
  for (std::size_t index = 0; index < module.globals.size(); ++index) {
    const auto& global = module.globals[index];
    if (!validate_global(global, error, std::string("global ") + std::to_string(index))) {
      return false;
    }
    if (!referenced_object_kinds.emplace(global.name, BackendAddressBaseKind::Global).second) {
      return fail(error, "duplicate global '" + global.name + "'");
    }
    if (const auto bounds = global_bounds_info(global); bounds.has_value()) {
      referenced_bounds.emplace(global.name, *bounds);
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
    if (!referenced_object_kinds
             .emplace(string_constant.name, BackendAddressBaseKind::StringConstant)
             .second) {
      return fail(error, "duplicate global '" + string_constant.name + "'");
    }
    referenced_string_constants.insert(string_constant.name);
    if (const auto bounds = string_constant_bounds_info(string_constant); bounds.has_value()) {
      referenced_bounds.emplace(string_constant.name, *bounds);
    }
  }

  for (std::size_t index = 0; index < module.functions.size(); ++index) {
    if (!function_signatures
             .emplace(module.functions[index].signature.name, &module.functions[index].signature)
             .second) {
      return fail(error,
                  "duplicate function '" + module.functions[index].signature.name + "'");
    }
  }

  for (std::size_t index = 0; index < module.functions.size(); ++index) {
    if (!validate_function(module.functions[index],
                           function_signatures,
                           referenced_object_kinds,
                           referenced_string_constants,
                           referenced_bounds,
                           error,
                           std::string("function ") + std::to_string(index))) {
      return false;
    }
  }
  return true;
}

}  // namespace c4c::backend
