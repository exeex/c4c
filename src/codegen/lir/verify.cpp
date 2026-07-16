#include "ir.hpp"
#include "call_args_ops.hpp"
#include "../shared/llvm_helpers.hpp"

#include <algorithm>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace c4c::codegen::lir {

namespace {

[[noreturn]] void fail_verify(std::string_view field, const std::string& detail) {
  throw LirVerifyError(LirVerifyErrorKind::Malformed,
                       std::string(field) + ": " + detail);
}

std::string operand_kind_name(LirOperandKind kind) {
  switch (kind) {
    case LirOperandKind::SsaValue: return "ssa";
    case LirOperandKind::DirectConstant: return "direct-constant";
    case LirOperandKind::Global: return "global";
    case LirOperandKind::Label: return "label";
    case LirOperandKind::Immediate: return "immediate";
    case LirOperandKind::SpecialToken: return "special-token";
    case LirOperandKind::RawText: return "raw-text";
  }
  return "unknown";
}

void verify_operand_authority_kind(const LirOperand& operand,
                                   std::string_view field) {
  if (!operand.has_authority()) return;

  LirOperandKind expected = LirOperandKind::RawText;
  if (const auto* id = operand.value_id()) {
    if (!id->valid()) fail_verify(field, "invalid LirValueId authority");
    expected = operand.kind() == LirOperandKind::DirectConstant
                   ? LirOperandKind::DirectConstant
                   : LirOperandKind::SsaValue;
  } else if (operand.link_name_id()) {
    expected = LirOperandKind::Global;
  } else if (operand.integer_immediate()) {
    expected = LirOperandKind::Immediate;
  } else if (operand.special_token()) {
    expected = LirOperandKind::SpecialToken;
  } else {
    fail_verify(field, "unknown operand authority alternative");
  }

  if (operand.kind() != expected) {
    fail_verify(field,
                "authority alternative disagrees with stored operand kind");
  }
}

void verify_phi_special_token_authority(const LirOperand& operand,
                                        std::string_view field) {
  if (operand.kind() != LirOperandKind::SpecialToken) return;

  const LirSpecialToken* token = operand.special_token();
  if (!token) {
    fail_verify(field, "special-token PHI input must carry native authority");
  }
  const std::string_view expected_display = lir_special_token_spelling(*token);
  if (expected_display.empty()) {
    fail_verify(field, "special-token PHI input has invalid native authority");
  }
  if (operand.str() != expected_display) {
    fail_verify(field,
                "special-token display mirror disagrees with native authority");
  }
}

std::size_t count_inline_asm_constraints(std::string_view constraints) {
  if (constraints.empty()) return 0;
  std::size_t count = 1;
  for (char ch : constraints) {
    if (ch == ',') ++count;
  }
  return count;
}

bool same_inline_asm_type(const LirTypeRef& lhs, const LirTypeRef& rhs) {
  return lhs.kind() == rhs.kind() && lhs.str() == rhs.str() &&
         lhs.struct_name_id() == rhs.struct_name_id();
}

bool same_native_type_fact(const LirTypeRef& lhs, const LirTypeRef& rhs) {
  if (lhs.kind() != rhs.kind()) return false;
  if (lhs.integer_bit_width() != rhs.integer_bit_width()) return false;
  if (lhs.vrm_width() != rhs.vrm_width()) return false;
  if (lhs.builtin_type() != rhs.builtin_type()) return false;
  if (lhs.named_composite_kind() != rhs.named_composite_kind()) return false;
  if (lhs.has_struct_name_id() || rhs.has_struct_name_id()) {
    return lhs.has_struct_name_id() && rhs.has_struct_name_id() &&
           lhs.struct_name_id() == rhs.struct_name_id();
  }
  if (lhs.has_array_shape() || rhs.has_array_shape()) {
    if (!lhs.has_array_shape() || !rhs.has_array_shape() ||
        lhs.array_length() != rhs.array_length()) {
      return false;
    }
    return same_native_type_fact(*lhs.array_element_type(),
                                 *rhs.array_element_type());
  }
  if (lhs.has_anonymous_struct_layout() || rhs.has_anonymous_struct_layout()) {
    const auto* lhs_fields = lhs.anonymous_struct_field_types();
    const auto* rhs_fields = rhs.anonymous_struct_field_types();
    if (!lhs_fields || !rhs_fields || lhs_fields->size() != rhs_fields->size()) {
      return false;
    }
    for (std::size_t index = 0; index < lhs_fields->size(); ++index) {
      if (!same_native_type_fact((*lhs_fields)[index], (*rhs_fields)[index])) {
        return false;
      }
    }
  }
  return true;
}

bool operand_kind_allowed(LirOperandKind kind,
                          std::initializer_list<LirOperandKind> allowed_kinds) {
  for (const auto allowed : allowed_kinds) {
    if (allowed == kind) return true;
  }
  if (kind == LirOperandKind::RawText) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Immediate) return true;
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  if (kind == LirOperandKind::SpecialToken) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  if (kind == LirOperandKind::Immediate) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  return false;
}

std::optional<std::string> type_ref_mismatch_detail(const LirTypeRef& type) {
  if (type.has_anonymous_struct_layout()) {
    const auto* fields = type.anonymous_struct_field_types();
    if (!fields || type.kind() != LirTypeKind::Struct || type.has_struct_name_id()) {
      return "anonymous aggregate layout must belong to an unnamed struct type";
    }
    if (fields->empty()) {
      return "anonymous aggregate layout must carry ordered field types";
    }
    if (type.str() != type.render_llvm()) {
      return "anonymous aggregate layout display mirror disagrees with native fields";
    }
  }
  return std::nullopt;
}

std::optional<std::string> type_ref_struct_name_mismatch_detail(
    const StructNameTable& struct_names,
    const LirTypeRef& type) {
  if (!type.has_struct_name_id()) return std::nullopt;

  const std::string_view rendered_name =
      struct_names.spelling(type.struct_name_id());
  if (rendered_name.empty()) {
    return "StructNameId mirror must resolve to a struct name";
  }

  if (rendered_name != type.str()) {
    std::ostringstream detail;
    detail << "StructNameId mirror '" << rendered_name
           << "' disagrees with text '" << type.str() << "'";
    return detail.str();
  }

  return std::nullopt;
}

const std::string& require_module_type_ref(const LirModule& mod,
                                           const LirTypeRef& type,
                                           std::string_view field,
                                           bool allow_void = false) {
  const std::string& rendered = require_type_ref(type, field, allow_void);
  if (const auto mismatch =
          type_ref_struct_name_mismatch_detail(mod.struct_names, type);
      mismatch.has_value()) {
    fail_verify(field, *mismatch);
  }
  if (const auto* fields = type.anonymous_struct_field_types()) {
    for (const LirTypeRef& field_type : *fields) {
      require_module_type_ref(mod, field_type, field);
    }
  }
  return rendered;
}

const std::vector<LirTypeRef>& require_native_anonymous_struct_fields(
    const LirModule& mod,
    const LirTypeRef& type,
    std::string_view field) {
  if (type.kind() != LirTypeKind::Struct || type.has_struct_name_id()) {
    fail_verify(field,
                "native aggregate authority requires an unnamed struct type");
  }
  const std::vector<LirTypeRef>* fields = type.anonymous_struct_field_types();
  if (!fields || fields->empty()) {
    fail_verify(field,
                "native aggregate authority requires ordered native field types");
  }
  for (const LirTypeRef& field_type : *fields) {
    require_module_type_ref(mod, field_type, field);
  }
  return *fields;
}

void require_module_cast_endpoint_type_ref(const LirModule& mod,
                                           const LirTypeRef& type,
                                           std::string_view field) {
  if (type.kind() == LirTypeKind::Integer) {
    (void)render_integer_type_ref(type, field);
    return;
  }
  if (type.kind() == LirTypeKind::Floating) {
    (void)render_floating_type_ref(type, field);
    return;
  }
  require_module_type_ref(mod, type, field);
}

void require_module_phi_boundary_value_type_ref(const LirModule& mod,
                                                const LirPhiBoundaryValueType& boundary_type,
                                                std::string_view field) {
  if (boundary_type.kind == LirPhiBoundaryValueKind::Scalar) {
    if (boundary_type.type.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(boundary_type.type, field);
      return;
    }
    if (boundary_type.type.kind() == LirTypeKind::Floating) {
      (void)render_floating_type_ref(boundary_type.type, field);
      return;
    }
  }
  require_module_type_ref(mod, boundary_type.type, field);
}

StructNameId find_declared_struct_name_id(const LirModule& mod,
                                          std::string_view rendered_name) {
  const StructNameId struct_name_id = mod.struct_names.find(rendered_name);
  if (struct_name_id == kInvalidStructName ||
      !mod.find_struct_decl(struct_name_id)) {
    return kInvalidStructName;
  }
  return struct_name_id;
}

void verify_declared_struct_type_ref_mirror(const LirModule& mod,
                                            const LirTypeRef& mirror,
                                            std::string_view field);

void verify_known_struct_type_ref_mirror(const LirModule& mod,
                                         const LirTypeRef& type,
                                         std::string_view field,
                                         std::string_view type_role) {
  if (type.has_struct_name_id()) {
    verify_declared_struct_type_ref_mirror(mod, type, field);
    return;
  }

  if (find_declared_struct_name_id(mod, type.str()) != kInvalidStructName) {
    std::ostringstream detail;
    detail << "known struct " << type_role
           << " type must carry matching StructNameId";
    fail_verify(field, detail.str());
  }
}

void verify_declared_struct_type_ref_mirror(const LirModule& mod,
                                            const LirTypeRef& mirror,
                                            std::string_view field) {
  if (mirror.kind() != LirTypeKind::Struct) {
    fail_verify(field, "StructNameId mirror must be a struct type");
  }
  const std::string_view rendered_name =
      mod.struct_names.spelling(mirror.struct_name_id());
  if (rendered_name.empty()) {
    fail_verify(field, "StructNameId mirror must resolve to a struct name");
  }
  if (!mod.find_struct_decl(mirror.struct_name_id())) {
    fail_verify(field, "StructNameId mirror must resolve to a declared struct");
  }
}

void verify_call_aggregate_type_ref_store_entry(const LirModule& mod,
                                                const LirTypeRef& mirror,
                                                std::string_view field) {
  if (!mirror.has_struct_name_id() || mod.aggregate_store.empty()) return;

  const LirAggregateStoreEntry* found = nullptr;
  for (const LirAggregateStoreEntry& entry : mod.aggregate_store) {
    if (entry.name_id == mirror.struct_name_id()) {
      found = &entry;
      break;
    }
  }
  if (!found) {
    fail_verify(field,
                "call aggregate mirror requires matching canonical LIR aggregate store entry");
  }

  const bool layout_is_union =
      found->layout_kind == LirAggregateLayoutKind::Union;
  const bool mirror_is_union =
      mirror.named_composite_kind() == LirNamedCompositeKind::Union;
  if (mirror_is_union != layout_is_union) {
    fail_verify(field,
                "call aggregate mirror disagrees with canonical aggregate store kind");
  }

  const LirStructDecl* decl = mod.find_struct_decl(mirror.struct_name_id());
  if (!decl) {
    fail_verify(field,
                "call aggregate mirror requires matching structured declaration facts");
  }
  if (decl->name_id != found->name_id || decl->fields.size() != found->fields.size() ||
      decl->is_packed != found->is_packed || decl->is_opaque != found->is_opaque) {
    fail_verify(field,
                "call aggregate mirror disagrees with canonical aggregate store facts");
  }
}

bool call_arg_type_matches_byval_pointee(std::string_view formatted_type,
                                         std::string_view pointee_type) {
  const std::string byval_fragment = "byval(" + std::string(pointee_type) + ")";
  return formatted_type.find(byval_fragment) != std::string_view::npos;
}

void verify_call_return_type_ref_mirror(const LirModule& mod,
                                        const LirTypeRef& mirror) {
  const std::string& shadow =
      require_type_ref(mirror, "LirCallOp.return_type", true);
  const StructNameId formatted_struct_name_id =
      find_declared_struct_name_id(mod, shadow);

  if (mirror.has_struct_name_id()) {
    verify_declared_struct_type_ref_mirror(mod, mirror, "LirCallOp.return_type");
    verify_call_aggregate_type_ref_store_entry(mod, mirror,
                                               "LirCallOp.return_type");
    if (formatted_struct_name_id != kInvalidStructName) {
      if (mirror.struct_name_id() != formatted_struct_name_id) {
        fail_verify("LirCallOp.return_type",
                    "return mirror StructNameId does not match call text");
      }
    }
    return;
  } else if (formatted_struct_name_id != kInvalidStructName) {
    fail_verify("LirCallOp.return_type",
                "known struct return type must carry matching StructNameId");
  }

  if (const auto mismatch =
          type_ref_struct_name_mismatch_detail(mod.struct_names, mirror);
      mismatch.has_value()) {
    fail_verify("LirCallOp.return_type", *mismatch);
  }
}

void verify_call_arg_type_ref_mirror(const LirModule& mod,
                                     const LirTypeRef& mirror,
                                     std::string_view formatted_type,
                                     size_t index) {
  const std::string& shadow =
      require_type_ref(mirror, "LirCallOp.arg_type_refs");
  const StructNameId formatted_struct_name_id =
      find_declared_struct_name_id(mod, formatted_type);

  if (mirror.has_struct_name_id()) {
    verify_declared_struct_type_ref_mirror(mod, mirror, "LirCallOp.arg_type_refs");
    verify_call_aggregate_type_ref_store_entry(mod, mirror,
                                               "LirCallOp.arg_type_refs");
    const std::string_view rendered_name =
        mod.struct_names.spelling(mirror.struct_name_id());
    if (formatted_struct_name_id != kInvalidStructName) {
      if (mirror.struct_name_id() != formatted_struct_name_id) {
        std::ostringstream detail;
        detail << "argument " << index
               << " mirror StructNameId does not match call text";
        fail_verify("LirCallOp.arg_type_refs", detail.str());
      }
      return;
    }
    if (call_arg_type_matches_byval_pointee(formatted_type, rendered_name)) {
      return;
    }
  } else if (formatted_struct_name_id != kInvalidStructName) {
    fail_verify("LirCallOp.arg_type_refs",
                "known struct argument type must carry matching StructNameId");
  }

  if (const auto mismatch =
          type_ref_struct_name_mismatch_detail(mod.struct_names, mirror);
      mismatch.has_value()) {
    fail_verify("LirCallOp.arg_type_refs", *mismatch);
  }

  if (shadow != formatted_type) {
    std::ostringstream detail;
    detail << "argument " << index
           << " mirror does not match call text; shadow '" << shadow
           << "', call argument type '" << formatted_type << "'";
    fail_verify("LirCallOp.arg_type_refs", detail.str());
  }
}

const LirFunctionSignatureStoreEntry* call_signature_store_entry(
    const LirModule& mod, const LirCallOp& call) {
  if (!call.callee_signature_ref.valid()) return nullptr;
  return mod.find_function_signature(call.callee_signature_ref);
}

struct DirectVoidFixedIntegerSignatureView {
  const std::optional<LirTypeRef>* return_type_ref = nullptr;
  const std::vector<LirTypeRef>* fixed_param_type_refs = nullptr;
  LirExtAttr return_ext_attr = LirExtAttr::None;
  bool is_variadic = false;
  bool has_unspecified_params = false;
  bool has_void_param_list = false;
  std::string_view parameter_field;
  std::string_view return_field;
};

std::optional<DirectVoidFixedIntegerSignatureView>
direct_void_fixed_integer_signature_view(const LirModule& mod,
                                         const LirCallOp& call) {
  if (call.return_type.kind() != LirTypeKind::Void ||
      call.direct_callee_link_name_id == kInvalidLinkName) {
    return std::nullopt;
  }
  if (const LirFunctionSignatureStoreEntry* signature =
          call_signature_store_entry(mod, call)) {
    return DirectVoidFixedIntegerSignatureView{
        &signature->return_type_ref,
        &signature->fixed_param_type_refs,
        signature->return_ext_attr,
        signature->is_variadic,
        false,
        signature->has_void_param_list,
        "LirCallOp.callee_signature_ref.fixed_param_type_refs",
        "LirCallOp.callee_signature_ref.return_type_ref"};
  }
  if (!call.callee_signature.has_value()) return std::nullopt;
  const LirCallSignature& signature = *call.callee_signature;
  return DirectVoidFixedIntegerSignatureView{
      &signature.return_type_ref,
      &signature.fixed_param_type_refs,
      signature.return_ext_attr,
      signature.is_variadic,
      signature.has_unspecified_params,
      signature.has_void_param_list,
      "LirCallOp.callee_signature.fixed_param_type_refs",
      "LirCallOp.callee_signature.return_type_ref"};
}

bool is_direct_void_fixed_integer_signature_claim(const LirModule& mod,
                                                  const LirCallOp& call) {
  const auto signature = direct_void_fixed_integer_signature_view(mod, call);
  return signature && !signature->is_variadic &&
         !signature->has_unspecified_params &&
         !signature->has_void_param_list &&
         signature->fixed_param_type_refs &&
         signature->fixed_param_type_refs->size() == 1 &&
         (*signature->fixed_param_type_refs)[0].kind() == LirTypeKind::Integer;
}

bool is_direct_void_fixed_integer_immediate_claim(const LirModule& mod,
                                                  const LirCallOp& call) {
  return is_direct_void_fixed_integer_signature_claim(mod, call) &&
         !call.structured_args.empty() &&
         call.structured_args[0].operand.kind() == LirOperandKind::Immediate &&
         call.structured_args[0].type_ref.kind() == LirTypeKind::Integer &&
         call.arg_type_refs.size() == 1 &&
         call.arg_type_refs[0].kind() == LirTypeKind::Integer;
}

bool is_direct_void_fixed_integer_ssa_claim(const LirModule& mod,
                                            const LirCallOp& call) {
  return is_direct_void_fixed_integer_signature_claim(mod, call) &&
         !call.structured_args.empty() &&
         call.structured_args[0].operand.kind() == LirOperandKind::SsaValue &&
         call.structured_args[0].type_ref.kind() == LirTypeKind::Integer &&
         call.arg_type_refs.size() == 1 &&
         call.arg_type_refs[0].kind() == LirTypeKind::Integer;
}

bool has_complete_direct_void_integer_immediate_authority(
    const LirModule& mod, const LirCallOp& call) {
  if (!is_direct_void_fixed_integer_immediate_claim(mod, call)) return false;
  const auto signature = direct_void_fixed_integer_signature_view(mod, call);
  if (!signature || !signature->return_type_ref ||
      !signature->fixed_param_type_refs ||
      signature->fixed_param_type_refs->empty()) {
    return false;
  }
  const LirTypeRef& parameter_type = (*signature->fixed_param_type_refs)[0];
  return signature->return_type_ref->has_value() &&
         **signature->return_type_ref == call.return_type &&
         call.structured_args.size() == 1 && call.arg_type_refs.size() == 1 &&
         call.structured_args[0].type_ref == parameter_type &&
         call.arg_type_refs[0] == parameter_type &&
         call.structured_args[0].operand.integer_immediate() &&
         call.structured_args[0].ext_attr == LirExtAttr::None &&
         call.result.empty() && !call.result.has_authority();
}

bool has_complete_direct_void_integer_ssa_authority(const LirModule& mod,
                                                    const LirCallOp& call) {
  if (!is_direct_void_fixed_integer_ssa_claim(mod, call)) return false;
  const auto signature = direct_void_fixed_integer_signature_view(mod, call);
  if (!signature || !signature->return_type_ref ||
      !signature->fixed_param_type_refs ||
      signature->fixed_param_type_refs->empty()) {
    return false;
  }
  const LirTypeRef& parameter_type = (*signature->fixed_param_type_refs)[0];
  return signature->return_type_ref->has_value() &&
         **signature->return_type_ref == call.return_type &&
         call.structured_args.size() == 1 && call.arg_type_refs.size() == 1 &&
         call.structured_args[0].type_ref == parameter_type &&
         call.arg_type_refs[0] == parameter_type &&
         call.structured_args[0].operand.value_id() &&
         call.structured_args[0].ext_attr == LirExtAttr::None &&
         call.result.empty() && !call.result.has_authority();
}

// Once every argument has native type authority and a fixed signature supplies
// the corresponding native parameter refs, presentation text and duplicate
// argument-type mirrors are deliberately not part of type validation. Older
// producers can still use the raw path below while they fill these facts
// incrementally.
bool has_complete_fixed_call_type_authority(const LirModule& mod,
                                            const LirCallOp& call) {
  const LirFunctionSignatureStoreEntry* store_signature =
      call_signature_store_entry(mod, call);
  const bool is_variadic =
      store_signature ? store_signature->is_variadic
                      : call.callee_signature && call.callee_signature->is_variadic;
  const bool has_void_param_list =
      store_signature ? store_signature->has_void_param_list
                      : call.callee_signature &&
                            call.callee_signature->has_void_param_list;
  const bool has_unspecified_params =
      store_signature ? false
                      : call.callee_signature &&
                            call.callee_signature->has_unspecified_params;
  const std::vector<LirTypeRef>* fixed_param_type_refs =
      store_signature ? &store_signature->fixed_param_type_refs
                      : call.callee_signature
                            ? &call.callee_signature->fixed_param_type_refs
                            : nullptr;
  if (!fixed_param_type_refs || has_unspecified_params || is_variadic ||
      has_void_param_list ||
      fixed_param_type_refs->size() != call.structured_args.size()) {
    return false;
  }
  for (size_t index = 0; index < call.structured_args.size(); ++index) {
    if (call.structured_args[index].type_ref != (*fixed_param_type_refs)[index]) {
      return false;
    }
  }
  return true;
}

bool is_native_scalar_floating_type(const LirTypeRef& type) {
  return type.kind() == LirTypeKind::Floating &&
         (type.str() == "float" || type.str() == "double" ||
          type.str() == "x86_fp80" || type.str() == "fp128");
}

bool is_direct_zero_arg_scalar_floating_result_claim(const LirCallOp& call) {
  return call.return_type.kind() == LirTypeKind::Floating &&
         is_native_scalar_floating_type(call.return_type) &&
         call.callee.kind() == LirOperandKind::Global &&
         call.direct_callee_link_name_id != kInvalidLinkName &&
         call.callee_signature.has_value() &&
         !call.callee_signature->is_variadic &&
         !call.callee_signature->has_unspecified_params &&
         call.callee_signature->has_void_param_list &&
         call.callee_signature->fixed_param_type_refs.empty() &&
         call.structured_args.empty() && call.arg_type_refs.empty();
}

bool is_direct_one_double_arg_scalar_floating_result_claim(const LirCallOp& call) {
  return call.return_type == LirTypeRef("double") &&
         call.callee.kind() == LirOperandKind::Global &&
         call.direct_callee_link_name_id != kInvalidLinkName &&
         call.callee_signature.has_value() &&
         !call.callee_signature->is_variadic &&
         !call.callee_signature->has_unspecified_params &&
         !call.callee_signature->has_void_param_list &&
         call.callee_signature->fixed_param_type_refs.size() == 1 &&
         call.callee_signature->fixed_param_type_refs[0] == LirTypeRef("double") &&
         call.structured_args.size() == 1;
}

void verify_direct_zero_arg_scalar_floating_result_call(
    const LirModule& mod, const LirFunction* owner_function,
    const LirCallOp& call) {
  const bool direct_scalar_floating_result_authority =
      is_native_scalar_floating_type(call.return_type) &&
      call.callee.kind() == LirOperandKind::Global &&
      call.direct_zero_arg_scalar_floating_call_authority.has_value();
  if (!direct_scalar_floating_result_authority &&
      !is_direct_zero_arg_scalar_floating_result_claim(call)) {
    return;
  }

  const auto& authority = call.direct_zero_arg_scalar_floating_call_authority;
  if (!authority.has_value()) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call requires native authority");
  }
  if (!call.result.value_id()) {
    fail_verify("LirCallOp.result",
                "direct zero-argument scalar floating call requires LirValueId result authority");
  }
  if (authority->result != *call.result.value_id() ||
      !authority->result.valid()) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call result authority must match the result operand");
  }
  if (!owner_function ||
      owner_function->link_name_id == kInvalidLinkName ||
      authority->owner != owner_function->link_name_id) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call owner must match the current function");
  }
  if (authority->callee != call.direct_callee_link_name_id ||
      authority->callee == kInvalidLinkName) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call callee authority must match the direct callee");
  }
  if (authority->return_type != call.return_type ||
      !is_native_scalar_floating_type(authority->return_type)) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call return authority must match the call return type");
  }
  if (authority->role !=
      LirDirectZeroArgScalarFloatingCallRole::ResultIntoFloatingBinaryLhs) {
    fail_verify("LirCallOp.direct_zero_arg_scalar_floating_call_authority",
                "direct zero-argument scalar floating call requires the selected result-consumer role");
  }
  if (!call.callee_signature.has_value() ||
      call.direct_callee_link_name_id == kInvalidLinkName ||
      call.callee_signature->is_variadic ||
      call.callee_signature->has_unspecified_params ||
      !call.callee_signature->has_void_param_list ||
      !call.callee_signature->fixed_param_type_refs.empty() ||
      !call.structured_args.empty() || !call.arg_type_refs.empty()) {
    fail_verify("LirCallOp.callee_signature",
                "direct zero-argument scalar floating call requires a fixed nonvariadic empty signature");
  }
  const LirFunction* callee_function = nullptr;
  for (const LirFunction& function : mod.functions) {
    if (function.link_name_id != call.direct_callee_link_name_id) continue;
    if (callee_function) {
      fail_verify("LirCallOp.direct_callee_link_name_id",
                  "direct zero-argument scalar floating call requires a unique module Function LinkNameId");
    }
    callee_function = &function;
  }
  if (!callee_function) {
    fail_verify("LirCallOp.direct_callee_link_name_id",
                "direct zero-argument scalar floating call requires a module-owned LinkNameId");
  }
  const LirCallSignature& signature = *call.callee_signature;
  if (!signature.return_type_ref.has_value() ||
      *signature.return_type_ref != call.return_type ||
      !is_native_scalar_floating_type(*signature.return_type_ref) ||
      call.return_ext_attr != LirExtAttr::None ||
      signature.return_ext_attr != LirExtAttr::None) {
    fail_verify("LirCallOp.callee_signature",
                "direct zero-argument scalar floating call requires matching native return authority");
  }
  if (!callee_function->signature_return_type_ref.has_value() ||
      *callee_function->signature_return_type_ref != call.return_type ||
      callee_function->signature_is_variadic ||
      !callee_function->signature_has_void_param_list ||
      !callee_function->signature_params.empty() ||
      !callee_function->signature_param_type_refs.empty()) {
    fail_verify("LirCallOp.direct_callee_link_name_id",
                "direct zero-argument scalar floating call requires a matching module Function signature");
  }
}

void verify_direct_one_double_arg_scalar_floating_result_call(
    const LirModule& mod, const LirFunction* owner_function,
    const LirCallOp& call) {
  const bool authority_present =
      call.callee.kind() == LirOperandKind::Global &&
      call.direct_one_double_arg_scalar_floating_call_authority.has_value();
  if (!authority_present &&
      !is_direct_one_double_arg_scalar_floating_result_claim(call)) {
    return;
  }

  constexpr std::string_view field =
      "LirCallOp.direct_one_double_arg_scalar_floating_call_authority";
  const auto& authority = call.direct_one_double_arg_scalar_floating_call_authority;
  if (!authority.has_value()) {
    fail_verify(field,
                "direct one-double-argument scalar floating call requires native authority");
  }
  if (!call.result.value_id()) {
    fail_verify("LirCallOp.result",
                "direct one-double-argument scalar floating call requires LirValueId result authority");
  }
  if (authority->result != *call.result.value_id() ||
      !authority->result.valid()) {
    fail_verify(field,
                "direct one-double-argument scalar floating call result authority must match the result operand");
  }
  if (!owner_function ||
      owner_function->link_name_id == kInvalidLinkName ||
      authority->owner != owner_function->link_name_id) {
    fail_verify(field,
                "direct one-double-argument scalar floating call owner must match the current function");
  }
  if (authority->callee != call.direct_callee_link_name_id ||
      authority->callee == kInvalidLinkName) {
    fail_verify(field,
                "direct one-double-argument scalar floating call callee authority must match the direct callee");
  }
  if (authority->return_type != LirTypeRef("double") ||
      authority->return_type != call.return_type ||
      authority->argument_type != LirTypeRef("double")) {
    fail_verify(field,
                "direct one-double-argument scalar floating call type authority must be double(double)");
  }
  if (authority->role !=
      LirDirectOneDoubleArgScalarFloatingCallRole::DirectCallResult) {
    fail_verify(field,
                "direct one-double-argument scalar floating call requires the selected call-result role");
  }
  if (!is_direct_one_double_arg_scalar_floating_result_claim(call)) {
    fail_verify("LirCallOp.callee_signature",
                "direct one-double-argument scalar floating call requires a fixed double(double) signature");
  }
  const LirFunction* callee_function = nullptr;
  for (const LirFunction& function : mod.functions) {
    if (function.link_name_id != call.direct_callee_link_name_id) continue;
    if (callee_function) {
      fail_verify("LirCallOp.direct_callee_link_name_id",
                  "direct one-double-argument scalar floating call requires a unique module Function LinkNameId");
    }
    callee_function = &function;
  }
  if (!callee_function) {
    fail_verify("LirCallOp.direct_callee_link_name_id",
                "direct one-double-argument scalar floating call requires a module-owned LinkNameId");
  }
  if (!callee_function->signature_return_type_ref.has_value() ||
      *callee_function->signature_return_type_ref != LirTypeRef("double") ||
      callee_function->signature_is_variadic ||
      callee_function->signature_has_void_param_list ||
      callee_function->signature_param_type_refs.size() != 1 ||
      callee_function->signature_param_type_refs[0] != LirTypeRef("double")) {
    fail_verify("LirCallOp.direct_callee_link_name_id",
                "direct one-double-argument scalar floating call requires a matching module Function signature");
  }
}

const LirFunction* find_unique_function_by_link_name(const LirModule& mod,
                                                     LinkNameId link_name_id,
                                                     std::string_view field) {
  const LirFunction* found = nullptr;
  for (const LirFunction& function : mod.functions) {
    if (function.link_name_id != link_name_id) continue;
    if (found) {
      fail_verify(field,
                  "direct call signature ref requires a unique module Function LinkNameId");
    }
    found = &function;
  }
  return found;
}

const LirModule::ExternDeclInfo* find_extern_decl_by_link_name(
    const LirModule& mod, LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) return nullptr;
  const auto it = mod.extern_decl_link_name_map.find(link_name_id);
  return it == mod.extern_decl_link_name_map.end() ? nullptr : &it->second;
}

void verify_call_callee_signature_ref(const LirModule& mod,
                                      const LirCallOp& call) {
  constexpr std::string_view field = "LirCallOp.callee_signature_ref";
  if (!call.callee_signature_ref.valid()) {
    return;
  }

  if (call.direct_callee_link_name_id == kInvalidLinkName) {
    fail_verify(field, "callee signature ref requires a direct module callee");
  }
  const LirFunction* callee = find_unique_function_by_link_name(
      mod, call.direct_callee_link_name_id, field);
  const LirModule::ExternDeclInfo* extern_decl =
      callee ? nullptr : find_extern_decl_by_link_name(
                            mod, call.direct_callee_link_name_id);
  if (!callee && !extern_decl) {
    fail_verify(field,
                "callee signature ref requires a matching module Function or extern declaration LinkNameId");
  }
  const LirFunctionSignatureRef resolved_signature_ref =
      callee ? callee->function_signature_ref
             : extern_decl->function_signature_ref;
  if (resolved_signature_ref.value != call.callee_signature_ref.value) {
    fail_verify(field,
                "callee signature ref must match the resolved module callee signature ref");
  }

  const LirFunctionSignatureStoreEntry* signature =
      mod.find_function_signature(call.callee_signature_ref);
  if (!signature || !signature->return_type_ref.has_value()) {
    fail_verify(field,
                "callee signature ref must name a module-owned function signature");
  }

  if (call.callee_signature.has_value()) {
    const LirCallSignature& retained = *call.callee_signature;
    if (retained.return_type_ref.has_value() !=
            signature->return_type_ref.has_value() ||
        (retained.return_type_ref.has_value() &&
         *retained.return_type_ref != *signature->return_type_ref) ||
        retained.return_ext_attr != signature->return_ext_attr ||
        retained.fixed_param_type_refs != signature->fixed_param_type_refs ||
        retained.fixed_param_types.size() != signature->fixed_param_is_byval.size() ||
        retained.is_variadic != signature->is_variadic ||
        retained.has_void_param_list != signature->has_void_param_list ||
        retained.has_unspecified_params) {
      std::ostringstream detail;
      detail << "callee signature ref disagrees with retained structured call signature"
             << " for callee '" << call.callee.str() << "'";
      fail_verify(field, detail.str());
    }
    for (std::size_t index = 0; index < retained.fixed_param_types.size();
         ++index) {
      const bool retained_spells_byval =
          retained.fixed_param_types[index].find("byval(") != std::string::npos;
      if (retained_spells_byval != signature->fixed_param_is_byval[index]) {
        fail_verify(field,
                    "callee signature ref byval facts disagree with retained structured call signature");
      }
    }
  }

  if (*signature->return_type_ref != call.return_type ||
      call.return_ext_attr != signature->return_ext_attr) {
    fail_verify(field,
                "callee signature ref return facts must match call-site return facts");
  }

  if (extern_decl) {
    if (signature->fixed_param_is_byval.size() !=
        signature->fixed_param_type_refs.size()) {
      fail_verify(field,
                  "extern callee signature ref byval facts must match fixed parameter refs");
    }
    for (std::size_t index = 0;
         index < signature->fixed_param_type_refs.size() &&
         index < call.structured_args.size();
         ++index) {
      const LirTypeRef& parameter_type = signature->fixed_param_type_refs[index];
      if (!parameter_type.has_struct_name_id()) {
        continue;
      }
      if (!signature->fixed_param_is_byval[index]) {
        fail_verify(field,
                    "extern aggregate fixed parameter requires a byval signature-store fact");
      }
      const LirTypeRef& argument_type = call.structured_args[index].type_ref;
      if (!argument_type.empty() &&
          argument_type.struct_name_id() != parameter_type.struct_name_id()) {
        fail_verify(field,
                    "extern aggregate fixed parameter family must match the structured argument family");
      }
    }
  }
}

void verify_call_callee_signature(const LirModule& mod, const LirCallOp& call,
                                  bool structured_authority_complete) {
  verify_call_callee_signature_ref(mod, call);
  const LirFunctionSignatureStoreEntry* store_signature =
      call_signature_store_entry(mod, call);
  if (!store_signature && !call.callee_signature.has_value()) return;

  const auto return_type_ref = [&]() -> const std::optional<LirTypeRef>& {
    return store_signature ? store_signature->return_type_ref
                           : call.callee_signature->return_type_ref;
  };
  const auto return_ext_attr = [&]() {
    return store_signature ? store_signature->return_ext_attr
                           : call.callee_signature->return_ext_attr;
  };
  const auto fixed_param_type_refs = [&]() -> const std::vector<LirTypeRef>& {
    return store_signature ? store_signature->fixed_param_type_refs
                           : call.callee_signature->fixed_param_type_refs;
  };
  const bool has_void_param_list =
      store_signature ? store_signature->has_void_param_list
                      : call.callee_signature->has_void_param_list;
  const bool is_variadic = store_signature ? store_signature->is_variadic
                                           : call.callee_signature->is_variadic;
  const bool has_unspecified_params =
      store_signature ? false : call.callee_signature->has_unspecified_params;
  const bool direct_integer_result_contract =
      call.return_type.kind() == LirTypeKind::Integer &&
      call.direct_callee_link_name_id != kInvalidLinkName;
  if (direct_integer_result_contract && !return_type_ref().has_value()) {
    fail_verify("LirCallOp.callee_signature.return_type_ref",
                "structured direct integer call requires a return type ref");
  }
  if (return_type_ref().has_value()) {
    verify_call_return_type_ref_mirror(mod, *return_type_ref());
    if (*return_type_ref() != call.return_type ||
        return_ext_attr() != call.return_ext_attr) {
      fail_verify("LirCallOp.callee_signature.return_type_ref",
                  "structured callee return type must match call return type");
    }
  }

  if (has_void_param_list) {
    if (!fixed_param_type_refs().empty() ||
        (!store_signature && !structured_authority_complete &&
         !call.callee_signature->fixed_param_types.empty())) {
      fail_verify("LirCallOp.callee_signature",
                  "void parameter list must not carry fixed parameter mirrors");
    }
    if (is_variadic) {
      fail_verify("LirCallOp.callee_signature",
                  "void parameter list must not be variadic");
    }
  }

  if (!store_signature && !structured_authority_complete &&
      call.callee_signature->fixed_param_types.size() !=
          call.callee_signature->fixed_param_type_refs.size()) {
    fail_verify("LirCallOp.callee_signature.fixed_param_type_refs",
                "fixed parameter type mirrors must match fixed parameter count");
  }

  if (!store_signature && !structured_authority_complete) {
    for (size_t index = 0;
         index < call.callee_signature->fixed_param_type_refs.size(); ++index) {
      verify_call_arg_type_ref_mirror(
          mod, call.callee_signature->fixed_param_type_refs[index],
          call.callee_signature->fixed_param_types[index], index);
    }
  }

  if (!has_unspecified_params && !is_variadic && !has_void_param_list &&
      fixed_param_type_refs().size() == call.structured_args.size()) {
    for (size_t index = 0; index < call.structured_args.size(); ++index) {
      const LirTypeRef& structured_type = call.structured_args[index].type_ref;
      if (structured_type.empty()) continue;
      require_module_type_ref(mod, structured_type,
                              "LirCallOp.structured_args.type_ref");
      if (structured_type != fixed_param_type_refs()[index]) {
        fail_verify("LirCallOp.structured_args.type_ref",
                    "structured argument type must match fixed parameter type");
      }
    }
  }

  if (structured_authority_complete) {
    for (size_t index = 0; index < fixed_param_type_refs().size(); ++index) {
      const auto verify_native_param_type = [&](const LirTypeRef& type,
                                                std::string_view field,
                                                std::string_view role) {
        require_type_ref(type, field);
        if (type.has_anonymous_struct_layout()) {
          require_module_type_ref(mod, type, field);
        }
        verify_known_struct_type_ref_mirror(mod, type, field, role);
        if (type.has_struct_name_id() &&
            type.str() != mod.struct_names.spelling(type.struct_name_id())) {
          fail_verify(field,
                      "native structured call type text must match its StructNameId");
        }
      };
      verify_native_param_type(
          fixed_param_type_refs()[index],
          "LirCallOp.callee_signature.fixed_param_type_refs", "parameter");
      verify_native_param_type(call.structured_args[index].type_ref,
                               "LirCallOp.structured_args.type_ref", "argument");
      if (call.structured_args[index].type_ref != fixed_param_type_refs()[index]) {
        fail_verify("LirCallOp.structured_args.type_ref",
                    "structured argument type must match fixed parameter type");
      }
    }
  }

  if (!has_unspecified_params && !structured_authority_complete) {
    const auto parsed = parse_lir_typed_call_or_infer_params(call);
    if (!parsed.has_value()) {
      fail_verify("LirCallOp.callee_signature",
                  "structured callee signature does not match call arguments");
    }
  }
}

void verify_result_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field, {LirOperandKind::SsaValue});
}

void verify_value_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue,
                        LirOperandKind::Global,
                        LirOperandKind::Immediate,
                        LirOperandKind::SpecialToken});
}

void verify_store_value_operand(const LirOperand& operand,
                                std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue,
                        LirOperandKind::DirectConstant,
                        LirOperandKind::Global,
                        LirOperandKind::Immediate,
                        LirOperandKind::SpecialToken});
}

void verify_pointer_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue, LirOperandKind::Global});
}

void verify_cast_op_authority(const LirCastOp& op) {
  switch (op.kind) {
    case LirCastKind::Trunc:
    case LirCastKind::ZExt:
    case LirCastKind::SExt:
    case LirCastKind::FPTrunc:
    case LirCastKind::FPExt:
    case LirCastKind::FPToSI:
    case LirCastKind::FPToUI:
    case LirCastKind::SIToFP:
    case LirCastKind::UIToFP:
    case LirCastKind::PtrToInt:
    case LirCastKind::IntToPtr:
    case LirCastKind::Bitcast:
      break;
    default:
      fail_verify("LirCastOp.kind", "invalid native cast kind");
  }

  // The explicit standalone contract keeps this result requirement separate
  // from compatibility casts and logical/PHI lowering. Once selected, the
  // result identity is native and the function ownership pass rejects
  // invalid, duplicate, and foreign IDs without rendered spelling lookups.
  if (op.requires_native_result_authority && !op.result.value_id()) {
    fail_verify("LirCastOp.result",
                "standalone native cast requires LirValueId result authority");
  }
  if (!op.result.value_id()) {
    return;
  }
  const auto floating_width = [](const LirTypeRef& type)
      -> std::optional<unsigned> {
    switch (type.builtin_type().value_or(LirBuiltinType::Void)) {
      case LirBuiltinType::Half: return 16;
      case LirBuiltinType::Float: return 32;
      case LirBuiltinType::Double: return 64;
      case LirBuiltinType::X86Fp80: return 80;
      case LirBuiltinType::Fp128: return 128;
      default: return std::nullopt;
    }
  };
  if (op.kind == LirCastKind::FPToSI || op.kind == LirCastKind::FPToUI) {
    if (op.from_type.kind() != LirTypeKind::Floating ||
        op.to_type.kind() != LirTypeKind::Integer) {
      fail_verify(
          "LirCastOp.from_type",
          "authoritative floating-to-integer cast requires floating-to-integer endpoint type refs");
    }
    if (!floating_width(op.from_type)) {
      fail_verify("LirCastOp.from_type",
                  "authoritative floating-to-integer cast requires an exact floating source type");
    }
    if (!op.to_type.integer_bit_width()) {
      fail_verify(
          "LirCastOp.to_type",
          "authoritative floating-to-integer cast requires an exact integer destination type");
    }
    return;
  }
  if (op.kind == LirCastKind::SIToFP || op.kind == LirCastKind::UIToFP) {
    if (op.from_type.kind() != LirTypeKind::Integer ||
        op.to_type.kind() != LirTypeKind::Floating) {
      fail_verify(
          "LirCastOp.from_type",
          "authoritative integer-to-floating cast requires integer-to-floating endpoint type refs");
    }
    if (!op.from_type.integer_bit_width()) {
      fail_verify("LirCastOp.from_type",
                  "authoritative integer-to-floating cast requires an exact integer source type");
    }
    if (!floating_width(op.to_type)) {
      fail_verify(
          "LirCastOp.to_type",
          "authoritative integer-to-floating cast requires an exact floating destination type");
    }
    return;
  }
  if (op.kind == LirCastKind::FPTrunc || op.kind == LirCastKind::FPExt) {
    if (op.from_type.kind() != LirTypeKind::Floating ||
        op.to_type.kind() != LirTypeKind::Floating) {
      fail_verify("LirCastOp.from_type",
                  "authoritative floating cast requires floating endpoint type refs");
    }
    const std::optional<unsigned> from_width = floating_width(op.from_type);
    const std::optional<unsigned> to_width = floating_width(op.to_type);
    if (!from_width || !to_width) {
      fail_verify("LirCastOp.from_type",
                  "authoritative floating cast requires exact floating endpoints");
    }
    if (op.kind == LirCastKind::FPTrunc && *to_width >= *from_width) {
      fail_verify("LirCastOp.to_type",
                  "FPTrunc requires a narrower floating destination type");
    }
    if (op.kind == LirCastKind::FPExt && *to_width <= *from_width) {
      fail_verify("LirCastOp.to_type",
                  "FPExt requires a wider floating destination type");
    }
    return;
  }
  if (op.kind == LirCastKind::IntToPtr) {
    if (op.from_type.kind() != LirTypeKind::Integer ||
        op.to_type.kind() != LirTypeKind::Pointer) {
      fail_verify("LirCastOp.from_type",
                  "authoritative integer-to-pointer cast requires integer-to-pointer endpoint type refs");
    }
    if (!op.from_type.integer_bit_width()) {
      fail_verify("LirCastOp.from_type",
                  "authoritative integer-to-pointer cast requires an exact integer source type");
    }
    return;
  }
  if (op.kind == LirCastKind::PtrToInt) {
    if (op.from_type.kind() != LirTypeKind::Pointer ||
        op.to_type.kind() != LirTypeKind::Integer) {
      fail_verify("LirCastOp.from_type",
                  "authoritative pointer-to-integer cast requires pointer-to-integer endpoint type refs");
    }
    if (!op.to_type.integer_bit_width()) {
      fail_verify("LirCastOp.to_type",
                  "authoritative pointer-to-integer cast requires an exact integer destination type");
    }
    return;
  }
  if (op.from_type.kind() != LirTypeKind::Integer ||
      op.to_type.kind() != LirTypeKind::Integer) {
    fail_verify("LirCastOp.from_type",
                "authoritative scalar integer cast requires integer type refs");
  }
  const std::optional<unsigned> from_width =
      op.from_type.integer_bit_width();
  const std::optional<unsigned> to_width = op.to_type.integer_bit_width();
  if (!from_width || !to_width) {
    fail_verify("LirCastOp.from_type",
                "authoritative scalar integer cast requires exact widths");
  }
  if (op.kind == LirCastKind::Trunc) {
    if (*to_width >= *from_width) {
      fail_verify("LirCastOp.to_type",
                  "integer trunc requires a narrower destination type");
    }
    return;
  }
  if (op.kind == LirCastKind::ZExt || op.kind == LirCastKind::SExt) {
    if (*to_width <= *from_width) {
      fail_verify("LirCastOp.to_type",
                  "integer extension requires a wider destination type");
    }
    return;
  }
  fail_verify("LirCastOp.kind",
              "authoritative scalar integer cast requires trunc/zext/sext");
}

bool is_integer_cmp_predicate(LirCmpPredicate predicate) {
  switch (predicate) {
    case LirCmpPredicate::Eq:
    case LirCmpPredicate::Ne:
    case LirCmpPredicate::Sgt:
    case LirCmpPredicate::Sge:
    case LirCmpPredicate::Slt:
    case LirCmpPredicate::Sle:
    case LirCmpPredicate::Ugt:
    case LirCmpPredicate::Uge:
    case LirCmpPredicate::Ult:
    case LirCmpPredicate::Ule:
      return true;
    default:
      return false;
  }
}

bool is_floating_cmp_predicate(LirCmpPredicate predicate) {
  switch (predicate) {
    case LirCmpPredicate::OEq:
    case LirCmpPredicate::ONe:
    case LirCmpPredicate::OGt:
    case LirCmpPredicate::OGe:
    case LirCmpPredicate::OLt:
    case LirCmpPredicate::OLe:
    case LirCmpPredicate::UEq:
    case LirCmpPredicate::UNe:
    case LirCmpPredicate::Ord:
    case LirCmpPredicate::Uno:
      return true;
    default:
      return false;
  }
}

bool is_floating_binary_opcode(LirBinaryOpcode opcode) {
  switch (opcode) {
    case LirBinaryOpcode::FAdd:
    case LirBinaryOpcode::FSub:
    case LirBinaryOpcode::FMul:
    case LirBinaryOpcode::FDiv:
    case LirBinaryOpcode::FRem:
    case LirBinaryOpcode::FNeg:
      return true;
    default:
      return false;
  }
}

bool integer_immediate_representable(long long value, unsigned bit_width);

const LirCompactScalarType* compact_scalar_binop_type(const LirBinOp& op,
                                                      std::string_view field) {
  if (!op.compact_scalar_type) return nullptr;
  const auto selected_scalar =
      LirCompactScalarType::from_type_ref(op.compact_scalar_type->type);
  if (!selected_scalar || op.compact_scalar_type->type != op.type_str) {
    fail_verify(field,
                "must mirror one selected integer or floating scalar binop type");
  }
  return &*op.compact_scalar_type;
}

const LirPhiBoundaryValueType& phi_boundary_value_type(const LirPhiOp& op,
                                                       std::string_view field) {
  if (!op.boundary_value_type) {
    fail_verify(field, "must carry a selected PHI boundary value type");
  }
  const auto selected =
      LirPhiBoundaryValueType::from_type_ref(op.boundary_value_type->type);
  if (!selected || selected->kind != op.boundary_value_type->kind ||
      op.boundary_value_type->type != op.type_str) {
    fail_verify(field,
                "must mirror one selected scalar, vector, aggregate, or pointer PHI value type");
  }
  return *op.boundary_value_type;
}

void verify_bin_op_authority(const LirBinOp& op) {
  if (!op.result.value_id()) return;
  const std::optional<LirBinaryOpcode> opcode = op.opcode.typed();
  if (!opcode) return;
  const LirCompactScalarType* compact_scalar =
      compact_scalar_binop_type(op, "LirBinOp.compact_scalar_type");
  const LirTypeRef& binary_type =
      compact_scalar ? compact_scalar->type : op.type_str;
  const bool floating_opcode = is_floating_binary_opcode(*opcode);
  const bool floating_type = binary_type.kind() == LirTypeKind::Floating;
  if (floating_opcode != floating_type) {
    fail_verify(compact_scalar ? "LirBinOp.compact_scalar_type" : "LirBinOp.type_str",
                "authoritative floating binary opcode and type must agree");
  }
  if (!floating_opcode && binary_type.kind() == LirTypeKind::Integer) {
    const auto verify_integer_operand_authority = [&binary_type](
        const LirOperand& operand, std::string_view field) {
      if (!operand.has_authority() || operand.value_id()) return;
      if (const LirIntegerImmediate* immediate = operand.integer_immediate()) {
        const std::optional<unsigned> width = binary_type.integer_bit_width();
        if (!width ||
            !integer_immediate_representable(immediate->value, *width)) {
          fail_verify(field,
                      "integer immediate is not representable by binary type");
        }
        return;
      }
      fail_verify(field,
                  "authoritative integer binary operand requires SSA or integer immediate authority");
    };
    verify_integer_operand_authority(op.lhs, "LirBinOp.lhs");
    verify_integer_operand_authority(op.rhs, "LirBinOp.rhs");
  }
}

void verify_cmp_op_authority(const LirCmpOp& op) {
  if (!op.result.value_id()) return;
  const std::optional<LirCmpPredicate> predicate = op.predicate.typed();
  if (!predicate) return;
  if (op.is_float) {
    if (op.type_str.kind() != LirTypeKind::Floating) {
      fail_verify("LirCmpOp.type_str",
                  "authoritative scalar floating compare requires floating type authority");
    }
    if (!is_floating_cmp_predicate(*predicate)) {
      fail_verify("LirCmpOp.predicate",
                  "authoritative scalar floating compare requires a floating predicate");
    }
    return;
  }
  if (op.type_str.kind() != LirTypeKind::Integer) {
    fail_verify("LirCmpOp.type_str",
                "authoritative scalar integer compare requires integer type authority");
  }
  if (!is_integer_cmp_predicate(*predicate)) {
    fail_verify("LirCmpOp.predicate",
                "authoritative scalar integer compare requires an integer predicate");
  }
}

void verify_select_op_authority(const LirSelectOp& op) {
  const bool scalar_integer_claim =
      op.type_str.kind() == LirTypeKind::Integer;
  if (scalar_integer_claim && !op.result.value_id()) {
    fail_verify("LirSelectOp.result",
                "scalar integer select requires native result authority");
  }
  if (op.result.value_id() && !scalar_integer_claim) {
    fail_verify("LirSelectOp.type_str",
                "authoritative scalar select requires integer type authority");
  }
  if (!scalar_integer_claim) return;
  require_operand_kind(op.cond, "LirSelectOp.cond",
                       {LirOperandKind::SsaValue});
}

void verify_abs_op_authority(const LirAbsOp& op) {
  const bool scalar_integer_claim =
      op.int_type.kind() == LirTypeKind::Integer;
  if (scalar_integer_claim && !op.result.value_id()) {
    fail_verify("LirAbsOp.result",
                "scalar integer abs requires native result authority");
  }
  if (op.result.value_id() && !scalar_integer_claim) {
    fail_verify("LirAbsOp.int_type",
                "authoritative scalar abs requires integer type authority");
  }
  if (!scalar_integer_claim) return;
  require_operand_kind(op.arg, "LirAbsOp.arg",
                       {LirOperandKind::SsaValue,
                        LirOperandKind::Immediate});
}

void verify_optional_count_operand(const LirOperand& operand,
                                   std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue, LirOperandKind::Immediate},
                       true);
}

bool integer_immediate_representable(long long value, unsigned bit_width) {
  if (bit_width == 0) return false;
  if (bit_width >= 64) return true;
  if (value < 0) {
    const long long minimum = -(1LL << (bit_width - 1));
    return value >= minimum;
  }
  const unsigned long long maximum = (1ULL << bit_width) - 1ULL;
  return static_cast<unsigned long long>(value) <= maximum;
}

void verify_integer_cmp_operand_authority(const LirCmpOp& op) {
  const auto verify_operand = [&op](const LirOperand& operand,
                                    std::string_view field) {
    if (!operand.has_authority() || operand.value_id()) return;
    if (const LirIntegerImmediate* immediate = operand.integer_immediate()) {
      const std::optional<unsigned> width = op.type_str.integer_bit_width();
      if (!width ||
          !integer_immediate_representable(immediate->value, *width)) {
        fail_verify(field,
                    "integer immediate is not representable by comparison type");
      }
      return;
    }
    fail_verify(field,
                "authoritative integer comparison operand requires SSA or integer immediate authority");
  };
  verify_operand(op.lhs, "LirCmpOp.lhs");
  verify_operand(op.rhs, "LirCmpOp.rhs");
}

bool is_integer_boolean_flag_call_claim(const LirCallOp& call) {
  return call.zero_count_behavior.has_value() ||
         call.intrinsic_kind == LirIntrinsicKind::Cttz ||
         call.intrinsic_kind == LirIntrinsicKind::Ctlz;
}

struct IntegerIntrinsicSignatureView {
  const std::optional<LirTypeRef>* return_type_ref = nullptr;
  const std::vector<LirTypeRef>* fixed_param_type_refs = nullptr;
  LirExtAttr return_ext_attr = LirExtAttr::None;
  bool is_variadic = false;
  bool has_unspecified_params = false;
  bool has_void_param_list = false;
};

std::optional<IntegerIntrinsicSignatureView> integer_intrinsic_signature_view(
    const LirModule& mod, const LirCallOp& call) {
  if (const LirFunctionSignatureStoreEntry* signature =
          call_signature_store_entry(mod, call)) {
    return IntegerIntrinsicSignatureView{
        &signature->return_type_ref,
        &signature->fixed_param_type_refs,
        signature->return_ext_attr,
        signature->is_variadic,
        false,
        signature->has_void_param_list};
  }
  if (!call.callee_signature.has_value()) return std::nullopt;
  const LirCallSignature& signature = *call.callee_signature;
  return IntegerIntrinsicSignatureView{
      &signature.return_type_ref,
      &signature.fixed_param_type_refs,
      signature.return_ext_attr,
      signature.is_variadic,
      signature.has_unspecified_params,
      signature.has_void_param_list};
}

bool has_complete_integer_boolean_flag_call_authority(const LirModule& mod,
                                                      const LirCallOp& call) {
  if (!is_integer_boolean_flag_call_claim(call) ||
      (call.intrinsic_kind != LirIntrinsicKind::Cttz &&
       call.intrinsic_kind != LirIntrinsicKind::Ctlz) ||
      !call.result.value_id() ||
      call.return_type.kind() != LirTypeKind::Integer ||
      !call.callee.link_name_id() ||
      call.direct_callee_link_name_id != *call.callee.link_name_id() ||
      !call.zero_count_behavior.has_value()) {
    return false;
  }
  const auto signature = integer_intrinsic_signature_view(mod, call);
  if (!signature || !signature->return_type_ref ||
      !signature->return_type_ref->has_value() ||
      !signature->fixed_param_type_refs || signature->is_variadic ||
      signature->has_unspecified_params || signature->has_void_param_list ||
      signature->fixed_param_type_refs->size() != 2 ||
      call.arg_type_refs.size() != 2 || call.structured_args.size() != 2) {
    return false;
  }
  const LirTypeRef& integer_type = call.return_type;
  const LirTypeRef i1_type = LirTypeRef::integer(1);
  const std::int64_t expected_flag =
      *call.zero_count_behavior == LirZeroCountBehavior::Defined ? 0 : 1;
  return **signature->return_type_ref == integer_type &&
         (*signature->fixed_param_type_refs)[0] == integer_type &&
         (*signature->fixed_param_type_refs)[1] == i1_type &&
         call.arg_type_refs[0] == integer_type &&
         call.arg_type_refs[1] == i1_type &&
         call.structured_args[0].type_ref == integer_type &&
         call.structured_args[1].type_ref == i1_type &&
         call.structured_args[1].operand.integer_immediate() &&
         call.structured_args[1].operand.integer_immediate()->value == expected_flag;
}

void verify_integer_boolean_flag_call_authority(const LirModule& mod,
                                                const LirCallOp& call) {
  if (!is_integer_boolean_flag_call_claim(call)) return;
  const auto signature = integer_intrinsic_signature_view(mod, call);
  if (!has_complete_integer_boolean_flag_call_authority(mod, call) ||
      !signature) {
    fail_verify("LirCallOp",
                "authoritative integer boolean-flag call requires complete native callee/signature/argument authority");
  }
  const LinkNameId callee_id = *call.callee.link_name_id();
  if (callee_id == kInvalidLinkName || mod.link_names.spelling(callee_id).empty()) {
    fail_verify("LirCallOp.callee",
                "authoritative integer boolean-flag callee must resolve in the module");
  }
  if (call.callee.kind() != LirOperandKind::Global ||
      call.return_ext_attr != LirExtAttr::None ||
      signature->return_ext_attr != LirExtAttr::None ||
      call.structured_args[0].ext_attr != LirExtAttr::None ||
      call.structured_args[1].ext_attr != LirExtAttr::None) {
    fail_verify("LirCallOp",
                "authoritative integer boolean-flag call has conflicting native shape");
  }
  const LirOperand& value_argument = call.structured_args[0].operand;
  if (value_argument.has_authority() && !value_argument.value_id()) {
    fail_verify("LirCallOp.structured_args[0].operand",
                "integer value argument requires SSA authority when available");
  }
  if (value_argument.kind() != LirOperandKind::SsaValue &&
      value_argument.kind() != LirOperandKind::Immediate) {
    fail_verify("LirCallOp.structured_args[0].operand",
                "integer value argument requires SSA or immediate presentation");
  }
}

bool has_complete_integer_count_call_authority(const LirModule& mod,
                                               const LirCallOp& call) {
  if (call.intrinsic_kind != LirIntrinsicKind::Ctpop ||
      call.zero_count_behavior.has_value() || !call.result.value_id() ||
      call.return_type.kind() != LirTypeKind::Integer ||
      !call.callee.link_name_id() ||
      call.direct_callee_link_name_id != *call.callee.link_name_id()) {
    return false;
  }
  const auto signature = integer_intrinsic_signature_view(mod, call);
  if (!signature || !signature->return_type_ref ||
      !signature->return_type_ref->has_value() ||
      !signature->fixed_param_type_refs || signature->is_variadic ||
      signature->has_unspecified_params || signature->has_void_param_list ||
      signature->fixed_param_type_refs->size() != 1 ||
      call.arg_type_refs.size() != 1 || call.structured_args.size() != 1) {
    return false;
  }
  return **signature->return_type_ref == call.return_type &&
         (*signature->fixed_param_type_refs)[0] == call.return_type &&
         call.arg_type_refs[0] == call.return_type &&
         call.structured_args[0].type_ref == call.return_type;
}

void verify_integer_count_call_authority(const LirModule& mod,
                                         const LirCallOp& call) {
  if (call.intrinsic_kind != LirIntrinsicKind::Ctpop) return;
  const auto signature = integer_intrinsic_signature_view(mod, call);
  if (!has_complete_integer_count_call_authority(mod, call) || !signature) {
    fail_verify("LirCallOp",
                "authoritative integer count call requires complete native callee/signature/argument authority and no zero-count behavior");
  }
  const LinkNameId callee_id = *call.callee.link_name_id();
  if (callee_id == kInvalidLinkName || mod.link_names.spelling(callee_id).empty()) {
    fail_verify("LirCallOp.callee",
                "authoritative integer count callee must resolve in the module");
  }
  if (call.callee.kind() != LirOperandKind::Global ||
      call.return_ext_attr != LirExtAttr::None ||
      signature->return_ext_attr != LirExtAttr::None ||
      call.structured_args[0].ext_attr != LirExtAttr::None) {
    fail_verify("LirCallOp",
                "authoritative integer count call has conflicting native shape");
  }
  const LirOperand& value_argument = call.structured_args[0].operand;
  if (value_argument.has_authority() && !value_argument.value_id()) {
    fail_verify("LirCallOp.structured_args[0].operand",
                "integer count value argument requires SSA authority when available");
  }
  if (value_argument.kind() != LirOperandKind::SsaValue &&
      value_argument.kind() != LirOperandKind::Immediate) {
    fail_verify("LirCallOp.structured_args[0].operand",
                "integer count value argument requires SSA or immediate presentation");
  }
}

void verify_direct_void_fixed_integer_immediate_call(const LirModule& mod,
                                                     const LirCallOp& call) {
  const auto signature = direct_void_fixed_integer_signature_view(mod, call);
  if (!signature ||
      !is_direct_void_fixed_integer_signature_claim(mod, call)) {
    return;
  }

  if (call.structured_args.size() == 1) {
    const LirOperand& operand = call.structured_args[0].operand;
    if (operand.has_authority() && !operand.integer_immediate() &&
        !operand.value_id()) {
      fail_verify("LirCallOp.structured_args.operand",
                  "fixed integer argument has the wrong authority alternative");
    }
  }
  if (!is_direct_void_fixed_integer_immediate_claim(mod, call)) return;

  if (!signature->return_type_ref || !signature->return_type_ref->has_value() ||
      (*signature->return_type_ref)->kind() != LirTypeKind::Void ||
      **signature->return_type_ref != call.return_type) {
    fail_verify(signature->return_field,
                "direct void immediate call requires exact void return type authority");
  }
  if (!signature->fixed_param_type_refs ||
      signature->fixed_param_type_refs->size() != 1) {
    fail_verify(signature->parameter_field,
                "direct void immediate call requires one fixed parameter");
  }
  if (call.structured_args.size() != 1 || call.arg_type_refs.size() != 1) {
    fail_verify("LirCallOp.structured_args",
                "direct void immediate call requires one typed structured argument");
  }

  const LirTypeRef& parameter_type = (*signature->fixed_param_type_refs)[0];
  const LirCallArg& argument = call.structured_args[0];
  require_module_type_ref(mod, parameter_type, signature->parameter_field);
  require_module_type_ref(mod, argument.type_ref,
                          "LirCallOp.structured_args.type_ref");
  require_module_type_ref(mod, call.arg_type_refs[0],
                          "LirCallOp.arg_type_refs");
  if (argument.type_ref != parameter_type ||
      call.arg_type_refs[0] != parameter_type) {
    fail_verify("LirCallOp.structured_args.type_ref",
                "structured argument type must match fixed parameter type");
  }
  require_operand_kind(argument.operand, "LirCallOp.structured_args.operand",
                       {LirOperandKind::Immediate});
  const LirIntegerImmediate* immediate = argument.operand.integer_immediate();
  if (!immediate) {
    fail_verify("LirCallOp.structured_args.operand",
                "fixed integer immediate requires native payload authority");
  }
  const std::optional<unsigned> width = parameter_type.integer_bit_width();
  if (!width || !integer_immediate_representable(immediate->value, *width)) {
    fail_verify("LirCallOp.structured_args.operand",
                "integer immediate is not representable by fixed parameter type");
  }
  if (argument.ext_attr != LirExtAttr::None) {
    fail_verify("LirCallOp.structured_args.ext_attr",
                "fixed nonvariadic immediate must not carry an extension attribute");
  }
}

void verify_direct_void_fixed_integer_ssa_call(const LirModule& mod,
                                               const LirCallOp& call) {
  const auto signature = direct_void_fixed_integer_signature_view(mod, call);
  if (!signature || !is_direct_void_fixed_integer_ssa_claim(mod, call)) return;

  if (!signature->return_type_ref || !signature->return_type_ref->has_value() ||
      (*signature->return_type_ref)->kind() != LirTypeKind::Void ||
      **signature->return_type_ref != call.return_type) {
    fail_verify(signature->return_field,
                "direct void SSA call requires exact void return type authority");
  }
  if (!signature->fixed_param_type_refs ||
      signature->fixed_param_type_refs->size() != 1) {
    fail_verify(signature->parameter_field,
                "direct void SSA call requires one fixed parameter");
  }
  if (call.structured_args.size() != 1 || call.arg_type_refs.size() != 1) {
    fail_verify("LirCallOp.structured_args",
                "direct void SSA call requires one typed structured argument");
  }

  const LirTypeRef& parameter_type = (*signature->fixed_param_type_refs)[0];
  const LirCallArg& argument = call.structured_args[0];
  require_module_type_ref(mod, parameter_type, signature->parameter_field);
  require_module_type_ref(mod, argument.type_ref,
                          "LirCallOp.structured_args.type_ref");
  require_module_type_ref(mod, call.arg_type_refs[0],
                          "LirCallOp.arg_type_refs");
  if (argument.type_ref != parameter_type ||
      call.arg_type_refs[0] != parameter_type) {
    fail_verify("LirCallOp.structured_args.type_ref",
                "structured SSA argument type must match fixed parameter type");
  }
  require_operand_kind(argument.operand, "LirCallOp.structured_args.operand",
                       {LirOperandKind::SsaValue});
  if (!argument.operand.value_id()) {
    fail_verify("LirCallOp.structured_args.operand",
                "fixed SSA argument requires native value authority");
  }
  if (argument.ext_attr != LirExtAttr::None) {
    fail_verify("LirCallOp.structured_args.ext_attr",
                "fixed nonvariadic SSA argument must not carry an extension attribute");
  }
}

void verify_global_pointer_owner(const LirModule& mod,
                                 const LirOperand& pointer,
                                 std::string_view field,
                                 std::string_view operation) {
  const LinkNameId* link_name_id = pointer.link_name_id();
  if (!link_name_id) {
    fail_verify(field, "global " + std::string(operation) +
                           " pointer requires LinkNameId authority");
  }
  if (*link_name_id == kInvalidLinkName ||
      mod.link_names.spelling(*link_name_id).empty()) {
    fail_verify(field, "global " + std::string(operation) +
                           " pointer LinkNameId must resolve in the module");
  }

  const std::size_t owner_count = static_cast<std::size_t>(std::count_if(
      mod.globals.begin(), mod.globals.end(), [&](const LirGlobal& global) {
        return global.link_name_id == *link_name_id;
      }));
  if (owner_count != 1) {
    fail_verify(field, owner_count == 0
                           ? "global " + std::string(operation) +
                                 " pointer LinkNameId has no LirGlobal owner"
                           : "global " + std::string(operation) +
                                 " pointer LinkNameId has ambiguous LirGlobal ownership");
  }
}

void verify_global_store_authority(const LirModule& mod,
                                   const LirStoreOp& op) {
  // CC-STORE-1 owns the direct-global pointer plus native integer-immediate
  // shape. Other pointer/value rows remain phased monostate compatibility.
  if (op.ptr.has_authority() && !op.ptr.value_id() && !op.ptr.link_name_id()) {
    fail_verify("LirStoreOp.ptr",
                "store pointer has the wrong authority alternative");
  }
  if (op.ptr.kind() != LirOperandKind::Global) return;
  verify_global_pointer_owner(mod, op.ptr, "LirStoreOp.ptr", "store");

  if (op.type_str.kind() != LirTypeKind::Integer) return;
  if (op.val.kind() == LirOperandKind::Immediate) {
    const LirIntegerImmediate* immediate = op.val.integer_immediate();
    if (!immediate) {
      fail_verify("LirStoreOp.val",
                  "integer immediate store requires native authority");
    }
    const std::optional<unsigned> bit_width =
        op.type_str.integer_bit_width();
    if (!bit_width.has_value() ||
        !integer_immediate_representable(immediate->value, *bit_width)) {
      fail_verify("LirStoreOp.val",
                  "integer immediate is not representable by the store type");
    }
    return;
  }

  if (op.val.has_authority() && !op.val.value_id()) {
    fail_verify("LirStoreOp.val",
                "integer store value has the wrong authority alternative");
  }
}

void verify_native_local_store_authority(const LirStoreOp& op) {
  if (!op.requires_native_store_authority) return;
  const auto* authority = op.local_object_authority
                              ? &*op.local_object_authority
                              : nullptr;
  const auto* value = op.val.integer_immediate();
  const auto width = op.type_str.integer_bit_width();
  if (!authority || op.type_str.kind() != LirTypeKind::Integer || !value ||
      !width || !integer_immediate_representable(value->value, *width) ||
      authority->pointee_type != op.type_str) {
    fail_verify("LirStoreOp.local_object_authority",
                "selected local-scalar store requires native immediate, matching type, and local authority");
  }
}

void verify_global_load_authority(const LirModule& mod,
                                  const LirLoadOp& op) {
  // CC-LOAD-1 owns direct-global pointer and result identity. Local/SSA
  // pointer loads retain phased monostate compatibility.
  if (op.ptr.has_authority() && !op.ptr.value_id() && !op.ptr.link_name_id()) {
    fail_verify("LirLoadOp.ptr",
                "load pointer has the wrong authority alternative");
  }
  if (op.ptr.kind() != LirOperandKind::Global) return;
  verify_global_pointer_owner(mod, op.ptr, "LirLoadOp.ptr", "load");
  if (!op.result.value_id()) {
    fail_verify("LirLoadOp.result",
                "direct-global load result requires LirValueId authority");
  }
}

void verify_native_load_result_authority(const LirLoadOp& op) {
  if (op.requires_native_result_authority && !op.result.value_id()) {
    fail_verify("LirLoadOp.result",
                "standalone native load requires LirValueId result authority");
  }
}

void verify_authoritative_gep(const LirModule& mod, const LirGepOp& op) {
  const bool authoritative =
      op.result.has_authority() || op.ptr.has_authority() ||
      std::any_of(op.indices.begin(), op.indices.end(),
                  [](const LirGepIndex& index) {
                    return index.is_authoritative();
                  });
  if (!authoritative) return;

  if (!op.result.value_id()) {
    fail_verify("LirGepOp.result",
                "authoritative GEP requires LirValueId result authority");
  }
  if (op.ptr.kind() == LirOperandKind::Global && op.ptr.link_name_id()) {
    verify_global_pointer_owner(mod, op.ptr, "LirGepOp.ptr", "GEP");
  } else if (op.ptr.kind() == LirOperandKind::DirectConstant &&
             op.ptr.value_id()) {
    // The enclosing-function check resolves this ID only through the
    // direct-label-address table; its display spelling is not authority.
  } else if (op.ptr.kind() != LirOperandKind::SsaValue || !op.ptr.value_id()) {
    fail_verify("LirGepOp.ptr",
                "authoritative GEP requires global, direct-constant, or SSA base authority");
  }
  if (op.indices.empty()) {
    fail_verify("LirGepOp.indices",
                "authoritative GEP requires at least one index");
  }

  for (const LirGepIndex& index : op.indices) {
    if (!index.is_authoritative()) {
      fail_verify("LirGepOp.indices",
                  "authoritative GEP cannot mix raw compatibility indices");
    }
    if (index.type_ref().kind() != LirTypeKind::Integer) {
      require_module_type_ref(mod, index.type_ref(),
                              "LirGepOp.indices.type");
      fail_verify("LirGepOp.indices.type",
                  "authoritative GEP index type must be integer");
    }
    (void)render_integer_type_ref(index.type_ref(),
                                  "LirGepOp.indices.type");

    require_operand_kind(index.value(), "LirGepOp.indices.value",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::Immediate});
    if (const LirIntegerImmediate* immediate =
            index.value().integer_immediate()) {
      const std::optional<unsigned> bit_width =
          index.type_ref().integer_bit_width();
      if (!bit_width ||
          !integer_immediate_representable(immediate->value, *bit_width)) {
        fail_verify("LirGepOp.indices.value",
                    "integer immediate is not representable by the GEP index type");
      }
      continue;
    }
    if (!index.value().value_id()) {
      fail_verify("LirGepOp.indices.value",
                  "authoritative GEP index requires integer or SSA authority");
    }
  }
}

void verify_native_gep_result_authority(const LirGepOp& op) {
  if (op.requires_native_result_authority && !op.result.value_id()) {
    fail_verify("LirGepOp.result",
                "standalone native GEP requires LirValueId result authority");
  }
}

void verify_native_local_gep_authority(const LirGepOp& op) {
  const auto* authority = op.local_object_authority
                              ? &*op.local_object_authority
                              : nullptr;
  const bool selected_shape =
      authority && op.result.value_id() && op.result.value_id()->valid() &&
      op.ptr.kind() == LirOperandKind::SsaValue && op.ptr.value_id() &&
      *op.ptr.value_id() == authority->pointer_definition &&
      authority->indexed_element_type &&
      op.element_type == *authority->indexed_element_type && op.indices.size() == 1 &&
      op.indices.front().is_authoritative() &&
      op.indices.front().type_ref() == LirTypeRef::integer(64) &&
      op.indices.front().value().integer_immediate();
  if (!op.requires_native_local_gep_authority) {
    if (selected_shape) {
      fail_verify("LirGepOp.requires_native_local_gep_authority",
                  "selected local-array GEP shape requires native local admission");
    }
    return;
  }
  if (!op.requires_native_result_authority || !authority ||
      !selected_shape) {
    fail_verify("LirGepOp.local_object_authority",
                "selected local-array GEP requires native result, base, type, and immediate index authority");
  }
}

void verify_native_stack_save_authority(const LirStackSaveOp& op) {
  const auto* authority = op.local_object_authority
                              ? &*op.local_object_authority
                              : nullptr;
  if (!op.requires_native_stack_save_authority) {
    if (authority) {
      fail_verify("LirStackSaveOp.requires_native_stack_save_authority",
                  "stack save local authority requires native stack-save admission");
    }
    return;
  }
  if (!authority || op.result.kind() != LirOperandKind::SsaValue ||
      !op.result.value_id() || !op.result.value_id()->valid() ||
      *op.result.value_id() != authority->pointer_definition ||
      authority->pointer_type.kind() != LirTypeKind::Pointer ||
      authority->pointee_type.kind() != LirTypeKind::Pointer || !authority->live) {
    fail_verify("LirStackSaveOp.local_object_authority",
                "selected VLA stack save requires native result and live pointer/object/type authority");
  }
}

void verify_native_stack_restore_authority(const LirStackRestoreOp& op) {
  const auto* authority = op.local_object_authority
                              ? &*op.local_object_authority
                              : nullptr;
  if (!op.requires_native_stack_restore_authority) {
    if (op.lifetime_transition) {
      fail_verify("LirStackRestoreOp.lifetime_transition",
                  "unselected stack restore must not carry a native checkpoint transition");
    }
    return;
  }
  if (!authority || !op.lifetime_transition ||
      op.saved_ptr.kind() != LirOperandKind::SsaValue || !op.saved_ptr.value_id() ||
      !op.saved_ptr.value_id()->valid() ||
      *op.saved_ptr.value_id() != authority->pointer_definition ||
      authority->pointer_type.kind() != LirTypeKind::Pointer ||
      authority->pointee_type.kind() != LirTypeKind::Pointer || !authority->live) {
    fail_verify("LirStackRestoreOp.local_object_authority",
                "selected VLA stack restore requires native saved-pointer and live pointer/object/type authority");
  }
  const auto& transition = *op.lifetime_transition;
  if (transition.kind != LirStackRestoreOp::LirStackRestoreLifetimeTransition::Kind::
                             RestoreSavedVlaStackCheckpoint ||
      !transition.saved_pointer_definition.valid() ||
      transition.saved_pointer_definition != *op.saved_ptr.value_id() ||
      transition.saved_pointer_definition != authority->pointer_definition) {
    fail_verify("LirStackRestoreOp.lifetime_transition",
                "selected VLA stack restore transition must consume its saved checkpoint definition");
  }
}

void verify_native_call_result_authority(const LirCallOp& op) {
  if (op.requires_native_result_authority && !op.result.value_id()) {
    fail_verify("LirCallOp.result",
                "standalone native call requires LirValueId result authority");
  }
}

bool requires_native_result_authority(const LirInst& inst) {
  if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
    return cast->requires_native_result_authority;
  }
  if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
    return load->requires_native_result_authority;
  }
  if (const auto* gep = std::get_if<LirGepOp>(&inst)) {
    return gep->requires_native_result_authority;
  }
  if (const auto* stack_save = std::get_if<LirStackSaveOp>(&inst)) {
    return stack_save->requires_native_stack_save_authority;
  }
  if (const auto* call = std::get_if<LirCallOp>(&inst)) {
    return call->requires_native_result_authority;
  }
  if (const auto* extract = std::get_if<LirExtractValueOp>(&inst)) {
    return extract->requires_native_result_authority;
  }
  if (const auto* insert = std::get_if<LirInsertValueOp>(&inst)) {
    return insert->requires_native_result_authority;
  }
  return false;
}

void verify_extract_value_authority(const LirModule& mod,
                                    const LirExtractValueOp& op) {
  if (!op.requires_native_result_authority) return;
  if (op.result.kind() != LirOperandKind::SsaValue || !op.result.value_id() ||
      !op.result.value_id()->valid()) {
    fail_verify("LirExtractValueOp.result",
                "native extractvalue result requires valid LirValueId authority");
  }
  if (op.agg.kind() != LirOperandKind::SsaValue || !op.agg.value_id() ||
      !op.agg.value_id()->valid()) {
    fail_verify("LirExtractValueOp.agg",
                "native extractvalue aggregate requires valid SSA LirValueId authority");
  }
  const std::vector<LirTypeRef>& fields = require_native_anonymous_struct_fields(
      mod, op.agg_type, "LirExtractValueOp.agg_type");
  if (op.index < 0 || static_cast<size_t>(op.index) >= fields.size()) {
    fail_verify("LirExtractValueOp.index",
                "native extractvalue field index must select an aggregate field");
  }
  if (!op.result_element_type ||
      !same_native_type_fact(*op.result_element_type,
                             fields[static_cast<size_t>(op.index)])) {
    fail_verify("LirExtractValueOp.result_element_type",
                "native extractvalue result type must match its selected aggregate field");
  }
}

void verify_insert_value_authority(const LirInsertValueOp& op) {
  if (!op.requires_native_result_authority) return;
  if (op.result.kind() != LirOperandKind::SsaValue || !op.result.value_id() ||
      !op.result.value_id()->valid() || !op.aggregate_result_type ||
      !same_native_type_fact(*op.aggregate_result_type, op.agg_type)) {
    fail_verify("LirInsertValueOp.result",
                "native insertvalue aggregate producer requires matching result and type authority");
  }
  const std::vector<LirTypeRef>* fields = op.aggregate_result_type->anonymous_struct_field_types();
  if (!fields || fields->empty()) {
    fail_verify("LirInsertValueOp.aggregate_result_type",
                "native insertvalue aggregate producer requires ordered native field types");
  }
  if (op.index < 0 || static_cast<size_t>(op.index) >= fields->size()) {
    fail_verify("LirInsertValueOp.index",
                "native insertvalue field index must select an aggregate field");
  }
  if (!same_native_type_fact(op.elem_type, (*fields)[static_cast<size_t>(op.index)])) {
    fail_verify("LirInsertValueOp.elem_type",
                "native insertvalue element type must match its selected aggregate field");
  }
}

void verify_inst(const LirModule& mod, const LirInst& inst,
                 const LirFunction* owner_function = nullptr) {
  if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    verify_pointer_operand(op->dst, "LirMemcpyOp.dst");
    verify_pointer_operand(op->src, "LirMemcpyOp.src");
    verify_value_operand(op->size, "LirMemcpyOp.size");
    return;
  }
  if (const auto* op = std::get_if<LirMemsetOp>(&inst)) {
    verify_pointer_operand(op->dst, "LirMemsetOp.dst");
    verify_value_operand(op->byte_val, "LirMemsetOp.byte_val");
    verify_value_operand(op->size, "LirMemsetOp.size");
    return;
  }
  if (const auto* op = std::get_if<LirVaStartOp>(&inst)) {
    verify_pointer_operand(op->ap_ptr, "LirVaStartOp.ap_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirVaEndOp>(&inst)) {
    verify_pointer_operand(op->ap_ptr, "LirVaEndOp.ap_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
    verify_pointer_operand(op->dst_ptr, "LirVaCopyOp.dst_ptr");
    verify_pointer_operand(op->src_ptr, "LirVaCopyOp.src_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) {
    verify_result_operand(op->result, "LirStackSaveOp.result");
    verify_native_stack_save_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    verify_pointer_operand(op->saved_ptr, "LirStackRestoreOp.saved_ptr");
    verify_native_stack_restore_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    verify_result_operand(op->result, "LirAbsOp.result");
    verify_value_operand(op->arg, "LirAbsOp.arg");
    (void)render_integer_type_ref(op->int_type, "LirAbsOp.int_type");
    verify_abs_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    require_operand_kind(op->addr, "LirIndirectBrOp.addr",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::DirectConstant,
                          LirOperandKind::Global});
    if (op->targets.empty()) {
      fail_verify("LirIndirectBrOp.targets", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    verify_result_operand(op->result, "LirExtractValueOp.result");
    if (op->requires_native_result_authority) {
      (void)require_native_anonymous_struct_fields(
          mod, op->agg_type, "LirExtractValueOp.agg_type");
    } else {
      require_module_type_ref(mod, op->agg_type, "LirExtractValueOp.agg_type");
    }
    verify_value_operand(op->agg, "LirExtractValueOp.agg");
    if (op->result_element_type) {
      require_module_type_ref(mod, *op->result_element_type,
                              "LirExtractValueOp.result_element_type");
    }
    verify_extract_value_authority(mod, *op);
    return;
  }
  if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertValueOp.result");
    if (op->requires_native_result_authority) {
      if (op->aggregate_result_type) {
        const std::vector<LirTypeRef>& fields =
            require_native_anonymous_struct_fields(
                mod, *op->aggregate_result_type,
                "LirInsertValueOp.aggregate_result_type");
        (void)fields;
      }
    } else {
      require_module_type_ref(mod, op->agg_type, "LirInsertValueOp.agg_type");
    }
    verify_value_operand(op->agg, "LirInsertValueOp.agg");
    if (!op->requires_native_result_authority) {
      require_module_type_ref(mod, op->elem_type, "LirInsertValueOp.elem_type");
    }
    verify_value_operand(op->elem, "LirInsertValueOp.elem");
    verify_insert_value_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    verify_result_operand(op->result, "LirLoadOp.result");
    if (op->requires_native_result_authority &&
        op->type_str.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirLoadOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirLoadOp.type_str", true);
    }
    verify_pointer_operand(op->ptr, "LirLoadOp.ptr");
    verify_global_load_authority(mod, *op);
    verify_native_load_result_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    if (op->requires_native_store_authority &&
        op->type_str.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirStoreOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirStoreOp.type_str", true);
    }
    verify_store_value_operand(op->val, "LirStoreOp.val");
    verify_pointer_operand(op->ptr, "LirStoreOp.ptr");
    verify_global_store_authority(mod, *op);
    return;
  }
  if (const auto* op = std::get_if<LirCastOp>(&inst)) {
    verify_result_operand(op->result, "LirCastOp.result");
    require_module_cast_endpoint_type_ref(mod, op->from_type,
                                          "LirCastOp.from_type");
    verify_value_operand(op->operand, "LirCastOp.operand");
    require_module_cast_endpoint_type_ref(mod, op->to_type,
                                          "LirCastOp.to_type");
    verify_cast_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    verify_result_operand(op->result, "LirGepOp.result");
    require_module_type_ref(mod, op->element_type, "LirGepOp.element_type");
    if (op->ptr.kind() != LirOperandKind::DirectConstant) {
      verify_pointer_operand(op->ptr, "LirGepOp.ptr");
    }
    verify_authoritative_gep(mod, *op);
    verify_native_gep_result_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    const bool structured_authority_complete =
        has_complete_fixed_call_type_authority(mod, *op) ||
        has_complete_direct_void_integer_immediate_authority(mod, *op) ||
        has_complete_direct_void_integer_ssa_authority(mod, *op) ||
        has_complete_integer_boolean_flag_call_authority(mod, *op) ||
        has_complete_integer_count_call_authority(mod, *op);
    require_operand_kind(op->result, "LirCallOp.result",
                         {LirOperandKind::SsaValue}, true);
    verify_call_return_type_ref_mirror(mod, op->return_type);
    verify_pointer_operand(op->callee, "LirCallOp.callee");
    verify_call_callee_signature(mod, *op, structured_authority_complete);
    for (const LirTypeRef& arg_type_ref : op->arg_type_refs) {
      verify_call_aggregate_type_ref_store_entry(
          mod, arg_type_ref, "LirCallOp.arg_type_refs");
    }
    if (!op->arg_type_refs.empty() && !structured_authority_complete) {
      auto parsed = parse_lir_typed_call_or_infer_params(*op);
      if (!parsed.has_value()) {
        const auto param_types = parse_lir_call_param_types(op->callee_type_suffix);
        const auto args = parse_lir_typed_call_args(op->args_str);
        if (param_types.has_value() && args.has_value() &&
            param_types->size() == args->size()) {
          bool mirrorable = true;
          for (std::size_t index = 0; index < args->size(); ++index) {
            if (!lir_call_param_type_accepts_arg_type((*param_types)[index],
                                                      (*args)[index].type)) {
              mirrorable = false;
              break;
            }
          }
          if (mirrorable) {
            parsed = ParsedLirTypedCallView{*param_types, *args};
          }
        }
      }
      if (!parsed.has_value()) {
        fail_verify("LirCallOp.arg_type_refs",
                    "cannot validate argument mirrors against malformed call arguments");
      }
      if (op->arg_type_refs.size() != parsed->args.size()) {
        std::ostringstream detail;
        detail << "has " << op->arg_type_refs.size()
               << " argument mirrors but call text has " << parsed->args.size()
               << " arguments";
        fail_verify("LirCallOp.arg_type_refs", detail.str());
      }
      for (size_t index = 0; index < op->arg_type_refs.size(); ++index) {
        verify_call_arg_type_ref_mirror(
            mod, op->arg_type_refs[index], parsed->args[index].type, index);
      }
    }
    verify_direct_void_fixed_integer_immediate_call(mod, *op);
    verify_direct_void_fixed_integer_ssa_call(mod, *op);
    verify_direct_zero_arg_scalar_floating_result_call(mod, owner_function, *op);
    verify_direct_one_double_arg_scalar_floating_result_call(mod, owner_function, *op);
    verify_integer_boolean_flag_call_authority(mod, *op);
    verify_integer_count_call_authority(mod, *op);
    verify_native_call_result_authority(*op);
    if (op->result.empty() && op->return_type != "void") {
      fail_verify("LirCallOp.result",
                  "must hold an SSA result for non-void calls");
    }
    if ((!op->result.empty() || op->result.has_authority()) &&
        op->return_type == "void") {
      fail_verify("LirCallOp.return_type",
                  "void calls must not carry a result operand");
    }
    const bool authoritative_direct_integer_result =
        op->return_type.kind() == LirTypeKind::Integer &&
        op->direct_callee_link_name_id != kInvalidLinkName &&
        op->callee_signature.has_value();
    if (authoritative_direct_integer_result && !op->result.value_id()) {
      fail_verify(
          "LirCallOp.result",
          "structured direct integer call requires LirValueId result authority");
    }
    return;
  }
  if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    verify_result_operand(op->result, "LirBinOp.result");
    (void)render_binary_opcode(op->opcode, "LirBinOp.opcode");
    if (op->compact_scalar_type &&
        op->compact_scalar_type->type.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirBinOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirBinOp.type_str", true);
    }
    verify_value_operand(op->lhs, "LirBinOp.lhs");
    if (op->rhs.empty()) {
      if (op->opcode.typed() != LirBinaryOpcode::FNeg) {
        fail_verify("LirBinOp.rhs",
                    "must not be empty for non-unary binary instructions");
      }
    } else {
      if (op->opcode.typed() == LirBinaryOpcode::FNeg) {
        fail_verify("LirBinOp.rhs", "must be empty for unary fneg instructions");
      }
      verify_value_operand(op->rhs, "LirBinOp.rhs");
    }
    verify_bin_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    verify_result_operand(op->result, "LirCmpOp.result");
    (void)render_cmp_predicate(op->predicate, "LirCmpOp.predicate");
    if (op->is_float) {
      (void)render_floating_type_ref(op->type_str, "LirCmpOp.type_str");
    } else if (op->type_str.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirCmpOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirCmpOp.type_str");
    }
    verify_value_operand(op->lhs, "LirCmpOp.lhs");
    verify_value_operand(op->rhs, "LirCmpOp.rhs");
    verify_cmp_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    verify_result_operand(op->result, "LirPhiOp.result");
    const LirPhiBoundaryValueType& boundary_type =
        phi_boundary_value_type(*op, "LirPhiOp.boundary_value_type");
    require_module_phi_boundary_value_type_ref(
        mod, boundary_type, "LirPhiOp.boundary_value_type");
    if (op->incoming.empty()) {
      fail_verify("LirPhiOp.incoming", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    verify_result_operand(op->result, "LirSelectOp.result");
    (void)render_integer_type_ref(op->type_str, "LirSelectOp.type_str");
    verify_value_operand(op->cond, "LirSelectOp.cond");
    verify_value_operand(op->true_val, "LirSelectOp.true_val");
    verify_value_operand(op->false_val, "LirSelectOp.false_val");
    verify_select_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertElementOp.result");
    if (!op->requires_native_vector_authority) {
      require_module_type_ref(mod, op->vec_type, "LirInsertElementOp.vec_type");
    }
    verify_value_operand(op->vec, "LirInsertElementOp.vec");
    if (!op->requires_native_vector_authority) {
      require_module_type_ref(mod, op->elem_type, "LirInsertElementOp.elem_type");
    }
    verify_value_operand(op->elem, "LirInsertElementOp.elem");
    verify_value_operand(op->index, "LirInsertElementOp.index");
    return;
  }
  if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
    verify_result_operand(op->result, "LirExtractElementOp.result");
    require_module_type_ref(mod, op->vec_type, "LirExtractElementOp.vec_type");
    verify_value_operand(op->vec, "LirExtractElementOp.vec");
    require_module_type_ref(mod, op->index_type, "LirExtractElementOp.index_type");
    verify_value_operand(op->index, "LirExtractElementOp.index");
    return;
  }
  if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
    verify_result_operand(op->result, "LirShuffleVectorOp.result");
    if (!op->requires_native_vector_authority) {
      require_module_type_ref(mod, op->vec_type, "LirShuffleVectorOp.vec_type");
    }
    verify_value_operand(op->vec1, "LirShuffleVectorOp.vec1");
    verify_value_operand(op->vec2, "LirShuffleVectorOp.vec2");
    if (!op->requires_native_vector_authority) {
      require_module_type_ref(mod, op->mask_type, "LirShuffleVectorOp.mask_type");
    }
    verify_value_operand(op->mask, "LirShuffleVectorOp.mask");
    return;
  }
  if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    verify_result_operand(op->result, "LirVaArgOp.result");
    verify_pointer_operand(op->ap_ptr, "LirVaArgOp.ap_ptr");
    if (op->requires_native_memory_va_authority &&
        op->result_authority.has_value() &&
        op->result_type_authority.has_value() &&
        op->type_str.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirVaArgOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirVaArgOp.type_str");
    }
    return;
  }
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    verify_result_operand(op->result, "LirAllocaOp.result");
    if (op->local_object_authority &&
        op->type_str.kind() == LirTypeKind::Integer) {
      (void)render_integer_type_ref(op->type_str, "LirAllocaOp.type_str");
    } else {
      require_module_type_ref(mod, op->type_str, "LirAllocaOp.type_str");
    }
    verify_optional_count_operand(op->count, "LirAllocaOp.count");
    return;
  }
  if (const auto* op = std::get_if<LirInlineAsmOp>(&inst)) {
    require_operand_kind(op->result, "LirInlineAsmOp.result",
                         {LirOperandKind::SsaValue}, true);
    if (op->result.has_authority()) {
      fail_verify("LirInlineAsmOp.result",
                  "compatibility result must remain presentation-only");
    }
    require_module_type_ref(mod, op->ret_type, "LirInlineAsmOp.ret_type", true);
    if (op->result.empty() && op->ret_type != "void") {
      fail_verify("LirInlineAsmOp.result",
                  "must hold an SSA result for non-void inline asm");
    }
    if (!op->result.empty() && op->ret_type == "void") {
      fail_verify("LirInlineAsmOp.ret_type",
                  "void inline asm must not carry a result operand");
    }

    const bool native_scalar_integer_output_only =
        !op->result.empty() && op->ret_type.kind() == LirTypeKind::Integer &&
        op->ordinary_inputs.empty() && op->ordinary_results.size() == 1 &&
        op->ordinary_results[0].role == LirInlineAsmValueRole::Output &&
        op->ordinary_results[0].type == op->ret_type &&
        op->ordinary_results[0].constraint_index == 0;
    const std::size_t semantic_constraint_count =
        count_inline_asm_constraints(op->original_constraint_text);
    if ((!op->ordinary_inputs.empty() || !op->ordinary_results.empty()) &&
        semantic_constraint_count == 0 &&
        !native_scalar_integer_output_only) {
      fail_verify("LirInlineAsmOp.original_constraint_text",
                  "structured values require original semantic constraints");
    }

    const auto verify_bindings =
        [&](const std::vector<LirInlineAsmValueBinding>& bindings,
            std::string_view field, LirInlineAsmValueRole ordinary_role,
            bool constraint_count_is_opaque = false) {
          std::optional<std::size_t> previous_constraint;
          for (std::size_t index = 0; index < bindings.size(); ++index) {
            const auto& binding = bindings[index];
            const std::string item_field =
                std::string(field) + "[" + std::to_string(index) + "]";
            if (ordinary_role == LirInlineAsmValueRole::Output) {
              verify_result_operand(binding.value, item_field + ".value");
            } else {
              verify_value_operand(binding.value, item_field + ".value");
            }
            require_module_type_ref(mod, binding.type, item_field + ".type");
            if (binding.role != ordinary_role &&
                binding.role != LirInlineAsmValueRole::ReadWrite) {
              fail_verify(item_field + ".role",
                          "does not match the binding list role");
            }
            if (!constraint_count_is_opaque &&
                binding.constraint_index >= semantic_constraint_count) {
              fail_verify(item_field + ".constraint_index",
                          "is outside the original constraint list");
            }
            if (previous_constraint &&
                binding.constraint_index <= *previous_constraint) {
              fail_verify(item_field + ".constraint_index",
                          "bindings must follow strict constraint order");
            }
            previous_constraint = binding.constraint_index;
          }
        };
    verify_bindings(op->ordinary_inputs, "LirInlineAsmOp.ordinary_inputs",
                    LirInlineAsmValueRole::Input);
    verify_bindings(op->ordinary_results, "LirInlineAsmOp.ordinary_results",
                    LirInlineAsmValueRole::Output,
                    native_scalar_integer_output_only);

    const bool scalar_integer_result =
        !op->result.empty() && op->ret_type.kind() == LirTypeKind::Integer;
    if (scalar_integer_result &&
        (op->ordinary_results.size() != 1 ||
         op->ordinary_results[0].type != op->ret_type ||
         op->ordinary_results[0].constraint_index != 0)) {
      fail_verify("LirInlineAsmOp.ordinary_results",
                  "scalar integer result requires one exact type/index binding");
    }
    if (scalar_integer_result &&
        op->ordinary_results[0].role == LirInlineAsmValueRole::Output) {
      const LirInlineAsmValueBinding& output = op->ordinary_results[0];
      if (!output.value.value_id() || !output.value.value_id()->valid()) {
        fail_verify("LirInlineAsmOp.ordinary_results[0]",
                    "scalar integer output-only binding requires exact result, type, role, and output-index authority");
      }
    }

    for (std::size_t result_index = 0;
         result_index < op->ordinary_results.size(); ++result_index) {
      const auto& result = op->ordinary_results[result_index];
      for (std::size_t earlier = 0; earlier < result_index; ++earlier) {
        if (op->ordinary_results[earlier].value == result.value) {
          fail_verify("LirInlineAsmOp.ordinary_results",
                      "result identities must be unique");
        }
      }

      const LirInlineAsmValueBinding* matching_input = nullptr;
      for (const auto& input : op->ordinary_inputs) {
        if (input.value == result.value) {
          fail_verify("LirInlineAsmOp.ordinary_results",
                      "a produced result must be distinct from every input use");
        }
        if (input.constraint_index == result.constraint_index) {
          matching_input = &input;
        }
      }

      if (result.role == LirInlineAsmValueRole::ReadWrite) {
        if (!matching_input ||
            matching_input->role != LirInlineAsmValueRole::ReadWrite) {
          fail_verify("LirInlineAsmOp.ordinary_results",
                      "a read/write result requires a matching read/write input");
        }
        if (!same_inline_asm_type(matching_input->type, result.type)) {
          fail_verify("LirInlineAsmOp.ordinary_results",
                      "a read/write input and result must have the same type");
        }
      } else if (matching_input) {
        fail_verify("LirInlineAsmOp.ordinary_results",
                    "only read/write bindings may share a constraint position");
      }
    }
    for (const auto& input : op->ordinary_inputs) {
      if (input.role != LirInlineAsmValueRole::ReadWrite) continue;
      const auto match = std::find_if(
          op->ordinary_results.begin(), op->ordinary_results.end(),
          [&](const LirInlineAsmValueBinding& result) {
            return result.constraint_index == input.constraint_index &&
                   result.role == LirInlineAsmValueRole::ReadWrite;
          });
      if (match == op->ordinary_results.end()) {
        fail_verify("LirInlineAsmOp.ordinary_inputs",
                    "a read/write input requires a matching read/write result");
      }
    }
    if (op->insn_r) {
      const std::size_t operand_count =
          count_inline_asm_constraints(op->constraints);
      for (const std::size_t operand_index : op->insn_r->operand_indices) {
        if (operand_index >= operand_count) {
          fail_verify("LirInlineAsmOp.insn_r",
                      ".insn r operand index is outside the constraint list");
        }
      }
    }
    return;
  }
}

template <typename T, typename = void>
struct has_lir_operand_result : std::false_type {};

template <typename T>
struct has_lir_operand_result<
    T, std::void_t<decltype(std::declval<const T&>().result)>>
    : std::is_same<std::decay_t<decltype(std::declval<const T&>().result)>,
                   LirOperand> {};

const LirOperand* modeled_result_operand(const LirInst& inst) {
  return std::visit(
      [](const auto& op) -> const LirOperand* {
        using Op = std::decay_t<decltype(op)>;
        if constexpr (has_lir_operand_result<Op>::value) {
          return &op.result;
        }
        return nullptr;
      },
      inst);
}

const LirTypeRef* modeled_scalar_result_type(const LirInst& inst) {
  if (const auto* op = std::get_if<LirLoadOp>(&inst)) return &op->type_str;
  if (const auto* op = std::get_if<LirCastOp>(&inst)) return &op->to_type;
  if (const auto* op = std::get_if<LirCallOp>(&inst)) return &op->return_type;
  if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    if (const LirCompactScalarType* compact_scalar =
            compact_scalar_binop_type(*op, "LirBinOp.compact_scalar_type")) {
      return &compact_scalar->type;
    }
    return &op->type_str;
  }
  if (std::get_if<LirCmpOp>(&inst)) {
    static const LirTypeRef kBooleanType = LirTypeRef::integer(1);
    return &kBooleanType;
  }
  if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    return &phi_boundary_value_type(*op, "LirPhiOp.boundary_value_type").type;
  }
  if (const auto* op = std::get_if<LirSelectOp>(&inst)) return &op->type_str;
  if (const auto* op = std::get_if<LirVaArgOp>(&inst)) return &op->type_str;
  return nullptr;
}

bool modeled_pointer_result(const LirInst& inst) {
  if (const LirTypeRef* type = modeled_scalar_result_type(inst)) {
    return type->kind() == LirTypeKind::Pointer;
  }
  return std::holds_alternative<LirStackSaveOp>(inst) ||
         std::holds_alternative<LirGepOp>(inst) ||
         std::holds_alternative<LirAllocaOp>(inst);
}

template <typename Visitor>
void visit_modeled_value_uses(const LirInst& inst, Visitor&& visit) {
  if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    visit(op->dst); visit(op->src); visit(op->size); return;
  }
  if (const auto* op = std::get_if<LirMemsetOp>(&inst)) {
    visit(op->dst); visit(op->byte_val); visit(op->size); return;
  }
  if (const auto* op = std::get_if<LirVaStartOp>(&inst)) {
    visit(op->ap_ptr); return;
  }
  if (const auto* op = std::get_if<LirVaEndOp>(&inst)) {
    visit(op->ap_ptr); return;
  }
  if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
    visit(op->dst_ptr); visit(op->src_ptr); return;
  }
  if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    visit(op->saved_ptr); return;
  }
  if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    visit(op->arg); return;
  }
  if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    visit(op->addr); return;
  }
  if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    visit(op->agg); return;
  }
  if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    visit(op->agg); visit(op->elem); return;
  }
  if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    visit(op->ptr); return;
  }
  if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    visit(op->val); visit(op->ptr); return;
  }
  if (const auto* op = std::get_if<LirCastOp>(&inst)) {
    visit(op->operand); return;
  }
  if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    visit(op->ptr);
    for (const LirGepIndex& index : op->indices) {
      if (index.is_authoritative()) visit(index.value());
    }
    return;
  }
  if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    visit(op->callee);
    for (const auto& arg : op->structured_args) visit(arg.operand);
    return;
  }
  if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    visit(op->lhs);
    if (!op->rhs.empty()) visit(op->rhs);
    return;
  }
  if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    visit(op->lhs); visit(op->rhs); return;
  }
  if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    visit(op->cond); visit(op->true_val); visit(op->false_val); return;
  }
  if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    visit(op->vec); visit(op->elem); visit(op->index); return;
  }
  if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
    visit(op->vec); visit(op->index); return;
  }
  if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
    visit(op->vec1); visit(op->vec2); visit(op->mask); return;
  }
  if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    visit(op->ap_ptr); return;
  }
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    if (!op->count.empty()) visit(op->count);
    return;
  }
  if (const auto* op = std::get_if<LirInlineAsmOp>(&inst)) {
    for (const auto& input : op->ordinary_inputs) visit(input.value);
  }
}

void verify_selected_memcpy_pointer_authority(const LirModule& mod,
                                              const LirFunction& function,
                                              std::unordered_set<uint32_t>& definitions,
                                              std::unordered_map<uint32_t,
                                                                 const LirInst*>& definition_insts) {
  if (!function.selected_memcpy_pointer_authority.has_value()) return;

  if (function.link_name_id == kInvalidLinkName ||
      mod.link_names.spelling(function.link_name_id).empty()) {
    fail_verify("LirFunction.selected_memcpy_pointer_authority",
                "selected pointer authority requires a current-function LinkNameId");
  }
  const std::size_t owner_count = static_cast<std::size_t>(std::count_if(
      mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
        return candidate.link_name_id == function.link_name_id;
      }));
  if (owner_count != 1) {
    fail_verify("LirFunction.selected_memcpy_pointer_authority",
                "selected pointer authority requires one current-function owner");
  }

  const auto& authority = *function.selected_memcpy_pointer_authority;
  const auto verify_definition = [&](const LirCurrentFunctionPointerDefinition& definition,
                                     LirSelectedMemcpyPointerRole expected_role,
                                     std::string_view field) {
    if (definition.role != expected_role) {
      fail_verify(field, "selected pointer definition has the wrong role");
    }
    if (!definition.value.valid()) {
      fail_verify(field, "selected pointer definition has an invalid LirValueId");
    }
    if (definition.pointer_type.kind() != LirTypeKind::Pointer) {
      fail_verify(field, "selected pointer definition requires pointer type authority");
    }
    if (!definition.object.valid()) {
      fail_verify(field, "selected pointer definition has an invalid LirObjectId");
    }
    if (definition.object_owner != function.link_name_id) {
      fail_verify(field, "selected local object is not owned by this LirFunction");
    }
    if (!definition.live_at_selected_site) {
      fail_verify(field, "selected pointer definition is not live at the selected site");
    }
    if (!definitions.insert(definition.value.value).second) {
      fail_verify(field, "duplicate current-function LirValueId definition " +
                             std::to_string(definition.value.value));
    }
    // These carriers are definitions in their own right; no instruction
    // pointer exists for a byval parameter definition.
    definition_insts.emplace(definition.value.value, nullptr);
  };

  verify_definition(authority.byval_parameter,
                    LirSelectedMemcpyPointerRole::ByvalParameter,
                    "LirFunction.selected_memcpy_pointer_authority.byval_parameter");
  verify_definition(authority.destination_alloca,
                    LirSelectedMemcpyPointerRole::DestinationAlloca,
                    "LirFunction.selected_memcpy_pointer_authority.destination_alloca");
  if (authority.byval_parameter.value == authority.destination_alloca.value ||
      authority.byval_parameter.object == authority.destination_alloca.object) {
    fail_verify("LirFunction.selected_memcpy_pointer_authority",
                "selected pointer definitions require distinct values and local objects");
  }
}

bool plain_fixed_scalar_parameter(const TypeSpec& type);
bool direct_scalar_parameter_type(const TypeSpec& type);
bool same_plain_fixed_scalar_type(const TypeSpec& lhs, const TypeSpec& rhs);
bool exact_plain_scalar_mirror(const LirModule& mod, const TypeSpec& type,
                               const LirTypeRef& mirror);

void verify_native_body_parameter_definitions(
    const LirModule& mod, const LirFunction& function,
    std::unordered_set<uint32_t>& definitions,
    std::unordered_map<uint32_t, const LirInst*>& definition_insts) {
  if (function.native_body_parameter_definitions.empty()) return;
  if (function.is_declaration || function.link_name_id == kInvalidLinkName ||
      mod.link_names.spelling(function.link_name_id).empty()) {
    fail_verify("LirFunction.native_body_parameter_definitions",
                "body parameter authority requires a defined current-function owner");
  }
  std::unordered_set<uint32_t> parameter_indices;
  for (const auto& definition : function.native_body_parameter_definitions) {
    constexpr std::string_view field =
        "LirFunction.native_body_parameter_definitions";
    const bool in_range = definition.parameter_index < function.params.size() &&
                          definition.parameter_index < function.signature_params.size() &&
                          definition.parameter_index < function.signature_param_type_refs.size();
    const bool direct_pointer = in_range &&
                                definition.type.kind() == LirTypeKind::Pointer &&
                                definition.abi == LirNativeBodyParameterAbi::DirectPointer;
    const bool direct_scalar = in_range &&
                               definition.abi == LirNativeBodyParameterAbi::DirectScalar &&
                               direct_scalar_parameter_type(
                                   function.params[definition.parameter_index].second) &&
                               !function.signature_params[definition.parameter_index].is_byval &&
                               direct_scalar_parameter_type(
                                   function.signature_params[definition.parameter_index].type) &&
                               same_plain_fixed_scalar_type(
                                   function.params[definition.parameter_index].second,
                                   function.signature_params[definition.parameter_index].type) &&
                               exact_plain_scalar_mirror(
                                   mod, function.signature_params[definition.parameter_index].type,
                                   definition.type);
    if (!definition.value.valid() || definition.owner != function.link_name_id || !in_range ||
        function.signature_param_type_refs[definition.parameter_index] != definition.type) {
      fail_verify(field,
                  "requires a native direct-pointer or direct-scalar current-function parameter identity and type");
    }
    if (!direct_pointer && !direct_scalar) {
      if (definition.abi == LirNativeBodyParameterAbi::DirectScalar) {
        const auto& logical = function.params[definition.parameter_index].second;
        const auto& signature = function.signature_params[definition.parameter_index];
        if (!direct_scalar_parameter_type(logical))
          fail_verify(field, "direct-scalar parameter has a non-plain logical type");
        if (signature.is_byval)
          fail_verify(field, "direct-scalar parameter must not be passed byval");
        if (!direct_scalar_parameter_type(signature.type))
          fail_verify(field, "direct-scalar parameter has a non-plain signature type");
        if (!same_plain_fixed_scalar_type(logical, signature.type))
          fail_verify(field, "direct-scalar parameter logical and signature types disagree");
        fail_verify(field, "direct-scalar parameter type does not match its typed signature mirror");
      }
      fail_verify(field,
                  "requires a native direct-pointer or direct-scalar current-function parameter identity and type");
    }
    if (!parameter_indices.insert(definition.parameter_index).second) {
      fail_verify(field, "must not duplicate a native body parameter index");
    }
    if (!definitions.insert(definition.value.value).second) {
      fail_verify(field, "duplicates a current-function LirValueId definition");
    }
    definition_insts.emplace(definition.value.value, nullptr);
  }
}

void verify_local_object_authority(const LirModule& mod, const LirFunction& function,
                                   const LirCurrentFunctionLocalObjectPointer& authority,
                                   std::string_view field) {
  if (!authority.pointer_definition.valid() || !authority.object.valid() ||
      authority.owner == kInvalidLinkName || authority.owner != function.link_name_id ||
      authority.pointer_type.kind() != LirTypeKind::Pointer || !authority.live) {
    fail_verify(std::string(field),
                "local object authority requires valid current-function pointer, object, "
                "owner, type, and liveness facts");
  }
  const std::size_t owner_count = static_cast<std::size_t>(std::count_if(
      mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
        return candidate.link_name_id == authority.owner;
      }));
  if (owner_count != 1) {
    fail_verify(std::string(field), "local object authority has no unique current-function owner");
  }
}

void verify_local_object_authorities(const LirModule& mod, const LirFunction& function) {
  const auto verify = [&](const auto& op, std::string_view field) {
    if (op.local_object_authority) {
      verify_local_object_authority(mod, function, *op.local_object_authority, field);
    }
  };
  for (const auto& inst : function.alloca_insts) {
    if (const auto* op = std::get_if<LirAllocaOp>(&inst)) verify(*op, "LirAllocaOp.local_object_authority");
    if (const auto* op = std::get_if<LirStoreOp>(&inst)) verify(*op, "LirStoreOp.local_object_authority");
    if (const auto* op = std::get_if<LirGepOp>(&inst)) verify(*op, "LirGepOp.local_object_authority");
    if (const auto* op = std::get_if<LirLoadOp>(&inst)) verify(*op, "LirLoadOp.local_object_authority");
  }
  for (const auto& block : function.blocks) for (const auto& inst : block.insts) {
    if (const auto* op = std::get_if<LirAllocaOp>(&inst)) verify(*op, "LirAllocaOp.local_object_authority");
    if (const auto* op = std::get_if<LirStoreOp>(&inst)) verify(*op, "LirStoreOp.local_object_authority");
    if (const auto* op = std::get_if<LirGepOp>(&inst)) verify(*op, "LirGepOp.local_object_authority");
    if (const auto* op = std::get_if<LirLoadOp>(&inst)) verify(*op, "LirLoadOp.local_object_authority");
    if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) verify(*op, "LirStackSaveOp.local_object_authority");
    if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) verify(*op, "LirStackRestoreOp.local_object_authority");
  }
}

void verify_local_object_authority_bindings(
    const LirFunction& function,
    const std::unordered_map<uint32_t, const LirInst*>& definition_insts) {
  std::unordered_map<uint32_t, const LirCurrentFunctionLocalObjectPointer*>
      authorities_by_pointer;

  const auto verify = [&](const LirCurrentFunctionLocalObjectPointer& authority,
                          const LirOperand& pointer,
                          const LirTypeRef* expected_pointee,
                          std::string_view field) {
    const LirValueId* pointer_id = pointer.value_id();
    if (!pointer_id || *pointer_id != authority.pointer_definition) {
      fail_verify(std::string(field),
                  "local object authority must identify this instruction's pointer operand");
    }
    const auto definition = definition_insts.find(pointer_id->value);
    if (definition == definition_insts.end() || definition->second == nullptr ||
        !modeled_pointer_result(*definition->second)) {
      fail_verify(std::string(field),
                  "local object authority must identify a current-function pointer definition");
    }
    if (expected_pointee && authority.pointee_type != *expected_pointee) {
      fail_verify(std::string(field),
                  "local object authority pointee type disagrees with the instruction type");
    }

    const auto [existing, inserted] =
        authorities_by_pointer.emplace(authority.pointer_definition.value, &authority);
    if (!inserted) {
      const auto& canonical = *existing->second;
      if (authority.object != canonical.object || authority.owner != canonical.owner ||
          authority.pointer_type != canonical.pointer_type ||
          authority.pointee_type != canonical.pointee_type ||
          authority.live != canonical.live) {
        fail_verify(std::string(field),
                    "local object authority disagrees with the pointer's canonical object facts");
      }
    }
  };

  const auto verify_inst = [&](const LirInst& inst) {
    if (const auto* op = std::get_if<LirAllocaOp>(&inst);
        op && op->local_object_authority) {
      verify(*op->local_object_authority, op->result, &op->type_str,
             "LirAllocaOp.local_object_authority");
    } else if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
      verify_native_local_store_authority(*op);
      if (op->local_object_authority) {
        verify(*op->local_object_authority, op->ptr, nullptr,
               "LirStoreOp.local_object_authority");
      }
    } else if (const auto* op = std::get_if<LirLoadOp>(&inst);
               op && op->local_object_authority) {
      if (!op->requires_native_result_authority || !op->result.value_id() ||
          !op->result.value_id()->valid()) {
        fail_verify("LirLoadOp.local_object_authority",
                    "selected local-scalar load requires native result authority");
      }
      verify(*op->local_object_authority, op->ptr, &op->type_str,
             "LirLoadOp.local_object_authority");
    } else if (const auto* op = std::get_if<LirGepOp>(&inst);
               op && op->local_object_authority) {
      // A selected GEP may step from an aggregate local to an element, so its
      // element type is not necessarily the local object's pointee type.  The
      // canonical pointer facts below still bind its object and pointee type.
      verify(*op->local_object_authority, op->ptr, nullptr,
             "LirGepOp.local_object_authority");
      verify_native_local_gep_authority(*op);
    } else if (const auto* op = std::get_if<LirGepOp>(&inst)) {
      verify_native_local_gep_authority(*op);
    } else if (const auto* op = std::get_if<LirStackSaveOp>(&inst);
               op && op->local_object_authority) {
      verify(*op->local_object_authority, op->result, nullptr,
             "LirStackSaveOp.local_object_authority");
    } else if (const auto* op = std::get_if<LirStackRestoreOp>(&inst);
               op && op->local_object_authority) {
      verify(*op->local_object_authority, op->saved_ptr, nullptr,
             "LirStackRestoreOp.local_object_authority");
    }
  };

  for (const auto& inst : function.alloca_insts) verify_inst(inst);
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) verify_inst(inst);
  }
}

void verify_native_memory_va_authority(
    const LirModule& mod, const LirFunction& function,
    const std::unordered_map<uint32_t, const LirInst*>& definition_insts) {
  std::unordered_map<uint32_t, const LirCurrentFunctionLocalObjectPointer*>
      canonical_pointers;
  const auto record_local = [&](const auto& op) {
    if (!op.local_object_authority) return;
    canonical_pointers.emplace(op.local_object_authority->pointer_definition.value,
                               &*op.local_object_authority);
  };
  const auto record = [&](const LirInst& inst) {
    if (const auto* op = std::get_if<LirAllocaOp>(&inst)) record_local(*op);
    if (const auto* op = std::get_if<LirStoreOp>(&inst)) record_local(*op);
    if (const auto* op = std::get_if<LirGepOp>(&inst)) record_local(*op);
    if (const auto* op = std::get_if<LirLoadOp>(&inst)) record_local(*op);
    if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) record_local(*op);
    if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) record_local(*op);
  };
  for (const auto& inst : function.alloca_insts) record(inst);
  for (const auto& block : function.blocks)
    for (const auto& inst : block.insts) record(inst);

  const auto verify_pointer = [&](const LirOperand& operand,
                                  const LirMemoryVaPointerAuthority& binding,
                                  std::string_view field) {
    const auto& authority = binding.local_pointer;
    verify_local_object_authority(mod, function, authority, field);
    if (operand.kind() != LirOperandKind::SsaValue || !operand.value_id() ||
        *operand.value_id() != authority.pointer_definition) {
      fail_verify(std::string(field),
                  "native memory/VA authority must bind its current-function pointer operand");
    }
    const auto definition = definition_insts.find(authority.pointer_definition.value);
    if (definition == definition_insts.end() || !definition->second ||
        !modeled_pointer_result(*definition->second)) {
      fail_verify(std::string(field),
                  "native memory/VA authority must identify a pointer definition");
    }
    const auto canonical = canonical_pointers.find(authority.pointer_definition.value);
    if (canonical == canonical_pointers.end() || !canonical->second ||
        canonical->second->object != authority.object ||
        canonical->second->owner != authority.owner ||
        canonical->second->pointer_type != authority.pointer_type ||
        canonical->second->pointee_type != authority.pointee_type ||
        canonical->second->live != authority.live) {
      fail_verify(std::string(field),
                  "native memory/VA authority disagrees with canonical local pointer facts");
    }
  };
  const auto verify_integer = [&](const LirOperand& operand,
                                  const LirMemoryVaIntegerAuthority& authority,
                                  unsigned width, bool positive,
                                  std::string_view field) {
    if (authority.type != LirTypeRef::integer(width) ||
        !integer_immediate_representable(authority.value.value, width) ||
        (positive && authority.value.value <= 0) ||
        !operand.integer_immediate() ||
        operand.integer_immediate()->value != authority.value.value) {
      fail_verify(std::string(field),
                  "native memory/VA integer authority must bind a matching typed immediate");
    }
  };
  const auto verify_memcpy = [&](const LirMemcpyOp& op) {
    const bool fields = op.dst_authority || op.src_authority || op.size_authority ||
                        op.amd64_sysv_overflow_aggregate_carrier;
    if (!op.requires_native_memory_va_authority) {
      if (fields) fail_verify("LirMemcpyOp.requires_native_memory_va_authority",
                              "unselected memcpy must not carry native memory/VA authority");
      return;
    }
    if (op.amd64_sysv_overflow_aggregate_carrier) {
      if (op.dst_authority || op.src_authority || op.size_authority || op.is_volatile) {
        fail_verify("LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier",
                    "overflow aggregate carrier is exclusive and requires non-volatile memcpy");
      }
      const auto& carrier = *op.amd64_sysv_overflow_aggregate_carrier;
      verify_local_object_authority(mod, function, carrier.va_list_object,
                                    "LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier.va_list_object");
      verify_local_object_authority(mod, function, carrier.destination,
                                    "LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier.destination");
      const auto canonical_va = canonical_pointers.find(carrier.va_list_object.pointer_definition.value);
      const auto canonical_dst = canonical_pointers.find(carrier.destination.pointer_definition.value);
      if (canonical_va == canonical_pointers.end() || canonical_dst == canonical_pointers.end() ||
          !canonical_va->second || !canonical_dst->second ||
          canonical_va->second->object != carrier.va_list_object.object ||
          canonical_va->second->owner != carrier.va_list_object.owner ||
          canonical_va->second->pointee_type != carrier.va_list_object.pointee_type ||
          canonical_va->second->live != carrier.va_list_object.live ||
          canonical_dst->second->object != carrier.destination.object ||
          canonical_dst->second->owner != carrier.destination.owner ||
          canonical_dst->second->pointee_type != carrier.destination.pointee_type ||
          canonical_dst->second->live != carrier.destination.live) {
        fail_verify("LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier",
                    "carrier local objects disagree with canonical current-function facts");
      }
      const auto field = definition_insts.find(carrier.overflow_field_address.value);
      const auto source = definition_insts.find(carrier.overflow_pointer_load.value);
      const auto destination = definition_insts.find(carrier.destination.pointer_definition.value);
      const auto final_load = definition_insts.find(carrier.final_load.value);
      const auto* gep = field == definition_insts.end() ? nullptr :
          std::get_if<LirGepOp>(field->second);
      const auto* load = source == definition_insts.end() ? nullptr :
          std::get_if<LirLoadOp>(source->second);
      const auto* alloca = destination == definition_insts.end() ? nullptr :
          std::get_if<LirAllocaOp>(destination->second);
      const auto* result_load = final_load == definition_insts.end() ? nullptr :
          std::get_if<LirLoadOp>(final_load->second);
      const auto typed_index = [](const LirGepIndex& index, long long expected) {
        return index.is_authoritative() && index.type_ref() == LirTypeRef::integer(32) &&
               index.value().integer_immediate() &&
               index.value().integer_immediate()->value == expected;
      };
      if (!gep || !load || !alloca || !result_load ||
          !carrier.overflow_field_address.valid() || !carrier.overflow_pointer_load.valid() ||
          !carrier.final_load.valid() ||
          gep->element_type.kind() != LirTypeKind::Struct ||
          !gep->ptr.value_id() || *gep->ptr.value_id() != carrier.va_list_object.pointer_definition ||
          gep->indices.size() != 2 || !typed_index(gep->indices[0], 0) ||
          !typed_index(gep->indices[1], 2) || load->type_str.kind() != LirTypeKind::Pointer ||
          !load->ptr.value_id() || *load->ptr.value_id() != carrier.overflow_field_address ||
          !op.src.value_id() || *op.src.value_id() != carrier.overflow_pointer_load ||
          !op.dst.value_id() || *op.dst.value_id() != carrier.destination.pointer_definition ||
          !alloca->local_object_authority ||
          alloca->local_object_authority->object != carrier.destination.object ||
          !result_load->ptr.value_id() ||
          *result_load->ptr.value_id() != carrier.destination.pointer_definition ||
          carrier.storage != LirAmd64SysVOverflowStorage::Amd64SysVOverflowArgArea) {
        fail_verify("LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier",
                    "carrier must be the direct va_list field-2 overflow load into its live temporary");
      }
      if (carrier.payload_type.empty() || carrier.payload_type.kind() != LirTypeKind::Struct ||
          carrier.payload_type != alloca->type_str || carrier.payload_type != result_load->type_str ||
          carrier.payload_size_type != LirTypeRef::integer(64) || carrier.payload_size.value <= 0 ||
          !op.size.integer_immediate() ||
          op.size.integer_immediate()->value != carrier.payload_size.value) {
        fail_verify("LirMemcpyOp.amd64_sysv_overflow_aggregate_carrier",
                    "carrier payload type and positive typed i64 byte size must match memcpy, alloca, and load");
      }
      return;
    }
    if (!op.dst_authority || !op.src_authority || !op.size_authority || op.is_volatile)
      fail_verify("LirMemcpyOp.native_memory_va_authority",
                  "selected memcpy requires non-volatile pointer and i64 size authority");
    verify_pointer(op.dst, *op.dst_authority, "LirMemcpyOp.dst_authority");
    verify_pointer(op.src, *op.src_authority, "LirMemcpyOp.src_authority");
    verify_integer(op.size, *op.size_authority, 64, true, "LirMemcpyOp.size_authority");
  };
  const auto verify_memset = [&](const LirMemsetOp& op) {
    const bool fields = op.dst_authority || op.byte_authority || op.size_authority;
    if (!op.requires_native_memory_va_authority) {
      if (fields) fail_verify("LirMemsetOp.requires_native_memory_va_authority",
                              "unselected memset must not carry native memory/VA authority");
      return;
    }
    if (!op.dst_authority || !op.byte_authority || !op.size_authority || op.is_volatile)
      fail_verify("LirMemsetOp.native_memory_va_authority",
                  "selected memset requires non-volatile pointer, byte, and size authority");
    verify_pointer(op.dst, *op.dst_authority, "LirMemsetOp.dst_authority");
    verify_integer(op.byte_val, *op.byte_authority, 8, false, "LirMemsetOp.byte_authority");
    verify_integer(op.size, *op.size_authority, 64, true, "LirMemsetOp.size_authority");
  };
  const auto verify_single_va = [&](const LirOperand& operand, bool selected,
                                    const std::optional<LirMemoryVaPointerAuthority>& authority,
                                    std::string_view field) {
    if (!selected) {
      if (authority) fail_verify(std::string(field),
                                 "unselected VA operation must not carry native pointer authority");
      return;
    }
    if (!authority) fail_verify(std::string(field), "selected VA operation requires pointer authority");
    verify_pointer(operand, *authority, field);
  };
  const auto verify_inst = [&](const LirInst& inst) {
    if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) verify_memcpy(*op);
    if (const auto* op = std::get_if<LirMemsetOp>(&inst)) verify_memset(*op);
    if (const auto* op = std::get_if<LirVaStartOp>(&inst))
      verify_single_va(op->ap_ptr, op->requires_native_memory_va_authority,
                       op->ap_authority, "LirVaStartOp.ap_authority");
    if (const auto* op = std::get_if<LirVaEndOp>(&inst))
      verify_single_va(op->ap_ptr, op->requires_native_memory_va_authority,
                       op->ap_authority, "LirVaEndOp.ap_authority");
    if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
      const bool fields = op->dst_authority || op->src_authority;
      if (!op->requires_native_memory_va_authority) {
        if (fields) fail_verify("LirVaCopyOp.requires_native_memory_va_authority",
                                "unselected va_copy must not carry native pointer authority");
      } else {
        if (!op->dst_authority || !op->src_authority)
          fail_verify("LirVaCopyOp.native_memory_va_authority",
                      "selected va_copy requires both pointer authorities");
        verify_pointer(op->dst_ptr, *op->dst_authority, "LirVaCopyOp.dst_authority");
        verify_pointer(op->src_ptr, *op->src_authority, "LirVaCopyOp.src_authority");
      }
    }
    if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
      const bool fields = op->ap_authority || op->result_authority || op->result_type_authority;
      if (!op->requires_native_memory_va_authority) {
        if (fields) fail_verify("LirVaArgOp.requires_native_memory_va_authority",
                                "unselected va_arg must not carry native authority");
      } else {
        if (!op->ap_authority || !op->result_authority || !op->result_type_authority ||
            !op->result.value_id() || *op->result.value_id() != *op->result_authority ||
            op->type_str != *op->result_type_authority) {
          fail_verify("LirVaArgOp.native_memory_va_authority",
                      "selected va_arg requires matching result and pointer authority");
        }
        verify_pointer(op->ap_ptr, *op->ap_authority, "LirVaArgOp.ap_authority");
      }
    }
  };
  for (const auto& inst : function.alloca_insts) verify_inst(inst);
  for (const auto& block : function.blocks)
    for (const auto& inst : block.insts) verify_inst(inst);
}

void verify_selected_stack_save_authority(const LirFunction& function) {
  std::size_t selected_count = 0;
  const auto count_selected = [&](const LirInst& inst) {
    if (const auto* op = std::get_if<LirStackSaveOp>(&inst);
        op && op->requires_native_stack_save_authority) {
      ++selected_count;
    }
  };
  for (const auto& inst : function.alloca_insts) count_selected(inst);
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) count_selected(inst);
  }
  if (selected_count > 1) {
    fail_verify("LirStackSaveOp.requires_native_stack_save_authority",
                "current function may publish exactly one selected VLA stack-save authority");
  }
}

void verify_selected_memcpy_authority(const LirFunction& function) {
  const auto& pointer_authority = function.selected_memcpy_pointer_authority;
  std::size_t selected_count = 0;

  const auto verify_memcpy = [&](const LirMemcpyOp& memcpy) {
    if (!memcpy.selected_authority.has_value()) return;
    ++selected_count;
    if (!pointer_authority.has_value()) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy authority requires current-function pointer authority");
    }
    if (memcpy.is_volatile) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy authority is only valid for the selected non-volatile row");
    }

    const auto& selected = *memcpy.selected_authority;
    const auto& source = pointer_authority->byval_parameter;
    const auto& destination = pointer_authority->destination_alloca;
    if (!selected.destination.valid() || !selected.source.valid()) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy authority has an invalid pointer value ID");
    }
    if (selected.destination != destination.value || selected.source != source.value) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy pointer identities disagree with current-function authority");
    }
    if (selected.destination_object != destination.object ||
        selected.source_object != source.object ||
        !selected.destination_object.valid() || !selected.source_object.valid()) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy objects disagree with pointer authority");
    }
    if (selected.destination_object_owner != function.link_name_id ||
        selected.source_object_owner != function.link_name_id ||
        selected.destination_object_owner != destination.object_owner ||
        selected.source_object_owner != source.object_owner) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy objects are not owned by the current function");
    }
    if (!selected.destination_live_at_site || !selected.source_live_at_site ||
        !destination.live_at_selected_site || !source.live_at_selected_site) {
      fail_verify("LirMemcpyOp.selected_authority",
                  "selected memcpy pointer authority is not live at the selected site");
    }
    if (selected.size_type.kind() != LirTypeKind::Integer ||
        selected.size_type.integer_bit_width() != std::optional<unsigned>{64}) {
      fail_verify("LirMemcpyOp.selected_authority.size_type",
                  "selected memcpy size authority must be i64");
    }
    if (selected.size.value <= 0) {
      fail_verify("LirMemcpyOp.selected_authority.size",
                  "selected memcpy size authority must be positive");
    }
  };

  for (const auto& inst : function.alloca_insts) {
    if (const auto* memcpy = std::get_if<LirMemcpyOp>(&inst)) verify_memcpy(*memcpy);
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* memcpy = std::get_if<LirMemcpyOp>(&inst)) verify_memcpy(*memcpy);
    }
  }

  if (pointer_authority.has_value() && selected_count != 1) {
    fail_verify("LirMemcpyOp.selected_authority",
                "selected pointer authority requires exactly one selected memcpy row");
  }
}

void verify_function_value_ownership(const LirModule& mod,
                                     const LirFunction& function,
                                     const std::unordered_map<
                                         uint32_t,
                                         std::unordered_set<const LirFunction*>>&
                                         instruction_result_owners) {
  std::unordered_set<uint32_t> definitions;
  std::unordered_map<uint32_t, const LirInst*> definition_insts;

  verify_selected_memcpy_pointer_authority(mod, function, definitions,
                                           definition_insts);
  verify_native_body_parameter_definitions(mod, function, definitions,
                                           definition_insts);
  verify_selected_memcpy_authority(function);
  verify_selected_stack_save_authority(function);
  verify_local_object_authorities(mod, function);

  // Direct label addresses are function-owned pointer constants, not
  // instruction results.  Register them in the same current-function value
  // identity set while deliberately leaving definition_insts instruction-only.
  for (const auto& constant : function.direct_label_address_constants) {
    const std::size_t target_count = std::count_if(
        function.blocks.begin(), function.blocks.end(), [&](const LirBlock& block) {
          return block.id == constant.target;
        });
    if (!constant.value.valid() || constant.owner != function.link_name_id ||
        constant.owner == c4c::kInvalidLinkName ||
        constant.type.kind() != LirTypeKind::Pointer || target_count != 1) {
      fail_verify("LirFunction.direct_label_address_constants",
                  "must define a unique current-function pointer value and target");
    }
    if (!definitions.insert(constant.value.value).second) {
      fail_verify("LirFunction.value_definitions",
                  "duplicate LirValueId direct-constant authority " +
                      std::to_string(constant.value.value));
    }
  }

  const auto collect_operand_definition =
      [&](const LirInst& inst, const LirOperand* result) {
    if (!result) return;
    const LirValueId* id = result->value_id();
    if (!id) return;
    if (!id->valid()) {
      fail_verify("LirFunction.value_definitions",
                  "invalid LirValueId result authority");
    }
    if (!definitions.insert(id->value).second) {
      fail_verify("LirFunction.value_definitions",
                  "duplicate LirValueId result authority " +
                      std::to_string(id->value));
    }
    if (requires_native_result_authority(inst)) {
      const auto owners = instruction_result_owners.find(id->value);
      if (owners != instruction_result_owners.end() &&
          std::any_of(owners->second.begin(), owners->second.end(),
                      [&](const LirFunction* owner) { return owner != &function; })) {
        fail_verify("LirInst.result",
                    "standalone native result LirValueId is owned by another LirFunction");
      }
    }
    definition_insts.emplace(id->value, &inst);
  };

  const auto collect_definition = [&](const LirInst& inst) {
    if (const auto* inline_asm = std::get_if<LirInlineAsmOp>(&inst)) {
      for (const LirInlineAsmValueBinding& result : inline_asm->ordinary_results) {
        collect_operand_definition(inst, &result.value);
      }
      return;
    }
    collect_operand_definition(inst, modeled_result_operand(inst));
  };

  for (const auto& inst : function.alloca_insts) collect_definition(inst);
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) collect_definition(inst);
  }

  std::size_t selected_floating_fadd_lhs_authority_count = 0;
  std::size_t selected_floating_fsub_lhs_authority_count = 0;
  const auto verify_scalar_binary_lhs_authority = [&](const LirBinOp& op) {
    const LirCompactScalarType* compact_scalar =
        compact_scalar_binop_type(op, "LirBinOp.compact_scalar_type");
    const LirTypeRef& binary_type =
        compact_scalar ? compact_scalar->type : op.type_str;
    const auto scalar_lhs_definition = std::find_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return op.lhs.value_id() && definition.value == *op.lhs.value_id() &&
                 definition.abi == LirNativeBodyParameterAbi::DirectScalar;
        });
    const std::optional<LirBinaryOpcode> opcode = op.opcode.typed();
    const bool selected_floating_lhs_consumer =
        scalar_lhs_definition != function.native_body_parameter_definitions.end() &&
        scalar_lhs_definition->type.kind() == LirTypeKind::Floating &&
        (opcode == LirBinaryOpcode::FAdd || opcode == LirBinaryOpcode::FSub ||
         opcode == LirBinaryOpcode::FMul || opcode == LirBinaryOpcode::FNeg);
    const bool floating_lhs_definition =
        scalar_lhs_definition != function.native_body_parameter_definitions.end() &&
        scalar_lhs_definition->type.kind() == LirTypeKind::Floating;
    const bool duplicate_selected_floating_lhs_consumer =
        (opcode == LirBinaryOpcode::FAdd && selected_floating_fadd_lhs_authority_count > 0) ||
        (opcode == LirBinaryOpcode::FSub && selected_floating_fsub_lhs_authority_count > 0);
    if (!op.scalar_lhs_parameter_authority) {
      if (scalar_lhs_definition != function.native_body_parameter_definitions.end() &&
          (!floating_lhs_definition ||
           (selected_floating_lhs_consumer &&
            !duplicate_selected_floating_lhs_consumer))) {
        fail_verify("LirBinOp.scalar_lhs_parameter_authority",
                    "is required when LirBinOp.lhs uses a native direct-scalar parameter");
      }
      return;
    }
    const auto& authority = *op.scalar_lhs_parameter_authority;
    constexpr std::string_view field = "LirBinOp.scalar_lhs_parameter_authority";
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != LirScalarBinaryParameterRole::Lhs ||
        op.lhs.kind() != LirOperandKind::SsaValue || !op.lhs.value_id() ||
        *op.lhs.value_id() != authority.value || binary_type != authority.type) {
      fail_verify(field, "requires one native direct-scalar current-function LHS value binding");
    }
    if (authority.type.kind() == LirTypeKind::Floating) {
      if (opcode != LirBinaryOpcode::FAdd && opcode != LirBinaryOpcode::FSub &&
          opcode != LirBinaryOpcode::FMul &&
          opcode != LirBinaryOpcode::FNeg) {
        fail_verify(field,
                    "floating LHS parameter authority is limited to selected fadd, fsub, fmul, and fneg consumers");
      }
      if (opcode == LirBinaryOpcode::FAdd || opcode == LirBinaryOpcode::FSub ||
          opcode == LirBinaryOpcode::FMul) {
        if (op.rhs.empty() || op.scalar_rhs_parameter_authority) {
          fail_verify(field,
                      "selected floating binary LHS authority requires a nonselected scalar RHS");
        }
        if (opcode == LirBinaryOpcode::FAdd) ++selected_floating_fadd_lhs_authority_count;
        if (opcode == LirBinaryOpcode::FSub) ++selected_floating_fsub_lhs_authority_count;
      }
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (matches != 1 || authority.parameter_index >= function.params.size() ||
        !direct_scalar_parameter_type(function.params[authority.parameter_index].second)) {
      fail_verify(field, "must exactly mirror one native direct-scalar parameter definition");
    }
  };
  std::size_t selected_floating_fmul_rhs_authority_count = 0;
  std::size_t selected_floating_fadd_rhs_authority_count = 0;
  std::size_t selected_floating_fsub_rhs_authority_count = 0;
  const auto verify_scalar_binary_rhs_authority = [&](const LirBinOp& op) {
    const LirCompactScalarType* compact_scalar =
        compact_scalar_binop_type(op, "LirBinOp.compact_scalar_type");
    const LirTypeRef& binary_type =
        compact_scalar ? compact_scalar->type : op.type_str;
    const auto scalar_rhs_definition = std::find_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return op.rhs.value_id() && definition.value == *op.rhs.value_id() &&
                 definition.abi == LirNativeBodyParameterAbi::DirectScalar;
        });
    const std::optional<LirBinaryOpcode> opcode = op.opcode.typed();
    const bool selected_floating_rhs_consumer =
        scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
        scalar_rhs_definition->type.kind() == LirTypeKind::Floating &&
        (opcode == LirBinaryOpcode::FAdd || opcode == LirBinaryOpcode::FSub ||
         opcode == LirBinaryOpcode::FMul);
    const bool floating_rhs_definition =
        scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
        scalar_rhs_definition->type.kind() == LirTypeKind::Floating;
    const bool duplicate_selected_floating_rhs_consumer =
        (opcode == LirBinaryOpcode::FAdd && selected_floating_fadd_rhs_authority_count > 0) ||
        (opcode == LirBinaryOpcode::FSub && selected_floating_fsub_rhs_authority_count > 0) ||
        (opcode == LirBinaryOpcode::FMul && selected_floating_fmul_rhs_authority_count > 0);
    if (!op.scalar_rhs_parameter_authority) {
      if (!op.scalar_lhs_parameter_authority &&
          scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
          (!floating_rhs_definition ||
           (selected_floating_rhs_consumer &&
            !duplicate_selected_floating_rhs_consumer))) {
        fail_verify("LirBinOp.scalar_rhs_parameter_authority",
                    "is required when LirBinOp.rhs uses a native direct-scalar parameter");
      }
      return;
    }
    const auto& authority = *op.scalar_rhs_parameter_authority;
    constexpr std::string_view field = "LirBinOp.scalar_rhs_parameter_authority";
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != LirScalarBinaryParameterRole::Rhs ||
        op.rhs.kind() != LirOperandKind::SsaValue || !op.rhs.value_id() ||
        *op.rhs.value_id() != authority.value || binary_type != authority.type) {
      fail_verify(field, "requires one native direct-scalar current-function RHS value binding");
    }
    if (authority.type.kind() == LirTypeKind::Floating) {
      if (opcode != LirBinaryOpcode::FAdd && opcode != LirBinaryOpcode::FSub &&
          opcode != LirBinaryOpcode::FMul) {
        fail_verify(field,
                    "floating RHS parameter authority is limited to selected fadd, fsub, and fmul consumers");
      }
      const bool lhs_is_direct_scalar_parameter =
          op.lhs.value_id() &&
          std::any_of(function.native_body_parameter_definitions.begin(),
                      function.native_body_parameter_definitions.end(),
                      [&](const auto& definition) {
                        return definition.value == *op.lhs.value_id() &&
                               definition.abi == LirNativeBodyParameterAbi::DirectScalar;
                      });
      if (op.lhs.empty() || op.scalar_lhs_parameter_authority ||
          lhs_is_direct_scalar_parameter) {
        fail_verify(field,
                    "selected floating binary RHS authority requires a nonselected scalar LHS");
      }
      if (opcode == LirBinaryOpcode::FAdd) ++selected_floating_fadd_rhs_authority_count;
      if (opcode == LirBinaryOpcode::FSub) ++selected_floating_fsub_rhs_authority_count;
      if (opcode == LirBinaryOpcode::FMul) ++selected_floating_fmul_rhs_authority_count;
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (matches != 1 || authority.parameter_index >= function.params.size() ||
        !direct_scalar_parameter_type(function.params[authority.parameter_index].second)) {
      fail_verify(field, "must exactly mirror one native direct-scalar parameter definition");
    }
  };
  const auto verify_truthiness_lhs_parameter_authority = [&](const LirCmpOp& op) {
    const auto truthiness_definition =
        !op.is_float && op.predicate.typed() == LirCmpPredicate::Ne &&
                op.type_str.kind() == LirTypeKind::Integer &&
                op.lhs.kind() == LirOperandKind::SsaValue && op.lhs.value_id() &&
                op.rhs.integer_immediate() && op.rhs.integer_immediate()->value == 0
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& definition) {
                             return definition.value == *op.lhs.value_id() &&
                                    definition.type == op.type_str &&
                                    definition.abi ==
                                        LirNativeBodyParameterAbi::DirectScalar;
                           })
            : function.native_body_parameter_definitions.end();
    if (!op.truthiness_lhs_parameter_authority) {
      if (truthiness_definition != function.native_body_parameter_definitions.end()) {
        fail_verify("LirCmpOp.truthiness_lhs_parameter_authority",
                    "is required when LirCmpOp.lhs uses a native direct-scalar parameter in a truthiness comparison");
      }
      return;
    }
    const auto& authority = *op.truthiness_lhs_parameter_authority;
    constexpr std::string_view field = "LirCmpOp.truthiness_lhs_parameter_authority";
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
        authority.role !=
            LirTruthinessComparisonLhsParameterRole::TruthinessComparisonLhs ||
        op.is_float || op.predicate.typed() != LirCmpPredicate::Ne ||
        op.type_str.kind() != LirTypeKind::Integer ||
        op.lhs.kind() != LirOperandKind::SsaValue || !op.lhs.value_id() ||
        *op.lhs.value_id() != authority.value || op.type_str != authority.type ||
        !op.rhs.integer_immediate() || op.rhs.integer_immediate()->value != 0) {
      fail_verify(field,
                  "requires one native direct-scalar current-function truthiness-comparison LHS binding");
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (matches != 1 ||
        !direct_scalar_parameter_type(function.params[authority.parameter_index].second)) {
      fail_verify(field,
                  "must exactly mirror one native direct-scalar parameter definition");
    }
  };
  const auto verify_pointer_truthiness_parameter_authority = [&](const LirCmpOp& op) {
    const LirInst* lhs_definition_inst = nullptr;
    if (op.lhs.kind() == LirOperandKind::SsaValue && op.lhs.value_id()) {
      const auto found = definition_insts.find(op.lhs.value_id()->value);
      if (found != definition_insts.end()) lhs_definition_inst = found->second;
    }
    const auto* ptr_to_int =
        lhs_definition_inst ? std::get_if<LirCastOp>(lhs_definition_inst) : nullptr;
    const auto pointer_definition =
        !op.is_float && op.predicate.typed() == LirCmpPredicate::Ne &&
                op.type_str == LirTypeRef::integer(64) && ptr_to_int &&
                ptr_to_int->kind == LirCastKind::PtrToInt &&
                ptr_to_int->from_type.kind() == LirTypeKind::Pointer &&
                ptr_to_int->to_type == LirTypeRef::integer(64) &&
                ptr_to_int->operand.kind() == LirOperandKind::SsaValue &&
                ptr_to_int->operand.value_id() && op.rhs.integer_immediate() &&
                op.rhs.integer_immediate()->value == 0
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& definition) {
                             return definition.value == *ptr_to_int->operand.value_id() &&
                                    definition.type.kind() == LirTypeKind::Pointer &&
                                    definition.abi ==
                                        LirNativeBodyParameterAbi::DirectPointer;
                           })
            : function.native_body_parameter_definitions.end();
    if (!op.pointer_truthiness_parameter_authority) {
      if (pointer_definition != function.native_body_parameter_definitions.end()) {
        fail_verify("LirCmpOp.pointer_truthiness_parameter_authority",
                    "is required when LirCmpOp compares a native direct-pointer parameter PtrToInt against zero");
      }
      return;
    }
    const auto& authority = *op.pointer_truthiness_parameter_authority;
    constexpr std::string_view field = "LirCmpOp.pointer_truthiness_parameter_authority";
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectPointer ||
        authority.role != LirPointerTruthinessParameterRole::PointerTruthiness ||
        !ptr_to_int || ptr_to_int->kind != LirCastKind::PtrToInt ||
        ptr_to_int->from_type.kind() != LirTypeKind::Pointer ||
        ptr_to_int->to_type != LirTypeRef::integer(64) ||
        ptr_to_int->operand.kind() != LirOperandKind::SsaValue ||
        !ptr_to_int->operand.value_id() ||
        *ptr_to_int->operand.value_id() != authority.value ||
        op.is_float || op.predicate.typed() != LirCmpPredicate::Ne ||
        op.type_str != LirTypeRef::integer(64) ||
        op.lhs.kind() != LirOperandKind::SsaValue || !op.lhs.value_id() ||
        !op.rhs.integer_immediate() || op.rhs.integer_immediate()->value != 0 ||
        authority.type.kind() != LirTypeKind::Pointer) {
      fail_verify(field,
                  "requires one native direct-pointer current-function truthiness binding");
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (matches != 1 || authority.parameter_index >= function.params.size() ||
        function.params[authority.parameter_index].second.ptr_level == 0 ||
        function.params[authority.parameter_index].second.array_rank != 0) {
      fail_verify(field,
                  "must exactly mirror one native direct-pointer parameter definition");
    }
  };
  const auto verify_fixed_direct_call_argument_parameter_authority =
      [&](const LirCallOp& call, const std::size_t argument_index,
          const LirFixedDirectCallArgumentParameterRole role,
          const std::string_view field) {
    const bool selected_consumer =
        call.direct_callee_link_name_id != kInvalidLinkName &&
        call.callee.kind() == LirOperandKind::Global && call.callee.link_name_id() &&
        *call.callee.link_name_id() == call.direct_callee_link_name_id &&
        call.callee_signature && !call.callee_signature->is_variadic &&
        !call.callee_signature->has_unspecified_params &&
        call.structured_args.size() > argument_index &&
        call.arg_type_refs.size() > argument_index &&
        call.callee_signature->fixed_param_type_refs.size() > argument_index &&
        (argument_index != 1 ||
         (call.structured_args.size() == 2 &&
          call.callee_signature->fixed_param_type_refs.size() == 2));
    const LirCallArg* argument =
        selected_consumer ? &call.structured_args[argument_index] : nullptr;
    const auto definition = argument &&
                                argument->operand.kind() == LirOperandKind::SsaValue &&
                                argument->operand.value_id() && !argument->type_ref.empty() &&
                                argument->type_ref == call.arg_type_refs[argument_index] &&
                                argument->type_ref ==
                                    call.callee_signature->fixed_param_type_refs[argument_index]
                            ? std::find_if(
                                  function.native_body_parameter_definitions.begin(),
                                  function.native_body_parameter_definitions.end(),
                                  [&](const auto& candidate) {
                                    return candidate.value == *argument->operand.value_id() &&
                                           candidate.type == argument->type_ref &&
                                           candidate.abi ==
                                               LirNativeBodyParameterAbi::DirectScalar;
                                  })
                            : function.native_body_parameter_definitions.end();
    if (!argument || !argument->fixed_direct_call_argument_parameter_authority) {
      if (definition != function.native_body_parameter_definitions.end()) {
        fail_verify(field,
                    "is required when a selected fixed direct-call argument uses a native direct-scalar parameter");
      }
      return;
    }
    const auto& authority = *argument->fixed_direct_call_argument_parameter_authority;
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!selected_consumer || !argument || !authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != role ||
        argument->operand.kind() != LirOperandKind::SsaValue || !argument->operand.value_id() ||
        *argument->operand.value_id() != authority.value || argument->type_ref != authority.type ||
        call.arg_type_refs[argument_index] != authority.type ||
        call.callee_signature->fixed_param_type_refs[argument_index] != authority.type) {
      fail_verify(field,
                  "requires one native direct-scalar current-function fixed direct-call argument binding");
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& candidate) {
          return candidate.value == authority.value &&
                 candidate.parameter_index == authority.parameter_index &&
                 candidate.type == authority.type && candidate.owner == authority.owner &&
                 candidate.abi == authority.abi;
        });
    if (matches != 1 || !direct_scalar_parameter_type(
                            function.params[authority.parameter_index].second)) {
      fail_verify(field,
                  "must exactly mirror one native direct-scalar parameter definition");
    }
  };
  const auto verify_structural_direct_call_argument1_identity =
      [&](const LirCallOp& call) {
    const bool direct_two_parameter_call =
        call.direct_callee_link_name_id != kInvalidLinkName &&
        call.callee.kind() == LirOperandKind::Global && call.callee.link_name_id() &&
        *call.callee.link_name_id() == call.direct_callee_link_name_id &&
        call.callee_signature && !call.callee_signature->is_variadic &&
        !call.callee_signature->has_unspecified_params &&
        !call.callee_signature->has_void_param_list &&
        call.callee_signature->fixed_param_type_refs.size() == 2 &&
        call.structured_args.size() == 2;
    if (!direct_two_parameter_call) return;

    const LirCallArg& argument = call.structured_args[1];
    const auto definition =
        argument.operand.kind() == LirOperandKind::SsaValue && argument.operand.value_id()
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& candidate) {
                             return candidate.value == *argument.operand.value_id() &&
                                    candidate.abi == LirNativeBodyParameterAbi::DirectScalar;
                           })
            : function.native_body_parameter_definitions.end();
    if (definition == function.native_body_parameter_definitions.end()) return;

    constexpr std::string_view field = "LirCallOp.structured_args[1]";
    if (!argument.operand.value_id() || !argument.operand.value_id()->valid() ||
        call.arg_type_refs.size() != 2 || argument.type_ref.empty() ||
        call.arg_type_refs[1].empty() ||
        argument.type_ref != call.arg_type_refs[1] ||
        argument.type_ref != call.callee_signature->fixed_param_type_refs[1] ||
        argument.type_ref != definition->type) {
      fail_verify(field,
                  "requires a native argument-1 SSA identity and matching fixed parameter type");
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& candidate) {
          return candidate.value == *argument.operand.value_id() &&
                 candidate.type == argument.type_ref &&
                 candidate.abi == LirNativeBodyParameterAbi::DirectScalar;
        });
    if (matches != 1) {
      fail_verify(field,
                  "must identify one current-function direct-scalar SSA definition");
    }
  };
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* op = std::get_if<LirBinOp>(&inst)) {
        verify_scalar_binary_lhs_authority(*op);
        verify_scalar_binary_rhs_authority(*op);
      }
      if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
        verify_truthiness_lhs_parameter_authority(*op);
        verify_pointer_truthiness_parameter_authority(*op);
      }
      if (const auto* call = std::get_if<LirCallOp>(&inst)) {
        verify_fixed_direct_call_argument_parameter_authority(
            *call, 0, LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument0,
            "LirCallOp.structured_args[0].fixed_direct_call_argument_parameter_authority");
        verify_fixed_direct_call_argument_parameter_authority(
            *call, 1, LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument1,
            "LirCallOp.structured_args[1].fixed_direct_call_argument_parameter_authority");
        verify_structural_direct_call_argument1_identity(*call);
      }
    }
  }
  if (selected_floating_fmul_rhs_authority_count > 1) {
    fail_verify("LirBinOp.scalar_rhs_parameter_authority",
                "current function may publish exactly one selected floating fmul RHS authority");
  }
  if (selected_floating_fadd_rhs_authority_count > 1) {
    fail_verify("LirBinOp.scalar_rhs_parameter_authority",
                "current function may publish exactly one selected floating fadd RHS authority");
  }
  if (selected_floating_fsub_rhs_authority_count > 1) {
    fail_verify("LirBinOp.scalar_rhs_parameter_authority",
                "current function may publish exactly one selected floating fsub RHS authority");
  }
  if (selected_floating_fadd_lhs_authority_count > 1) {
    fail_verify("LirBinOp.scalar_lhs_parameter_authority",
                "current function may publish exactly one selected floating fadd LHS authority");
  }
  if (selected_floating_fsub_lhs_authority_count > 1) {
    fail_verify("LirBinOp.scalar_lhs_parameter_authority",
                "current function may publish exactly one selected floating fsub LHS authority");
  }

  const auto verify_return_value_parameter_authority = [&](const LirRet& ret) {
    const auto returned_definition =
        ret.type_str.kind() == LirTypeKind::Integer && ret.value_str &&
                ret.value_str->kind() == LirOperandKind::SsaValue &&
                ret.value_str->value_id() && function.signature_return_type_ref &&
                *function.signature_return_type_ref == ret.type_str
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& definition) {
                             return definition.value == *ret.value_str->value_id() &&
                                    definition.type == ret.type_str &&
                                     definition.abi ==
                                         LirNativeBodyParameterAbi::DirectScalar;
                           })
            : function.native_body_parameter_definitions.end();
    if (!ret.return_value_parameter_authority) {
      if (returned_definition != function.native_body_parameter_definitions.end()) {
        fail_verify("LirRet.return_value_parameter_authority",
                    "is required when LirRet returns a native direct-scalar parameter");
      }
      return;
    }
    const auto& authority = *ret.return_value_parameter_authority;
    constexpr std::string_view field = "LirRet.return_value_parameter_authority";
    const bool unique_owner = authority.owner != kInvalidLinkName &&
        authority.owner == function.link_name_id &&
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) == 1;
    if (!authority.value.valid() || !unique_owner ||
        authority.parameter_index >= function.params.size() ||
        authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != LirReturnValueParameterRole::ReturnValue ||
        ret.type_str.kind() != LirTypeKind::Integer || !ret.value_str ||
        ret.value_str->kind() != LirOperandKind::SsaValue || !ret.value_str->value_id() ||
        *ret.value_str->value_id() != authority.value || ret.type_str != authority.type ||
        !function.signature_return_type_ref ||
        *function.signature_return_type_ref != ret.type_str) {
      fail_verify(field,
                  "requires one native direct-scalar current-function return-value binding");
    }
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (matches != 1 || authority.parameter_index >= function.params.size() ||
        !direct_scalar_parameter_type(function.params[authority.parameter_index].second)) {
      fail_verify(field, "must exactly mirror one native direct-scalar parameter definition");
    }
  };
  for (const auto& block : function.blocks) {
    if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
      verify_return_value_parameter_authority(*ret);
    }
  }

  const auto verify_vector_authority = [&](const auto& op, std::string_view name,
                                           const LirOperand& first, const LirOperand* second,
                                           const LirOperand* element, const LirOperand* index,
                                           const LirTypeRef* index_type,
                                           const LirTypeRef& vector_type,
                                           bool check_vector_shapes = true) {
    if (!op.native_vector_authority) return;
    const LirNativeVectorAuthority& authority = *op.native_vector_authority;
    if (authority.owner != function.link_name_id || authority.owner == kInvalidLinkName ||
        std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
          return candidate.link_name_id == authority.owner;
        }) != 1) fail_verify(std::string(name) + ".native_vector_authority.owner", "must name one current function");
    if (!authority.result.valid() || !op.result.value_id() || *op.result.value_id() != authority.result ||
        !definitions.count(authority.result.value)) fail_verify(std::string(name) + ".native_vector_authority.result", "must mirror a defined current-function result");
    const auto check_use = [&](const std::optional<LirValueId>& id, const LirOperand& mirror, std::string_view field) {
      if (mirror.value_id()) {
        if (!id || *id != *mirror.value_id() || !definitions.count(id->value))
          fail_verify(std::string(name) + ".native_vector_authority." + std::string(field), "must mirror a defined current-function value use");
      } else if (id) {
        fail_verify(std::string(name) + ".native_vector_authority." + std::string(field), "must not invent a value ID for a non-value mirror");
      } else if (!mirror.special_token() &&
                 (mirror.kind() == LirOperandKind::SsaValue ||
                  mirror.kind() == LirOperandKind::DirectConstant)) {
        fail_verify(std::string(name) + ".native_vector_authority." + std::string(field),
                    "non-special value uses require a current-function LirValueId");
      }
    };
    check_use(authority.first_vector_use, first, "first_vector_use");
    if (second) check_use(authority.second_vector_use, *second, "second_vector_use");
    if (element) check_use(authority.element_use, *element, "element_use");
    const auto check_shape = [&](const LirNativeVectorShape& shape, const LirTypeRef& mirror, std::string_view field) {
      if (!shape.lane_count || shape.element_type.str().empty() ||
          mirror.str() != "<" + std::to_string(shape.lane_count) + " x " + shape.element_type.str() + ">")
        fail_verify(std::string(name) + ".native_vector_authority." + std::string(field), "must have a coherent vector display mirror");
    };
    if (check_vector_shapes) {
      check_shape(authority.result_shape, vector_type, "result_shape");
      if (!authority.first_vector_shape) fail_verify(std::string(name) + ".native_vector_authority.first_vector_shape", "must be present");
      check_shape(*authority.first_vector_shape, vector_type, "first_vector_shape");
      if (second) {
        if (!authority.second_vector_shape)
          fail_verify(std::string(name) + ".native_vector_authority.second_vector_shape", "must be present");
        check_shape(*authority.second_vector_shape, vector_type, "second_vector_shape");
      }
    }
    if (index) {
      if (!authority.index || !index_type ||
          authority.index->value.kind() != index->kind() ||
          authority.index->value.authority() != index->authority() || authority.index->type.str().empty())
        fail_verify(std::string(name) + ".native_vector_authority.index", "must mirror the structured index operand");
      if (authority.index->type != *index_type) {
        fail_verify(std::string(name) + ".native_vector_authority.index.type",
                    "must match the operation's native index type mirror");
      }
      if (const LirValueId* value = authority.index->value.value_id();
          value && (!value->valid() || !definitions.count(value->value))) {
        fail_verify(std::string(name) + ".native_vector_authority.index.value",
                    "must name a defined current-function index value");
      }
    }
  };
  const auto vector_element_has_accepted_aggregate_fact =
      [&](const LirTypeRef& element_type) {
        if (element_type.kind() != LirTypeKind::Struct) return true;
        if (!element_type.has_struct_name_id()) return false;
        return std::any_of(mod.aggregate_store.begin(), mod.aggregate_store.end(),
                           [&](const LirAggregateStoreEntry& entry) {
                             return entry.name_id == element_type.struct_name_id();
                           });
      };
  const auto verify_required_insert_vector_store =
      [&](const LirInsertElementOp& op, const LirNativeVectorAuthority& authority) {
        if (!authority.vector_ref) {
          fail_verify("LirInsertElementOp.native_vector_authority.vector_ref",
                      "must name the scalar-to-vector splat's native vector store fact");
        }
        const LirVectorStoreEntry* vector = mod.find_vector(*authority.vector_ref);
        if (!vector) {
          fail_verify("LirInsertElementOp.native_vector_authority.vector_ref",
                      "must reference a module-owned vector store fact");
        }
        if (vector->lane_count == 0 || vector->element_type.empty()) {
          fail_verify("LirInsertElementOp.native_vector_authority.vector_ref",
                      "must reference a complete vector store fact");
        }
        if (!vector_element_has_accepted_aggregate_fact(vector->element_type)) {
          fail_verify("LirInsertElementOp.native_vector_authority.vector_ref",
                      "aggregate element vectors must consume an accepted aggregate store fact");
        }
        if (authority.result_shape.lane_count != vector->lane_count ||
            !same_native_type_fact(authority.result_shape.element_type,
                                   vector->element_type)) {
          fail_verify("LirInsertElementOp.native_vector_authority.result_shape",
                      "must match the scalar-to-vector splat vector store fact");
        }
        if (!same_native_type_fact(op.elem_type, vector->element_type)) {
          fail_verify("LirInsertElementOp.elem_type",
                      "must match the scalar-to-vector splat vector store element type");
        }
      };
  const auto verify_extract_vector_store =
      [&](const LirExtractElementOp& op, const LirNativeVectorAuthority& authority) {
        if (!authority.vector_ref) {
          fail_verify("LirExtractElementOp.native_vector_authority.vector_ref",
                      "must name the direct vector index vector store fact");
        }
        const LirVectorStoreEntry* vector = mod.find_vector(*authority.vector_ref);
        if (!vector) {
          fail_verify("LirExtractElementOp.native_vector_authority.vector_ref",
                      "must reference a module-owned vector store fact");
        }
        if (vector->lane_count == 0 || vector->element_type.empty()) {
          fail_verify("LirExtractElementOp.native_vector_authority.vector_ref",
                      "must reference a complete vector store fact");
        }
        if (!vector_element_has_accepted_aggregate_fact(vector->element_type)) {
          fail_verify("LirExtractElementOp.native_vector_authority.vector_ref",
                      "aggregate element vectors must consume an accepted aggregate store fact");
        }
        if (authority.result_shape.lane_count != vector->lane_count ||
            !same_native_type_fact(authority.result_shape.element_type,
                                   vector->element_type)) {
          fail_verify("LirExtractElementOp.native_vector_authority.result_shape",
                      "must match the direct vector index vector store fact");
        }
      };
  const auto verify_required_shuffle_vector_store =
      [&](const LirShuffleVectorOp& op, const LirNativeVectorAuthority& authority) {
        if (!authority.vector_ref) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.vector_ref",
                      "must name the scalar-to-vector splat's native vector store fact");
        }
        const LirVectorStoreEntry* vector = mod.find_vector(*authority.vector_ref);
        if (!vector) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.vector_ref",
                      "must reference a module-owned vector store fact");
        }
        if (vector->lane_count == 0 || vector->element_type.empty()) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.vector_ref",
                      "must reference a complete vector store fact");
        }
        if (!vector_element_has_accepted_aggregate_fact(vector->element_type)) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.vector_ref",
                      "aggregate element vectors must consume an accepted aggregate store fact");
        }
        if (authority.result_shape.lane_count != vector->lane_count ||
            !same_native_type_fact(authority.result_shape.element_type,
                                   vector->element_type)) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.result_shape",
                      "must match the scalar-to-vector splat vector store fact");
        }
        if (authority.mask_lanes.size() != vector->lane_count ||
            authority.result_shape.lane_count != vector->lane_count ||
            op.mask_type.str() != "<" + std::to_string(vector->lane_count) + " x i32>") {
          fail_verify("LirShuffleVectorOp.native_vector_authority.mask_lanes",
                      "must mirror the scalar-to-vector splat vector store lane count");
        }
        if (!op.vec2.special_token() || *op.vec2.special_token() != LirSpecialToken::Poison ||
            authority.second_vector_use) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.second_vector_use",
                      "must mirror the scalar-to-vector splat poison second vector");
        }
      };
  const auto verify_vector_inst = [&](const LirInst& inst, const LirInst* preceding) {
    if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
      const LirTypeRef index_type = LirTypeRef::integer(64);
      if (op->requires_native_vector_authority && !op->native_vector_authority) {
        fail_verify("LirInsertElementOp.native_vector_authority",
                    "is required for the scalar-to-vector splat precursor");
      }
      const bool check_vector_shapes = !op->requires_native_vector_authority;
      verify_vector_authority(*op, "LirInsertElementOp", op->vec, nullptr, &op->elem,
                              &op->index, &index_type, op->vec_type, check_vector_shapes);
      if (op->requires_native_vector_authority) {
        const auto& authority = *op->native_vector_authority;
        if (!authority.index || !op->index.integer_immediate() ||
            op->index.integer_immediate()->value != 0 ||
            !authority.index->value.integer_immediate() ||
            authority.index->value.integer_immediate()->value != 0 ||
            authority.index->type != LirTypeRef::integer(64)) {
          fail_verify("LirInsertElementOp.native_vector_authority.index",
                      "must bind the scalar-to-vector splat's native i64 zero index");
        }
        verify_required_insert_vector_store(*op, authority);
      }
    } else if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
      // The only ExtractElement producer is direct vector IndexExpr lowering.
      // That route always coerces its index to i32 and publishes its native
      // result/vector/index/shape carrier at the LIR boundary.
      if (!op->native_vector_authority) {
        fail_verify("LirExtractElementOp.native_vector_authority",
                    "is required for direct vector IndexExpr lowering");
      }
      if (op->index_type != LirTypeRef::integer(32)) {
        fail_verify("LirExtractElementOp.index_type",
                    "must be the direct vector IndexExpr i32 index type");
      }
      verify_vector_authority(*op, "LirExtractElementOp", op->vec, nullptr, nullptr,
                              &op->index, &op->index_type, op->vec_type,
                              false);
      verify_extract_vector_store(*op, *op->native_vector_authority);
    } else if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
      if (op->requires_native_vector_authority && !op->native_vector_authority) {
        fail_verify("LirShuffleVectorOp.native_vector_authority",
                    "is required for the scalar-to-vector zero-initializer splat");
      }
      const LirOperand* second_vector_mirror =
          op->requires_native_vector_authority ? nullptr : &op->vec2;
      const bool check_vector_shapes = !op->requires_native_vector_authority;
      verify_vector_authority(*op, "LirShuffleVectorOp", op->vec1, second_vector_mirror,
                              nullptr, nullptr, nullptr, op->vec_type,
                              check_vector_shapes);
      if (op->native_vector_authority &&
          ((!op->requires_native_vector_authority &&
            (op->native_vector_authority->mask_lanes.size() != op->native_vector_authority->result_shape.lane_count ||
             op->mask_type.str() != "<" + std::to_string(op->native_vector_authority->result_shape.lane_count) + " x i32>")) ||
           !op->mask.special_token() || *op->mask.special_token() != LirSpecialToken::ZeroInitializer ||
           std::any_of(op->native_vector_authority->mask_lanes.begin(),
                       op->native_vector_authority->mask_lanes.end(),
                       [](const LirShuffleMaskLane& lane) {
                         return lane.kind != LirShuffleMaskLane::Kind::Selected || lane.selected_lane != 0;
                       })))
        fail_verify("LirShuffleVectorOp.native_vector_authority.mask_lanes", "must mirror the structured shuffle mask");
      if (op->requires_native_vector_authority) {
        const auto* preceding_insert = preceding ? std::get_if<LirInsertElementOp>(preceding) : nullptr;
        if (!preceding_insert || !preceding_insert->native_vector_authority ||
            !op->vec1.value_id() ||
            *op->vec1.value_id() != preceding_insert->native_vector_authority->result) {
          fail_verify("LirShuffleVectorOp.native_vector_authority.first_vector_use",
                      "must equal the preceding native insert result");
        }
        verify_required_shuffle_vector_store(*op, *op->native_vector_authority);
      }
    }
  };
  for (const auto& inst : function.alloca_insts) verify_vector_inst(inst, nullptr);
  for (const auto& block : function.blocks) {
    const LirInst* preceding = nullptr;
    for (const auto& inst : block.insts) {
      verify_vector_inst(inst, preceding);
      preceding = &inst;
    }
  }

  verify_local_object_authority_bindings(function, definition_insts);
  verify_native_memory_va_authority(mod, function, definition_insts);

  const auto successor_at_occurrence = [](const LirTerminator& terminator,
                                          LirSuccessorOccurrenceId occurrence)
      -> std::optional<LirBlockId> {
    if (!occurrence.valid()) return std::nullopt;
    if (const auto* branch = std::get_if<LirBr>(&terminator)) {
      if (occurrence == LirSuccessorOccurrenceId::direct_branch()) {
        return branch->successor;
      }
      return std::nullopt;
    }
    if (const auto* branch = std::get_if<LirCondBr>(&terminator)) {
      if (occurrence == LirSuccessorOccurrenceId::conditional_true()) {
        return branch->true_successor;
      }
      if (occurrence == LirSuccessorOccurrenceId::conditional_false()) {
        return branch->false_successor;
      }
      return std::nullopt;
    }
    if (const auto* sw = std::get_if<LirSwitch>(&terminator)) {
      if (occurrence == LirSuccessorOccurrenceId::switch_default()) {
        return sw->default_successor;
      }
      if (occurrence.value == 0) return std::nullopt;
      const std::size_t case_index = occurrence.value - 1;
      if (case_index < sw->case_successors.size()) {
        return sw->case_successors[case_index];
      }
    }
    return std::nullopt;
  };
  const auto successor_occurrences_to = [&](const LirBlock& predecessor,
                                             LirBlockId destination) {
    std::vector<LirSuccessorOccurrenceId> occurrences;
    const auto add_if_destination = [&](LirSuccessorOccurrenceId occurrence) {
      const std::optional<LirBlockId> successor =
          successor_at_occurrence(predecessor.terminator, occurrence);
      if (successor && *successor == destination) occurrences.push_back(occurrence);
    };
    if (std::holds_alternative<LirBr>(predecessor.terminator)) {
      add_if_destination(LirSuccessorOccurrenceId::direct_branch());
    } else if (std::holds_alternative<LirCondBr>(predecessor.terminator)) {
      add_if_destination(LirSuccessorOccurrenceId::conditional_true());
      add_if_destination(LirSuccessorOccurrenceId::conditional_false());
    } else if (const auto* sw = std::get_if<LirSwitch>(&predecessor.terminator)) {
      add_if_destination(LirSuccessorOccurrenceId::switch_default());
      for (std::size_t i = 0; i < sw->case_successors.size(); ++i) {
        add_if_destination(LirSuccessorOccurrenceId::switch_case(i));
      }
    }
    return occurrences;
  };
  const auto verify_phi_incoming = [&](const LirPhiIncoming& incoming,
                                       const LirBlock& destination) {
    require_operand_kind(incoming.value, "LirPhiIncoming.value",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::DirectConstant,
                          LirOperandKind::Immediate,
                          LirOperandKind::SpecialToken});
    verify_phi_special_token_authority(incoming.value, "LirPhiIncoming.value");
    if (incoming.value.kind() == LirOperandKind::SsaValue) {
      const LirValueId* id = incoming.value.value_id();
      if (!id || !id->valid() || definitions.find(id->value) == definitions.end()) {
        fail_verify("LirPhiIncoming.value",
                    "must identify a known current-function LirValueId");
      }
      const auto owners = instruction_result_owners.find(id->value);
      if (owners != instruction_result_owners.end() &&
          (owners->second.size() != 1 ||
           owners->second.find(&function) == owners->second.end())) {
        fail_verify("LirPhiIncoming.value",
                    "must not identify a value owned by another LirFunction");
      }
    }
    if (!incoming.predecessor.valid()) {
      fail_verify("LirPhiIncoming.predecessor",
                  "must carry a valid current-function LirBlockId");
    }
    const auto predecessor = std::find_if(
        function.blocks.begin(), function.blocks.end(),
        [&](const LirBlock& block) { return block.id == incoming.predecessor; });
    if (predecessor == function.blocks.end() ||
        std::count_if(function.blocks.begin(), function.blocks.end(),
                      [&](const LirBlock& block) {
                        return block.id == incoming.predecessor;
                      }) != 1) {
      fail_verify("LirPhiIncoming.predecessor",
                  "must identify exactly one current-function block");
    }
    if (incoming.label != predecessor->label) {
      fail_verify("LirPhiIncoming.label",
                  "display label must match the predecessor-selected block");
    }
    if (!incoming.successor_occurrence || !incoming.successor_occurrence->valid()) {
      fail_verify("LirPhiIncoming.successor_occurrence",
                  "must carry a valid typed predecessor successor occurrence");
    }
    const std::optional<LirBlockId> selected_successor =
        successor_at_occurrence(predecessor->terminator, *incoming.successor_occurrence);
    if (!selected_successor || !(*selected_successor == destination.id)) {
      fail_verify("LirPhiIncoming.successor_occurrence",
                  "must select an exact predecessor terminator occurrence to the PHI block");
    }
  };
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* phi = std::get_if<LirPhiOp>(&inst)) {
        std::unordered_set<uint64_t> selected_occurrences;
        for (const LirPhiIncoming& incoming : phi->incoming) {
          verify_phi_incoming(incoming, block);
          const uint64_t key = (static_cast<uint64_t>(incoming.predecessor.value) << 32) |
                               incoming.successor_occurrence->value;
          if (!selected_occurrences.insert(key).second) {
            fail_verify("LirPhiIncoming.successor_occurrence",
                        "must not select the same predecessor successor occurrence twice");
          }
        }
        std::unordered_set<uint64_t> required_occurrences;
        for (const LirBlock& predecessor : function.blocks) {
          for (const LirSuccessorOccurrenceId occurrence :
               successor_occurrences_to(predecessor, block.id)) {
            const uint64_t key = (static_cast<uint64_t>(predecessor.id.value) << 32) |
                                 occurrence.value;
            required_occurrences.insert(key);
          }
        }
        if (selected_occurrences != required_occurrences) {
          fail_verify("LirPhiIncoming.successor_occurrence",
                      "must uniquely cover every predecessor successor occurrence to the PHI block");
        }
      }
    }
  }

  const auto verify_conditional_condition = [&](const LirCondBr& branch) {
    if (!branch.condition.valid()) {
      fail_verify("LirCondBr.condition",
                  "must carry a valid current-function LirValueId");
    }
    const auto definition = definition_insts.find(branch.condition.value);
    if (definition == definition_insts.end() || definition->second == nullptr) {
      fail_verify("LirCondBr.condition",
                  "must identify a current-function boolean value definition");
    }
    const auto* comparison = std::get_if<LirCmpOp>(definition->second);
    if (!comparison) {
      fail_verify("LirCondBr.condition",
                  "must identify a current-function boolean comparison result");
    }
    if (!comparison->result.value_id() ||
        *comparison->result.value_id() != branch.condition) {
      fail_verify("LirCondBr.condition",
                  "does not match its selected current-function value definition");
    }
    if (branch.cond_name != comparison->result.str()) {
      fail_verify("LirCondBr.cond_name",
                  "display name must match the condition-selected value definition");
    }
  };
  const auto verify_switch_selector = [&](const LirSwitch& sw) {
    const auto native_selector_definition = std::find_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == sw.selector &&
                 definition.abi == LirNativeBodyParameterAbi::DirectScalar;
        });
    if (!sw.selector_parameter_authority) {
      if (native_selector_definition != function.native_body_parameter_definitions.end()) {
        fail_verify("LirSwitch.selector_parameter_authority",
                    "is required when LirSwitch.selector uses a native direct-scalar parameter");
      }
    } else {
      const auto& authority = *sw.selector_parameter_authority;
      constexpr std::string_view field = "LirSwitch.selector_parameter_authority";
      const bool unique_owner = authority.owner != kInvalidLinkName &&
          authority.owner == function.link_name_id &&
          std::count_if(mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
            return candidate.link_name_id == authority.owner;
          }) == 1;
      if (!authority.value.valid() || !unique_owner ||
          authority.parameter_index >= function.params.size() ||
          authority.abi != LirNativeBodyParameterAbi::DirectScalar ||
          authority.role != LirSwitchSelectorParameterRole::SwitchSelector ||
          sw.selector != authority.value || sw.selector_type_ref != authority.type) {
        fail_verify(field,
                    "requires one native direct-scalar current-function switch-selector binding");
      }
      const auto matches = std::count_if(
          function.native_body_parameter_definitions.begin(),
          function.native_body_parameter_definitions.end(), [&](const auto& definition) {
            return definition.value == authority.value &&
                   definition.parameter_index == authority.parameter_index &&
                   definition.type == authority.type && definition.owner == authority.owner &&
                   definition.abi == authority.abi;
          });
      if (matches != 1 || authority.parameter_index >= function.params.size() ||
          !direct_scalar_parameter_type(function.params[authority.parameter_index].second)) {
        fail_verify(field, "must exactly mirror one native direct-scalar parameter definition");
      }
    }
    if (!sw.selector.valid()) {
      fail_verify("LirSwitch.selector",
                  "must carry a valid current-function LirValueId");
    }
    const auto definition = definition_insts.find(sw.selector.value);
    if (definition == definition_insts.end()) {
      fail_verify("LirSwitch.selector",
                  "must identify a current-function integer value definition");
    }
    const bool native_direct_scalar =
        native_selector_definition != function.native_body_parameter_definitions.end();
    const LirOperand* result = definition->second ? modeled_result_operand(*definition->second) : nullptr;
    const LirTypeRef* type = definition->second ? modeled_scalar_result_type(*definition->second) : nullptr;
    if ((!native_direct_scalar && (!result || !result->value_id() ||
                                   *result->value_id() != sw.selector ||
                                   !type || type->kind() != LirTypeKind::Integer)) ||
        (native_direct_scalar &&
         native_selector_definition->type.kind() != LirTypeKind::Integer)) {
      fail_verify("LirSwitch.selector",
                  "must identify a current-function integer value definition");
    }
    if (result && sw.selector_name != result->str()) {
      fail_verify("LirSwitch.selector_name",
                  "display name must match the selector-selected value definition");
    }
    require_type_ref(sw.selector_type_ref, "LirSwitch.selector_type_ref");
    if (sw.selector_type_ref.kind() != LirTypeKind::Integer ||
        (type && sw.selector_type_ref.integer_bit_width() != type->integer_bit_width()) ||
        (native_direct_scalar && sw.selector_type_ref != native_selector_definition->type)) {
      fail_verify("LirSwitch.selector_type_ref",
                  "must match the selector-selected integer value definition");
    }
    if (sw.selector_type != sw.selector_type_ref.str()) {
      fail_verify("LirSwitch.selector_type",
                  "display type must match the structured selector type authority");
    }
  };
  const auto verify_indirect_br_address = [&](const LirIndirectBrOp& op) {
    if (!op.addr_value.has_value()) {
      fail_verify("LirIndirectBrOp.addr_value",
                  "must carry current-function pointer LirValueId authority");
    }
    if (!op.addr_value->valid()) {
      fail_verify("LirIndirectBrOp.addr_value",
                  "must carry a valid current-function pointer LirValueId");
    }
    if (op.addr.kind() == LirOperandKind::DirectConstant) {
      if (!op.addr.value_id() || *op.addr.value_id() != *op.addr_value) {
        fail_verify("LirIndirectBrOp.addr",
                    "direct constant use must match addr_value identity");
      }
      const auto direct = std::find_if(
          function.direct_label_address_constants.begin(),
          function.direct_label_address_constants.end(), [&](const auto& item) {
            return item.value == *op.addr_value;
          });
      if (direct == function.direct_label_address_constants.end()) {
        fail_verify("LirIndirectBrOp.addr",
                    "direct constant use must resolve in the enclosing function");
      }
      return;
    }
    const auto definition = definition_insts.find(op.addr_value->value);
    if (definition == definition_insts.end() || definition->second == nullptr) {
      fail_verify("LirIndirectBrOp.addr_value",
                  "must identify a current-function pointer value definition");
    }
    const LirOperand* result = modeled_result_operand(*definition->second);
    if (!result || !result->value_id() || *result->value_id() != *op.addr_value ||
        !modeled_pointer_result(*definition->second)) {
      fail_verify("LirIndirectBrOp.addr_value",
                  "must identify a current-function pointer value definition");
    }
    if (op.addr.str() != result->str()) {
      fail_verify("LirIndirectBrOp.addr",
                  "display mirror must match the address-selected value definition");
    }
  };
  for (const auto& block : function.blocks) {
    if (const auto* branch = std::get_if<LirCondBr>(&block.terminator)) {
      verify_conditional_condition(*branch);
    }
    if (const auto* sw = std::get_if<LirSwitch>(&block.terminator)) {
      verify_switch_selector(*sw);
    }
    for (const auto& inst : block.insts) {
      if (const auto* indirect_br = std::get_if<LirIndirectBrOp>(&inst)) {
        verify_indirect_br_address(*indirect_br);
      }
    }
  }

  const auto verify_use = [&](const LirOperand& operand) {
    const LirValueId* id = operand.value_id();
    if (!id) return;
    if (!id->valid()) {
      fail_verify("LirFunction.value_uses", "invalid LirValueId use authority");
    }
    if (definitions.find(id->value) == definitions.end()) {
      fail_verify("LirFunction.value_uses",
                  "unknown current-function LirValueId authority " +
                      std::to_string(id->value));
    }
  };

  for (const auto& inst : function.alloca_insts) {
    visit_modeled_value_uses(inst, verify_use);
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      visit_modeled_value_uses(inst, verify_use);
      if (const auto* gep = std::get_if<LirGepOp>(&inst);
          gep && gep->result.value_id() &&
          gep->ptr.kind() == LirOperandKind::SsaValue && gep->ptr.value_id()) {
        const auto base_definition = definition_insts.find(gep->ptr.value_id()->value);
        const bool native_body_parameter = std::any_of(
            function.native_body_parameter_definitions.begin(),
            function.native_body_parameter_definitions.end(), [&](const auto& parameter) {
              return parameter.value == *gep->ptr.value_id() &&
                     parameter.type.kind() == LirTypeKind::Pointer &&
                     parameter.abi == LirNativeBodyParameterAbi::DirectPointer;
            });
        if (base_definition == definition_insts.end() ||
            (!native_body_parameter &&
             (base_definition->second == nullptr ||
              !modeled_pointer_result(*base_definition->second)))) {
          fail_verify("LirGepOp.ptr",
                      "SSA GEP base must identify a current-function pointer value definition");
        }
      }
      if (const auto* gep = std::get_if<LirGepOp>(&inst);
          gep && gep->result.value_id() &&
          gep->ptr.kind() == LirOperandKind::DirectConstant) {
        const LirValueId* value_id = gep->ptr.value_id();
        const auto direct = value_id ? std::find_if(
            function.direct_label_address_constants.begin(),
            function.direct_label_address_constants.end(), [&](const auto& item) {
              return item.value == *value_id;
            }) : function.direct_label_address_constants.end();
        if (!value_id || direct == function.direct_label_address_constants.end() ||
            direct->type.kind() != LirTypeKind::Pointer || !gep->ptr.str().empty()) {
          fail_verify("LirGepOp.ptr",
                      "direct constant GEP base must match a current-function pointer label address");
        }
      }
      if (const auto* extract = std::get_if<LirExtractValueOp>(&inst);
          extract && extract->requires_native_result_authority &&
          extract->agg.kind() == LirOperandKind::SsaValue) {
        const LirValueId* aggregate_id = extract->agg.value_id();
        if (!aggregate_id || !aggregate_id->valid()) {
          fail_verify("LirExtractValueOp.agg",
                      "aggregate SSA operand requires valid LirValueId authority");
        }
        const auto definition = definition_insts.find(aggregate_id->value);
        if (definition == definition_insts.end() || !definition->second) {
          fail_verify("LirExtractValueOp.agg",
                      "aggregate SSA authority must identify a current-function definition");
        }
        const LirOperand* producer_result = modeled_result_operand(*definition->second);
        const LirTypeRef* producer_type = nullptr;
        bool selected_producer = false;
        if (const auto* call = std::get_if<LirCallOp>(definition->second)) {
          producer_type = &call->return_type;
          // Calls are the pre-existing aggregate SSA handoff.  The selected
          // load/insert extensions below must not narrow that legacy route.
          selected_producer = true;
        } else if (const auto* load = std::get_if<LirLoadOp>(definition->second)) {
          producer_type = load->aggregate_result_type
                              ? &*load->aggregate_result_type : nullptr;
          selected_producer = load->requires_native_result_authority && producer_type;
        } else if (const auto* insert = std::get_if<LirInsertValueOp>(definition->second)) {
          producer_type = insert->aggregate_result_type
                              ? &*insert->aggregate_result_type : nullptr;
          selected_producer = insert->requires_native_result_authority && producer_type;
        }
        if (!selected_producer || !producer_result || !producer_result->value_id() ||
            *producer_result->value_id() != *aggregate_id || !producer_type ||
            !same_native_type_fact(*producer_type, extract->agg_type)) {
          fail_verify("LirExtractValueOp.agg",
                      "aggregate SSA authority must select a matching current-function aggregate producer type");
        }
        if (extract->agg.str() != producer_result->str()) {
          fail_verify("LirExtractValueOp.agg",
                      "aggregate SSA display must mirror its selected producer result");
        }
      }
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        const LirValueId* value_id = store->val.value_id();
        if (store->val.kind() == LirOperandKind::DirectConstant) {
          const auto direct = value_id ? std::find_if(
              function.direct_label_address_constants.begin(),
              function.direct_label_address_constants.end(), [&](const auto& item) {
                return item.value == *value_id;
              }) : function.direct_label_address_constants.end();
          if (!value_id || direct == function.direct_label_address_constants.end() ||
              store->type_str.kind() != LirTypeKind::Pointer ||
              direct->type != store->type_str) {
            fail_verify("LirStoreOp.val",
                        "direct constant store value must match a current-function pointer definition");
          }
        }
        if (value_id) {
          const auto definition = definition_insts.find(value_id->value);
          if (definition != definition_insts.end()) {
            if (const auto* inline_asm =
                    std::get_if<LirInlineAsmOp>(definition->second)) {
              const auto binding = std::find_if(
                  inline_asm->ordinary_results.begin(),
                  inline_asm->ordinary_results.end(),
                  [&](const LirInlineAsmValueBinding& result) {
                    return result.value.value_id() &&
                           *result.value.value_id() == *value_id;
                  });
              if (binding != inline_asm->ordinary_results.end() &&
                  binding->type != store->type_str) {
                fail_verify("LirStoreOp.type_str",
                            "inline-asm output use type must match its semantic binding type");
              }
            }
          }
        }
      }
      const auto* select = std::get_if<LirSelectOp>(&inst);
      if (!select || select->type_str.kind() != LirTypeKind::Integer) continue;
      const LirValueId* condition_id = select->cond.value_id();
      if (!condition_id) continue;
      const auto definition = definition_insts.find(condition_id->value);
      if (definition == definition_insts.end()) continue;
      const auto* comparison = std::get_if<LirCmpOp>(definition->second);
      if (!comparison) {
        fail_verify("LirSelectOp.cond",
                    "authoritative integer select condition requires a comparison result");
      }
      verify_integer_cmp_operand_authority(*comparison);
    }
    if (const auto* ret = std::get_if<LirRet>(&block.terminator);
        ret && ret->value_str.has_value()) {
      verify_use(*ret->value_str);
    }
  }
}

void verify_terminator(const LirFunction& function, const LirTerminator& terminator) {
  const auto verify_successor = [&](LirBlockId successor, std::string_view label,
                                    std::string_view authority_field,
                                    std::string_view label_field) {
    if (!successor.valid()) {
      fail_verify(authority_field, "must carry a valid current-function LirBlockId");
    }
    const auto destination = std::find_if(
        function.blocks.begin(), function.blocks.end(), [&](const LirBlock& block) {
          return block.id == successor;
        });
    if (destination == function.blocks.end() ||
        std::count_if(function.blocks.begin(), function.blocks.end(),
                      [&](const LirBlock& block) { return block.id == successor; }) != 1) {
      fail_verify(authority_field, "must identify exactly one current-function block");
    }
    if (label != destination->label) {
      fail_verify(label_field, "display label must match the successor-selected destination");
    }
  };
  if (const auto* br = std::get_if<LirBr>(&terminator)) {
    verify_successor(br->successor, br->target_label, "LirBr.successor",
                     "LirBr.target_label");
    return;
  }
  if (const auto* cbr = std::get_if<LirCondBr>(&terminator)) {
    if (cbr->cond_name.empty()) {
      fail_verify("LirCondBr.cond_name", "display name must not be empty");
    }
    verify_successor(cbr->true_successor, cbr->true_label,
                     "LirCondBr.true_successor", "LirCondBr.true_label");
    verify_successor(cbr->false_successor, cbr->false_label,
                     "LirCondBr.false_successor", "LirCondBr.false_label");
    return;
  }
  if (const auto* ret = std::get_if<LirRet>(&terminator)) {
    if (ret->type_str.kind() == LirTypeKind::Void) {
      require_type_ref(ret->type_str, "LirRet.type_str", true);
      if (ret->value_str.has_value()) {
        fail_verify("LirRet.value_str", "void return must not carry a value");
      }
      return;
    }
    if (!ret->value_str.has_value()) {
      fail_verify("LirRet.value_str", "non-void return must carry a value");
    }

    const LirOperand& value = *ret->value_str;
    require_operand_kind(value, "LirRet.value_str",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::Global,
                          LirOperandKind::Immediate,
                          LirOperandKind::SpecialToken,
                          LirOperandKind::RawText});
    if (!value.has_authority()) {
      require_type_ref(ret->type_str, "LirRet.type_str", true);
      return;
    }

    if (ret->type_str.kind() != LirTypeKind::Integer) {
      fail_verify("LirRet.type_str",
                  "authoritative scalar return requires integer type");
    }
    (void)render_integer_type_ref(ret->type_str, "LirRet.type_str");
    if (const auto* immediate = value.integer_immediate()) {
      const std::optional<unsigned> width = ret->type_str.integer_bit_width();
      if (!width || !integer_immediate_representable(immediate->value, *width)) {
        fail_verify("LirRet.value_str",
                    "integer immediate is not representable by return type");
      }
      return;
    }
    if (value.value_id()) return;
    if (value.link_name_id()) {
      fail_verify("LirRet.value_str",
                  "LinkNameId authority is not a scalar return value");
    }
    fail_verify("LirRet.value_str", "unsupported scalar return authority");
    return;
  }
  if (const auto* sw = std::get_if<LirSwitch>(&terminator)) {
    if (sw->selector_name.empty() || sw->selector_type.empty() ||
        sw->selector_type_ref.str().empty()) {
      fail_verify("LirSwitch", "must carry selector name and structured selector type");
    }
    verify_successor(sw->default_successor, sw->default_label,
                     "LirSwitch.default_successor", "LirSwitch.default_label");
    if (sw->case_successors.size() != sw->cases.size()) {
      fail_verify("LirSwitch.case_successors",
                  "must carry one current-function LirBlockId for every case");
    }
    for (size_t i = 0; i < sw->cases.size(); ++i) {
      verify_successor(sw->case_successors[i], sw->cases[i].second,
                       "LirSwitch.case_successors", "LirSwitch.cases");
    }
    return;
  }
}

void verify_indirect_br_successors(const LirFunction& function,
                                   const LirIndirectBrOp& op) {
  if (op.successors.size() != op.targets.size()) {
    fail_verify("LirIndirectBrOp.successors",
                "must carry one current-function LirBlockId for every target label");
  }
  std::unordered_set<uint32_t> seen_successors;
  for (size_t i = 0; i < op.successors.size(); ++i) {
    const LirBlockId successor = op.successors[i];
    if (!successor.valid()) {
      fail_verify("LirIndirectBrOp.successors",
                  "must carry valid current-function LirBlockIds");
    }
    const auto destination = std::find_if(
        function.blocks.begin(), function.blocks.end(), [&](const LirBlock& block) {
          return block.id == successor;
        });
    if (destination == function.blocks.end() ||
        std::count_if(function.blocks.begin(), function.blocks.end(),
                      [&](const LirBlock& block) { return block.id == successor; }) != 1) {
      fail_verify("LirIndirectBrOp.successors",
                  "must identify exactly one current-function block per target");
    }
    if (!seen_successors.insert(successor.value).second) {
      fail_verify("LirIndirectBrOp.successors",
                  "must not contain duplicate current-function LirBlockIds");
    }
    if (op.targets[i] != destination->label) {
      fail_verify("LirIndirectBrOp.targets",
                  "display label must match the successor-selected destination");
    }
  }
}

std::string_view legacy_type_decl_name(std::string_view decl) {
  constexpr std::string_view marker = " = type ";
  const std::size_t marker_pos = decl.find(marker);
  if (marker_pos == std::string_view::npos) return {};
  return decl.substr(0, marker_pos);
}

void verify_struct_decl_shadows(const LirModule& mod) {
  // Parse legacy `type_decls` only to prove the compatibility shadow still
  // matches the structured declarations that printer/backend paths consume.
  std::unordered_map<std::string_view, std::string_view> legacy_by_name;
  for (const auto& decl : mod.type_decls) {
    const std::string_view decl_view(decl);
    const std::string_view name = legacy_type_decl_name(decl_view);
    if (name.empty()) continue;
    legacy_by_name.emplace(name, decl_view);
  }

  for (const auto& decl : mod.struct_decls) {
    const std::string_view name = mod.struct_names.spelling(decl.name_id);
    if (name.empty()) {
      fail_verify("LirStructDecl.name_id", "must resolve to a struct name");
    }

    const auto legacy_it = legacy_by_name.find(name);
    if (legacy_it == legacy_by_name.end()) {
      fail_verify("LirStructDecl.shadow",
                  "missing legacy type_decls line for '" + std::string(name) + "'");
    }

    for (const auto& field : decl.fields) {
      require_module_type_ref(mod, field.type, "LirStructDecl.fields.type");
    }

    const std::string shadow = render_struct_decl_llvm(mod, decl);
    if (shadow != legacy_it->second) {
      std::ostringstream detail;
      detail << "structured declaration for '" << name
             << "' does not match legacy type_decls line; shadow '" << shadow
             << "', legacy '" << legacy_it->second << "'";
      fail_verify("LirStructDecl.shadow", detail.str());
    }
  }
}

void verify_extern_decl_shadows(const LirModule& mod) {
  for (const auto& decl : mod.extern_decls) {
    if (!decl.return_type.empty()) {
      const std::string& shadow =
          require_type_ref(decl.return_type, "LirExternDecl.return_type", true);
      const StructNameId return_struct_name_id =
          find_declared_struct_name_id(mod, decl.return_type_str);

      if (decl.return_type.has_struct_name_id()) {
        if (decl.return_type.kind() != LirTypeKind::Struct) {
          fail_verify("LirExternDecl.return_type",
                      "StructNameId mirror must be a struct type");
        }
        const std::string_view rendered_name =
            mod.struct_names.spelling(decl.return_type.struct_name_id());
        if (rendered_name.empty()) {
          fail_verify("LirExternDecl.return_type",
                      "StructNameId mirror must resolve to a struct name");
        }
        if (!mod.find_struct_decl(decl.return_type.struct_name_id())) {
          fail_verify("LirExternDecl.return_type",
                      "StructNameId mirror must resolve to a declared struct");
        }

        if (return_struct_name_id != kInvalidStructName) {
          if (decl.return_type.struct_name_id() != return_struct_name_id) {
            std::ostringstream detail;
            detail << "structured return mirror for extern '" << decl.name
                   << "' names '" << rendered_name
                   << "' but return_type_str names '" << decl.return_type_str << "'";
            fail_verify("LirExternDecl.return_type", detail.str());
          }
          continue;
        }
        continue;
      } else if (return_struct_name_id != kInvalidStructName) {
        fail_verify("LirExternDecl.return_type",
                    "known struct return type must carry matching StructNameId");
      }

      if (shadow != decl.return_type_str) {
        std::ostringstream detail;
        detail << "return_type does not match return_type_str; shadow '" << shadow
               << "', return_type_str '" << decl.return_type_str << "'";
        fail_verify("LirExternDecl.return_type", detail.str());
      }
    }
  }
}

void verify_global_type_ref_shadows(const LirModule& mod) {
  for (const auto& global : mod.globals) {
    const StructNameId global_struct_name_id =
        find_declared_struct_name_id(mod, global.llvm_type);
    const bool selected_direct_aggregate_global =
        (global.type.base == TB_STRUCT || global.type.base == TB_UNION) &&
        global.type.ptr_level == 0 && global.type.array_rank == 0;
    if (!global.llvm_type_ref.has_value()) {
      if (selected_direct_aggregate_global &&
          global_struct_name_id != kInvalidStructName) {
        fail_verify("LirGlobal.llvm_type_ref",
                    "known aggregate global type must carry matching StructNameId");
      }
      continue;
    }
    const LirTypeRef& mirror = *global.llvm_type_ref;
    const std::string& shadow =
        require_type_ref(mirror, "LirGlobal.llvm_type_ref");

    if (mirror.has_struct_name_id()) {
      if (mirror.kind() != LirTypeKind::Struct) {
        fail_verify("LirGlobal.llvm_type_ref",
                    "StructNameId mirror must be a struct type");
      }
      const std::string_view rendered_name =
          mod.struct_names.spelling(mirror.struct_name_id());
      if (rendered_name.empty()) {
        fail_verify("LirGlobal.llvm_type_ref",
                    "StructNameId mirror must resolve to a struct name");
      }
      if (!mod.find_struct_decl(mirror.struct_name_id())) {
        fail_verify("LirGlobal.llvm_type_ref",
                    "StructNameId mirror must resolve to a declared struct");
      }

      if (global_struct_name_id != kInvalidStructName) {
        if (mirror.struct_name_id() != global_struct_name_id) {
          std::ostringstream detail;
          detail << "structured type mirror for global '" << global.name
                 << "' names '" << rendered_name
                 << "' but llvm_type names '" << global.llvm_type << "'";
          fail_verify("LirGlobal.llvm_type_ref", detail.str());
        }
        continue;
      }
      continue;
    }

    if (shadow != global.llvm_type) {
      std::ostringstream detail;
      detail << "structured type mirror for global '" << global.name
             << "' does not match llvm_type; shadow '" << shadow
             << "', llvm_type '" << global.llvm_type << "'";
      fail_verify("LirGlobal.llvm_type_ref", detail.str());
    }
  }
}

void verify_global_initializer_elements(const LirModule& mod) {
  for (const auto& global : mod.globals) {
    for (const auto& element : global.initializer_elements) {
      std::visit(
          [&](const auto& initializer) {
            using T = std::decay_t<decltype(initializer)>;
            if constexpr (std::is_same_v<T, LirGlobalInitializerLabelAddress>) {
              if (initializer.enclosing_function == kInvalidLinkName) {
                fail_verify("LirGlobal.initializer_elements",
                            "label address must carry a valid enclosing LinkNameId");
              }
              const std::size_t function_count = static_cast<std::size_t>(std::count_if(
                  mod.functions.begin(), mod.functions.end(), [&](const LirFunction& function) {
                    return function.link_name_id == initializer.enclosing_function;
                  }));
              if (function_count != 1) {
                fail_verify("LirGlobal.initializer_elements",
                            function_count == 0
                                ? "label address enclosing LinkNameId has no LirFunction owner"
                                : "label address enclosing LinkNameId has ambiguous LirFunction ownership");
              }
              if (!initializer.target.valid()) {
                fail_verify("LirGlobal.initializer_elements",
                            "label address must carry a valid target LirBlockId");
              }
              const auto function = std::find_if(
                  mod.functions.begin(), mod.functions.end(), [&](const LirFunction& candidate) {
                    return candidate.link_name_id == initializer.enclosing_function;
                  });
              const std::size_t block_count = static_cast<std::size_t>(std::count_if(
                  function->blocks.begin(), function->blocks.end(), [&](const LirBlock& block) {
                    return block.id == initializer.target;
                  }));
              if (block_count != 1) {
                fail_verify("LirGlobal.initializer_elements",
                            "label address target LirBlockId must identify exactly one enclosing-function block");
              }
            }
          },
          element);
    }
  }
}

std::string_view function_signature_line(const LirFunction& fn) {
  // Verifier compatibility parser for signature_text. This only confirms that
  // the final LLVM/output payload still contains a function header for legacy
  // hand-built/no-metadata LIR; semantic signature facts are validated from
  // structured LIR metadata below.
  std::string_view signature = fn.signature_text;
  while (!signature.empty()) {
    const size_t line_end = signature.find('\n');
    const std::string_view line =
        line_end == std::string_view::npos ? signature : signature.substr(0, line_end);
    if (line.rfind("define ", 0) == 0 || line.rfind("declare ", 0) == 0) {
      return line;
    }
    if (line_end == std::string_view::npos) break;
    signature.remove_prefix(line_end + 1);
  }
  return {};
}

StructNameId expected_direct_aggregate_signature_id(const LirModule& mod,
                                                    const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0 || type.tag_text_id == kInvalidText ||
      !mod.link_name_texts ||
      (type.tpl_struct_origin && type.tpl_struct_origin[0]) ||
      (type.tpl_struct_args.data && type.tpl_struct_args.size > 0)) {
    return kInvalidStructName;
  }
  const std::string_view tag = mod.link_name_texts->lookup(type.tag_text_id);
  if (tag.empty()) return kInvalidStructName;
  if (tag.rfind("%struct.", 0) == 0 || tag.rfind("%\"struct.", 0) == 0 ||
      tag.rfind("%union.", 0) == 0 || tag.rfind("%\"union.", 0) == 0) {
    return mod.struct_names.find(std::string(tag));
  }
  const std::string struct_rendered =
      c4c::codegen::llvm_helpers::llvm_struct_type_str(std::string(tag));
  if (const StructNameId struct_id = mod.struct_names.find(struct_rendered);
      struct_id != kInvalidStructName) {
    return struct_id;
  }
  return mod.struct_names.find("%union." + std::string(tag));
}

void verify_direct_aggregate_signature_store_entry(const LirModule& mod,
                                                   const TypeSpec& type,
                                                   StructNameId expected_id,
                                                   const LirTypeRef& mirror,
                                                   std::string_view field) {
  if (mod.aggregate_store.empty() && type.namespace_context_id < 0) return;

  const LirAggregateStoreEntry* found = nullptr;
  for (const LirAggregateStoreEntry& entry : mod.aggregate_store) {
    if (entry.name_id == expected_id) {
      found = &entry;
      break;
    }
  }
  if (!found) {
    fail_verify(field,
                "direct aggregate signature mirror requires matching canonical LIR aggregate store entry");
  }

  const bool mirror_is_union =
      mirror.named_composite_kind() == LirNamedCompositeKind::Union;
  if ((found->layout_kind == LirAggregateLayoutKind::Union) != mirror_is_union) {
    fail_verify(field,
                "direct aggregate signature mirror disagrees with canonical aggregate store kind");
  }

  const LirStructDecl* decl = mod.find_struct_decl(expected_id);
  if (!decl) {
    fail_verify(field,
                "direct aggregate signature mirror requires matching structured declaration facts");
  }
  if (decl->name_id != found->name_id || decl->fields.size() != found->fields.size() ||
      decl->is_packed != found->is_packed || decl->is_opaque != found->is_opaque) {
    fail_verify(field,
                "direct aggregate signature mirror disagrees with canonical aggregate store facts");
  }
}

bool aggregate_signature_param_mirror_matches_type(const LirTypeRef& mirror,
                                                   std::string_view type_text) {
  if (mirror.str() == type_text) return true;
  const std::string byval_fragment = "byval(" + std::string(type_text) + ")";
  return mirror.str().find(byval_fragment) != std::string::npos;
}

void verify_signature_type_ref_semantics(const LirModule& mod,
                                         const LirTypeRef& mirror,
                                         std::string_view field) {
  require_type_ref(mirror, field, true);

  if (mirror.has_struct_name_id()) {
    verify_declared_struct_type_ref_mirror(mod, mirror, field);
    const StructNameId rendered_struct_name_id =
        find_declared_struct_name_id(mod, mirror.str());
    if (rendered_struct_name_id != kInvalidStructName &&
        rendered_struct_name_id != mirror.struct_name_id()) {
      fail_verify(field,
                  "StructNameId mirror names a different declared struct than its text");
    }
    return;
  }

  verify_known_struct_type_ref_mirror(mod, mirror, field, "signature");
  if (const auto mismatch = type_ref_struct_name_mismatch_detail(mod.struct_names,
                                                                mirror);
      mismatch.has_value()) {
    fail_verify(field, *mismatch);
  }
}

void verify_function_signature_return_type_ref_mirror(
    const LirModule& mod,
    const LirFunction& fn,
    const LirTypeRef& mirror) {
  constexpr std::string_view field = "LirFunction.signature_return_type_ref";
  verify_signature_type_ref_semantics(mod, mirror, field);

  if (fn.return_type.base == TB_VRM_REGISTER && fn.return_type.ptr_level == 0 &&
      fn.return_type.array_rank == 0) {
    const unsigned expected_width = static_cast<unsigned>(fn.return_type.vrm_width);
    if (mirror.kind() != LirTypeKind::VrmRegister ||
        mirror.vrm_width() != expected_width) {
      std::ostringstream detail;
      detail << "return mirror for function '" << fn.name
             << "' must preserve VRM register carrier width "
             << expected_width << "; mirror '" << mirror.str() << "'";
      fail_verify(field, detail.str());
    }
    return;
  }

  const StructNameId expected_id =
      expected_direct_aggregate_signature_id(mod, fn.return_type);
  if (expected_id == kInvalidStructName) return;

  const std::string_view expected_name = mod.struct_names.spelling(expected_id);
  if (mirror.has_struct_name_id()) {
    if (mirror.struct_name_id() != expected_id) {
      if (fn.signature_text.find("; template-origin:") != std::string::npos) {
        return;
      }
      std::ostringstream detail;
      detail << "return mirror for function '" << fn.name
             << "' names a different structured return type than "
             << expected_name;
      fail_verify(field, detail.str());
    }
    verify_direct_aggregate_signature_store_entry(
        mod, fn.return_type, expected_id, mirror, field);
    return;
  }

  if (mirror.str() == expected_name) {
    fail_verify(field, "known struct return type must carry matching StructNameId");
  }
}

void verify_function_signature_param_type_ref_mirror(
    const LirModule& mod,
    const LirFunction& fn,
    const LirTypeRef& mirror,
    const LirSignatureParam* param,
    size_t index) {
  constexpr std::string_view field = "LirFunction.signature_param_type_refs";
  verify_signature_type_ref_semantics(mod, mirror, field);

  if (!param) return;

  if (param->type.base == TB_VRM_REGISTER && param->type.ptr_level == 0 &&
      param->type.array_rank == 0) {
    const unsigned expected_width = static_cast<unsigned>(param->type.vrm_width);
    if (mirror.kind() != LirTypeKind::VrmRegister ||
        mirror.vrm_width() != expected_width) {
      std::ostringstream detail;
      detail << "parameter " << index << " mirror for function '" << fn.name
             << "' must preserve VRM register carrier width "
             << expected_width << "; mirror '" << mirror.str() << "'";
      fail_verify(field, detail.str());
    }
    return;
  }

  if (param->is_byval && mirror.str().find("byval(") == std::string::npos) {
    std::ostringstream detail;
    detail << "parameter " << index << " for function '" << fn.name
           << "' is marked byval but its type mirror lacks a byval ABI fragment";
    fail_verify(field, detail.str());
  }
  if (!param->is_byval && mirror.str().find("byval(") != std::string::npos) {
    std::ostringstream detail;
    detail << "parameter " << index << " for function '" << fn.name
           << "' has a byval type mirror but is not marked byval";
    fail_verify(field, detail.str());
  }

  const StructNameId expected_id =
      expected_direct_aggregate_signature_id(mod, param->type);
  const bool structured_aggregate_param =
      (param->type.base == TB_STRUCT || param->type.base == TB_UNION) &&
      param->type.ptr_level == 0 && param->type.array_rank == 0;
  if (expected_id == kInvalidStructName && structured_aggregate_param &&
      !mirror.has_struct_name_id()) {
    if (mirror.str().find("byval(") == std::string::npos) {
      std::ostringstream detail;
      detail << "parameter " << index << " mirror for function '" << fn.name
             << "' must carry a StructNameId or aggregate ABI fragment";
      fail_verify(field, detail.str());
    }
    return;
  }
  if (expected_id == kInvalidStructName) return;

  const std::string_view expected_name = mod.struct_names.spelling(expected_id);
  if (mirror.has_struct_name_id()) {
    if (mirror.struct_name_id() != expected_id) {
      if (fn.signature_text.find("; template-origin:") != std::string::npos) {
        return;
      }
      std::ostringstream detail;
      detail << "parameter " << index << " mirror for function '" << fn.name
             << "' names a different structured parameter type than "
             << expected_name;
      fail_verify(field, detail.str());
    }
    verify_direct_aggregate_signature_store_entry(
        mod, param->type, expected_id, mirror, field);
    return;
  }

  if (!aggregate_signature_param_mirror_matches_type(mirror, expected_name)) {
    std::ostringstream detail;
    detail << "parameter " << index << " mirror for function '" << fn.name
           << "' does not match structured aggregate parameter type "
           << expected_name << "; mirror '" << mirror.str() << "'";
    fail_verify(field, detail.str());
  }

  if (mirror.str() == expected_name) {
    std::ostringstream detail;
    detail << "known struct parameter " << index
           << " type must carry matching StructNameId";
    fail_verify(field, detail.str());
  }
}

bool plain_fixed_scalar_parameter(const TypeSpec& type) {
  const bool plain_base = [&] {
    switch (type.base) {
      case TB_INT:
      case TB_UINT:
      case TB_LONG:
      case TB_ULONG:
      case TB_LONGLONG:
      case TB_ULONGLONG:
      case TB_FLOAT:
      case TB_DOUBLE: return true;
      default: return false;
    }
  }();
  const auto compatibility_array_fact = [](long long value) {
    return value == -1 || value == 0;
  };
  return plain_base && type.enum_underlying_base == TB_VOID &&
         type.ptr_level == 0 && !type.is_lvalue_ref && !type.is_rvalue_ref &&
         type.align_bytes == 0 && compatibility_array_fact(type.array_size) &&
         type.array_rank == 0 &&
         std::all_of(std::begin(type.array_dims), std::end(type.array_dims),
                     compatibility_array_fact) &&
         !type.is_ptr_to_array &&
         (type.inner_rank == -1 || type.inner_rank == 0) &&
         !type.is_vector && type.vector_lanes == 0 &&
         type.vector_bytes == 0 && type.vrm_width == 0 &&
         type.array_size_expr == nullptr && !type.is_const &&
         !type.is_volatile && !type.is_fn_ptr && !type.is_packed &&
         !type.is_noinline && !type.is_always_inline &&
         type.tag_text_id == kInvalidText &&
         type.template_param_owner_namespace_context_id == -1 &&
         type.template_param_owner_text_id == kInvalidText &&
         type.template_param_index == -1 &&
         type.template_param_text_id == kInvalidText &&
         type.record_def == nullptr && type.qualifier_segments == nullptr &&
         type.qualifier_text_ids == nullptr &&
         type.n_qualifier_segments == 0 && !type.is_global_qualified &&
         type.namespace_context_id == -1 && type.tpl_struct_origin == nullptr &&
         type.tpl_struct_origin_key == c4c::QualifiedNameKey{} &&
         type.tpl_struct_args.data == nullptr &&
         type.tpl_struct_args.size == 0 &&
         type.deferred_member_type_owner_key == c4c::QualifiedNameKey{} &&
         type.deferred_member_type_name == nullptr &&
         type.deferred_member_type_text_id == kInvalidText;
}

bool direct_scalar_parameter_type(const TypeSpec& type) {
  switch (type.base) {
    case TB_INT:
    case TB_UINT:
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG:
    case TB_FLOAT:
    case TB_DOUBLE: break;
    default: return false;
  }
  // Keep scalar admission structural. Source alias/qualification metadata is
  // not parameter identity; the matching logical/signature bases and exact
  // typed mirror below remain the authority contract.
  return type.ptr_level == 0 && type.array_rank == 0 && !type.is_ptr_to_array &&
         !type.is_vector && !type.is_lvalue_ref && !type.is_rvalue_ref &&
         !type.is_fn_ptr;
}

bool same_plain_fixed_scalar_type(const TypeSpec& lhs,
                                  const TypeSpec& rhs) {
  return lhs.base == rhs.base &&
         lhs.enum_underlying_base == rhs.enum_underlying_base &&
         lhs.ptr_level == rhs.ptr_level &&
         lhs.is_lvalue_ref == rhs.is_lvalue_ref &&
         lhs.is_rvalue_ref == rhs.is_rvalue_ref &&
         lhs.align_bytes == rhs.align_bytes &&
         lhs.array_size == rhs.array_size &&
         lhs.array_rank == rhs.array_rank &&
         std::equal(std::begin(lhs.array_dims), std::end(lhs.array_dims),
                    std::begin(rhs.array_dims)) &&
         lhs.is_ptr_to_array == rhs.is_ptr_to_array &&
         lhs.inner_rank == rhs.inner_rank &&
         lhs.is_vector == rhs.is_vector &&
         lhs.vector_lanes == rhs.vector_lanes &&
         lhs.vector_bytes == rhs.vector_bytes &&
         lhs.vrm_width == rhs.vrm_width &&
         lhs.array_size_expr == rhs.array_size_expr &&
         lhs.is_const == rhs.is_const &&
         lhs.is_volatile == rhs.is_volatile &&
         lhs.is_fn_ptr == rhs.is_fn_ptr;
}

bool exact_plain_scalar_mirror(const LirModule& mod, const TypeSpec& type,
                               const LirTypeRef& mirror) {
  if (type.base == TB_FLOAT)
    return mirror.kind() == LirTypeKind::Floating && mirror.str() == "float";
  if (type.base == TB_DOUBLE)
    return mirror.kind() == LirTypeKind::Floating && mirror.str() == "double";
  unsigned expected_width = 64;
  if (type.base == TB_INT || type.base == TB_UINT) {
    expected_width = 32;
  } else if (type.base == TB_LONG || type.base == TB_ULONG) {
    expected_width = c4c::long_width_bits(mod.target_profile);
  }
  return mirror.kind() == LirTypeKind::Integer &&
         mirror.integer_bit_width() == expected_width &&
         mirror.str() == "i" + std::to_string(expected_width);
}

bool abi_expanded_logical_parameter(const TypeSpec& type) {
  switch (type.base) {
    case TB_STRUCT:
    case TB_UNION:
    case TB_COMPLEX_FLOAT:
    case TB_COMPLEX_DOUBLE:
    case TB_COMPLEX_LONGDOUBLE:
    case TB_COMPLEX_CHAR:
    case TB_COMPLEX_SCHAR:
    case TB_COMPLEX_UCHAR:
    case TB_COMPLEX_SHORT:
    case TB_COMPLEX_USHORT:
    case TB_COMPLEX_INT:
    case TB_COMPLEX_UINT:
    case TB_COMPLEX_LONG:
    case TB_COMPLEX_ULONG:
    case TB_COMPLEX_LONGLONG:
    case TB_COMPLEX_ULONGLONG: return true;
    default: return type.is_vector;
  }
}

void verify_plain_fixed_scalar_parameter_relationship(const LirModule& mod,
                                                      const LirFunction& fn) {
  if (fn.signature_is_variadic || fn.signature_has_void_param_list) return;
  const bool logical_plain =
      !fn.params.empty() &&
      std::all_of(fn.params.begin(), fn.params.end(), [](const auto& param) {
        return plain_fixed_scalar_parameter(param.second);
      });
  const bool signature_plain =
      !fn.signature_params.empty() &&
      std::all_of(fn.signature_params.begin(), fn.signature_params.end(),
                  [](const LirSignatureParam& param) {
                    return !param.is_byval &&
                           plain_fixed_scalar_parameter(param.type);
                  });
  const bool has_expanded_logical_shape =
      std::any_of(fn.params.begin(), fn.params.end(), [](const auto& param) {
        return abi_expanded_logical_parameter(param.second);
      });
  const bool enforce = logical_plain ||
                       (signature_plain && !has_expanded_logical_shape);
  if (!enforce) return;

  constexpr std::string_view field = "LirFunction.params";
  if (fn.params.size() != fn.signature_params.size() ||
      fn.params.size() != fn.signature_param_type_refs.size()) {
    std::ostringstream detail;
    detail << "plain fixed scalar function '" << fn.name
           << "' must have exact logical/signature/mirror counts; got "
           << fn.params.size() << "/" << fn.signature_params.size() << "/"
           << fn.signature_param_type_refs.size();
    fail_verify(field, detail.str());
  }
  for (std::size_t index = 0; index < fn.params.size(); ++index) {
    const auto& logical = fn.params[index].second;
    const auto& signature = fn.signature_params[index];
    const auto& mirror = fn.signature_param_type_refs[index];
    if (!plain_fixed_scalar_parameter(logical) || signature.is_byval ||
        !plain_fixed_scalar_parameter(signature.type) ||
        !same_plain_fixed_scalar_type(logical, signature.type) ||
        !exact_plain_scalar_mirror(mod, signature.type, mirror)) {
      std::ostringstream detail;
      detail << "plain fixed scalar parameter " << index << " of function '"
             << fn.name
             << "' has conflicting logical, signature, or typed-mirror authority";
      fail_verify(field, detail.str());
    }
  }
}

void verify_function_signature_structured_param_shape(const LirModule& mod,
                                                      const LirFunction& fn) {
  if (fn.signature_has_void_param_list) {
    if (!fn.signature_params.empty() || !fn.signature_param_type_refs.empty()) {
      fail_verify("LirFunction.signature_has_void_param_list",
                  "void parameter list must not carry fixed signature params");
    }
    if (fn.signature_is_variadic) {
      fail_verify("LirFunction.signature_is_variadic",
                  "void parameter list must not be variadic");
    }
    return;
  }

  verify_plain_fixed_scalar_parameter_relationship(mod, fn);

  if (!fn.signature_params.empty() &&
      fn.signature_param_type_refs.size() != fn.signature_params.size()) {
    std::ostringstream detail;
    detail << "function '" << fn.name << "' has "
           << fn.signature_param_type_refs.size()
           << " parameter type mirrors but "
           << fn.signature_params.size() << " structured signature params";
    fail_verify("LirFunction.signature_param_type_refs", detail.str());
  }
}

bool same_signature_store_type_fact(const LirTypeRef& lhs,
                                    const LirTypeRef& rhs) {
  return lhs == rhs && lhs.str() == rhs.str();
}

bool same_signature_store_type_fact(const std::optional<LirTypeRef>& lhs,
                                    const std::optional<LirTypeRef>& rhs) {
  if (lhs.has_value() != rhs.has_value()) return false;
  if (!lhs.has_value()) return true;
  return same_signature_store_type_fact(*lhs, *rhs);
}

bool same_signature_store_type_facts(const std::vector<LirTypeRef>& lhs,
                                     const std::vector<LirTypeRef>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (!same_signature_store_type_fact(lhs[index], rhs[index])) return false;
  }
  return true;
}

void verify_function_signature_store_ref(const LirModule& mod,
                                         const LirFunction& fn) {
  constexpr std::string_view field = "LirFunction.function_signature_ref";
  if (!fn.function_signature_ref.valid()) {
    if (mod.function_signature_store.empty()) return;
    std::ostringstream detail;
    detail << (fn.is_declaration ? "declaration" : "definition")
           << " function '" << fn.name
           << "' must reference a module-owned function signature";
    fail_verify(field, detail.str());
  }

  const LirFunctionSignatureStoreEntry* entry =
      mod.find_function_signature(fn.function_signature_ref);
  if (!entry) {
    std::ostringstream detail;
    detail << (fn.is_declaration ? "declaration" : "definition")
           << " function '" << fn.name
           << "' must reference a module-owned function signature";
    fail_verify(field, detail.str());
  }

  if (!same_signature_store_type_fact(entry->return_type_ref,
                                      fn.signature_return_type_ref) ||
      entry->return_ext_attr != fn.signature_return_ext_attr ||
      !same_signature_store_type_facts(entry->fixed_param_type_refs,
                                       fn.signature_param_type_refs) ||
      entry->is_variadic != fn.signature_is_variadic ||
      entry->has_void_param_list != fn.signature_has_void_param_list) {
    std::ostringstream detail;
    detail << "stored signature for function '" << fn.name
           << "' disagrees with structured signature mirrors";
    fail_verify(field, detail.str());
  }

  if (entry->fixed_param_is_byval.size() != fn.signature_params.size()) {
    std::ostringstream detail;
    detail << "stored signature for function '" << fn.name
           << "' has " << entry->fixed_param_is_byval.size()
           << " byval facts for " << fn.signature_params.size()
           << " structured params";
    fail_verify(field, detail.str());
  }
  for (std::size_t index = 0; index < entry->fixed_param_is_byval.size();
       ++index) {
    if (entry->fixed_param_is_byval[index] !=
        fn.signature_params[index].is_byval) {
      std::ostringstream detail;
      detail << "stored signature byval fact " << index
             << " for function '" << fn.name
             << "' disagrees with structured signature param";
      fail_verify(field, detail.str());
    }
  }
}

void verify_function_signature_type_ref_shadows(const LirModule& mod) {
  // Despite the historical "shadow" name, structured signature mirrors are the
  // authority here. signature_text is only checked as retained output spelling.
  for (const auto& fn : mod.functions) {
    const std::string_view line = function_signature_line(fn);
    if (line.empty()) {
      fail_verify("LirFunction.signature_text", "missing define/declare signature line");
    }

    verify_function_signature_structured_param_shape(mod, fn);
    verify_function_signature_store_ref(mod, fn);

    if (fn.signature_return_type_ref.has_value()) {
      verify_function_signature_return_type_ref_mirror(
          mod, fn, *fn.signature_return_type_ref);
    }

    for (size_t index = 0; index < fn.signature_param_type_refs.size(); ++index) {
      const LirSignatureParam* param =
          index < fn.signature_params.size() ? &fn.signature_params[index] : nullptr;
      verify_function_signature_param_type_ref_mirror(
          mod, fn, fn.signature_param_type_refs[index], param, index);
    }
  }
}

std::string join_layout_fields(const std::vector<std::string>& fields) {
  std::ostringstream out;
  out << "[";
  for (size_t index = 0; index < fields.size(); ++index) {
    if (index != 0) out << ", ";
    out << fields[index];
  }
  out << "]";
  return out.str();
}

void verify_structured_layout_observations(const LirModule& mod) {
  for (const auto& observation : mod.structured_layout_observations) {
    if (observation.site.empty()) {
      fail_verify("LirStructuredLayoutObservation.site", "must not be empty");
    }
    if (observation.type_name.empty()) {
      fail_verify("LirStructuredLayoutObservation.type_name", "must not be empty");
    }
    if (observation.name_id == kInvalidStructName) {
      fail_verify("LirStructuredLayoutObservation.name_id", "must be valid");
    }

    const std::string_view rendered_name = mod.struct_names.spelling(observation.name_id);
    if (rendered_name.empty()) {
      fail_verify("LirStructuredLayoutObservation.name_id",
                  "must resolve to a struct name");
    }
    if (rendered_name != observation.type_name) {
      std::ostringstream detail;
      detail << "StructNameId mirror '" << rendered_name
             << "' disagrees with observed type '" << observation.type_name << "'";
      fail_verify("LirStructuredLayoutObservation.type_name", detail.str());
    }

    if (!observation.parity_checked || observation.parity_matches) continue;

    std::ostringstream detail;
    detail << "structured layout mismatch at " << observation.site << " for "
           << observation.type_name << "; legacy size "
           << observation.legacy_size_bytes << ", legacy align "
           << observation.legacy_align_bytes << ", legacy fields "
           << join_layout_fields(observation.legacy_field_types)
           << ", structured fields "
           << join_layout_fields(observation.structured_field_types);
    fail_verify("LirStructuredLayoutObservation.parity", detail.str());
  }
}

}  // namespace

const std::string& require_operand_kind(
    const LirOperand& operand,
    std::string_view field,
    std::initializer_list<LirOperandKind> allowed_kinds,
  bool allow_empty) {
  if (operand.empty()) {
    if (operand.kind() == LirOperandKind::DirectConstant &&
        operand.value_id()) {
      // Function-owned direct constants intentionally have no display spelling;
      // their legal consumers resolve the value ID structurally.
    } else {
    if (allow_empty && !operand.has_authority()) return operand.str();
    fail_verify(field, "must not be empty");
    }
  }
  verify_operand_authority_kind(operand, field);
  if (!operand_kind_allowed(operand.kind(), allowed_kinds)) {
    std::ostringstream detail;
    detail << "expected operand kind mismatch for '" << operand.str()
           << "'; got " << operand_kind_name(operand.kind());
    fail_verify(field, detail.str());
  }
  return operand.str();
}

const std::string& require_type_ref(const LirTypeRef& type,
                                    std::string_view field,
                                    bool allow_void) {
  if (type.empty()) fail_verify(field, "must not be empty");
  if (const auto mismatch = type_ref_mismatch_detail(type); mismatch.has_value()) {
    fail_verify(field, *mismatch);
  }
  if (!allow_void && type.kind() == LirTypeKind::Void) {
    fail_verify(field, "void is not valid here");
  }
  return type.str();
}

std::string render_integer_type_ref(const LirTypeRef& type,
                                    std::string_view field) {
  if (type.empty()) fail_verify(field, "must not be empty");
  if (type.kind() != LirTypeKind::Integer) {
    fail_verify(field, "requires integer type authority");
  }
  const std::optional<unsigned> width = type.integer_bit_width();
  if (!width.has_value()) {
    fail_verify(field, "integer type authority requires a bit width");
  }
  return "i" + std::to_string(*width);
}

std::string render_floating_type_ref(const LirTypeRef& type,
                                     std::string_view field) {
  if (type.empty()) fail_verify(field, "must not be empty");
  if (type.kind() != LirTypeKind::Floating) {
    fail_verify(field, "requires floating type authority");
  }
  switch (type.builtin_type().value_or(LirBuiltinType::Void)) {
    case LirBuiltinType::Half: return "half";
    case LirBuiltinType::Float: return "float";
    case LirBuiltinType::Double: return "double";
    case LirBuiltinType::Fp128: return "fp128";
    case LirBuiltinType::X86Fp80: return "x86_fp80";
    default:
      fail_verify(field, "floating type authority requires a builtin type");
  }
}

std::string_view render_binary_opcode(const LirBinaryOpcodeRef& opcode,
                                      std::string_view field) {
  const auto typed = opcode.typed();
  if (!typed.has_value()) {
    fail_verify(field, "unknown typed binary opcode '" + opcode.str() + "'");
  }
  return opcode.str();
}

std::string_view render_cmp_predicate(const LirCmpPredicateRef& predicate,
                                      std::string_view field) {
  const auto typed = predicate.typed();
  if (!typed.has_value()) {
    fail_verify(field,
                "unknown typed comparison predicate '" + predicate.str() + "'");
  }
  return predicate.str();
}

void verify_module(const LirModule& mod) {
  verify_struct_decl_shadows(mod);
  verify_structured_layout_observations(mod);
  verify_extern_decl_shadows(mod);
  verify_global_type_ref_shadows(mod);
  verify_global_initializer_elements(mod);
  verify_function_signature_type_ref_shadows(mod);

  std::unordered_map<uint32_t, std::unordered_set<const LirFunction*>>
      instruction_result_owners;
  const auto record_result_owner = [&](const LirFunction& function,
                                       const LirOperand& result) {
    if (const LirValueId* id = result.value_id(); id && id->valid()) {
      instruction_result_owners[id->value].insert(&function);
    }
  };
  const auto record_instruction_result_owners = [&](const LirFunction& function,
                                                     const LirInst& inst) {
    if (const auto* inline_asm = std::get_if<LirInlineAsmOp>(&inst)) {
      for (const LirInlineAsmValueBinding& result : inline_asm->ordinary_results) {
        record_result_owner(function, result.value);
      }
      return;
    }
    if (const LirOperand* result = modeled_result_operand(inst)) {
      record_result_owner(function, *result);
    }
  };
  for (const auto& function : mod.functions) {
    for (const auto& inst : function.alloca_insts) {
      record_instruction_result_owners(function, inst);
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        record_instruction_result_owners(function, inst);
      }
    }
  }
  for (const auto& function : mod.functions) {
    verify_function_value_ownership(mod, function, instruction_result_owners);
    for (const auto& inst : function.alloca_insts) {
      verify_inst(mod, inst, &function);
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        verify_inst(mod, inst, &function);
        if (const auto* indirect_br = std::get_if<LirIndirectBrOp>(&inst)) {
          verify_indirect_br_successors(function, *indirect_br);
        }
      }
      verify_terminator(function, block.terminator);
    }
  }
}

}  // namespace c4c::codegen::lir
