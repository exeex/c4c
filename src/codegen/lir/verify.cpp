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
    expected = LirOperandKind::SsaValue;
  } else if (operand.link_name_id()) {
    expected = LirOperandKind::Global;
  } else if (operand.integer_immediate()) {
    expected = LirOperandKind::Immediate;
  } else {
    fail_verify(field, "unknown operand authority alternative");
  }

  if (operand.kind() != expected) {
    fail_verify(field,
                "authority alternative disagrees with stored operand kind");
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
  const auto classified_kind = type.empty() ? LirTypeKind::RawText
                                            : LirTypeRef(type.str()).kind();
  if (type.kind() != classified_kind) {
    if (type.has_struct_name_id() && type.kind() == LirTypeKind::Struct) {
      return std::nullopt;
    }
    std::ostringstream detail;
    detail << "typed kind disagrees with text '" << type.str() << "'";
    return detail.str();
  }

  if (type.kind() == LirTypeKind::Integer) {
    const auto typed_width = type.integer_bit_width();
    const auto text_width = LirTypeRef(type.str()).integer_bit_width();
    if (typed_width != text_width) {
      std::ostringstream detail;
      detail << "typed integer width ";
      if (typed_width.has_value()) {
        detail << *typed_width;
      } else {
        detail << "<missing>";
      }
      detail << " disagrees with text '" << type.str() << "'";
      return detail.str();
    }
  }

  if (type.kind() == LirTypeKind::VrmRegister) {
    const auto typed_width = type.vrm_width();
    const auto text_width = LirTypeRef(type.str()).vrm_width();
    if (typed_width != text_width) {
      std::ostringstream detail;
      detail << "typed VRM width ";
      if (typed_width.has_value()) {
        detail << *typed_width;
      } else {
        detail << "<missing>";
      }
      detail << " disagrees with text '" << type.str() << "'";
      return detail.str();
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
  return rendered;
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

void verify_known_struct_type_ref_mirror(const LirModule& mod,
                                         const LirTypeRef& type,
                                         std::string_view field,
                                         std::string_view type_role) {
  if (type.has_struct_name_id()) return;

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

bool is_direct_void_fixed_integer_signature_claim(const LirCallOp& call) {
  if (call.return_type.kind() != LirTypeKind::Void ||
      call.direct_callee_link_name_id == kInvalidLinkName ||
      !call.callee_signature.has_value()) {
    return false;
  }
  const LirCallSignature& signature = *call.callee_signature;
  return !signature.is_variadic && !signature.has_unspecified_params &&
         !signature.has_void_param_list &&
         signature.fixed_param_type_refs.size() == 1 &&
         signature.fixed_param_type_refs[0].kind() == LirTypeKind::Integer;
}

bool is_direct_void_fixed_integer_immediate_claim(const LirCallOp& call) {
  return is_direct_void_fixed_integer_signature_claim(call) &&
         !call.structured_args.empty() &&
         call.structured_args[0].operand.kind() == LirOperandKind::Immediate &&
         call.structured_args[0].type_ref.kind() == LirTypeKind::Integer &&
         call.arg_type_refs.size() == 1 &&
         call.arg_type_refs[0].kind() == LirTypeKind::Integer;
}

bool is_direct_void_fixed_integer_ssa_claim(const LirCallOp& call) {
  return is_direct_void_fixed_integer_signature_claim(call) &&
         !call.structured_args.empty() &&
         call.structured_args[0].operand.kind() == LirOperandKind::SsaValue &&
         call.structured_args[0].type_ref.kind() == LirTypeKind::Integer &&
         call.arg_type_refs.size() == 1 &&
         call.arg_type_refs[0].kind() == LirTypeKind::Integer;
}

bool has_complete_direct_void_integer_immediate_authority(
    const LirCallOp& call) {
  if (!is_direct_void_fixed_integer_immediate_claim(call)) return false;
  const LirCallSignature& signature = *call.callee_signature;
  return signature.return_type_ref.has_value() &&
         *signature.return_type_ref == call.return_type &&
         signature.fixed_param_types.size() == 1 &&
         call.structured_args.size() == 1 && call.arg_type_refs.size() == 1 &&
         call.structured_args[0].type_ref ==
             signature.fixed_param_type_refs[0] &&
         call.arg_type_refs[0] == signature.fixed_param_type_refs[0] &&
         call.structured_args[0].operand.integer_immediate() &&
         call.structured_args[0].ext_attr == LirExtAttr::None &&
         call.result.empty() && !call.result.has_authority();
}

bool has_complete_direct_void_integer_ssa_authority(const LirCallOp& call) {
  if (!is_direct_void_fixed_integer_ssa_claim(call)) return false;
  const LirCallSignature& signature = *call.callee_signature;
  return signature.return_type_ref.has_value() &&
         *signature.return_type_ref == call.return_type &&
         signature.fixed_param_types.size() == 1 &&
         call.structured_args.size() == 1 && call.arg_type_refs.size() == 1 &&
         call.structured_args[0].type_ref ==
             signature.fixed_param_type_refs[0] &&
         call.arg_type_refs[0] == signature.fixed_param_type_refs[0] &&
         call.structured_args[0].operand.value_id() &&
         call.structured_args[0].ext_attr == LirExtAttr::None &&
         call.result.empty() && !call.result.has_authority();
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
         call.callee_signature->fixed_param_types.empty() &&
         call.callee_signature->fixed_param_type_refs.empty() &&
         call.structured_args.empty() && call.arg_type_refs.empty();
}

void verify_direct_zero_arg_scalar_floating_result_call(const LirModule& mod,
                                                        const LirCallOp& call) {
  const bool direct_scalar_floating_result_authority =
      is_native_scalar_floating_type(call.return_type) &&
      call.callee.kind() == LirOperandKind::Global && call.result.value_id();
  if (!direct_scalar_floating_result_authority &&
      !is_direct_zero_arg_scalar_floating_result_claim(call)) {
    return;
  }

  if (!call.result.value_id()) {
    fail_verify("LirCallOp.result",
                "direct zero-argument scalar floating call requires LirValueId result authority");
  }
  if (!call.callee_signature.has_value() ||
      call.direct_callee_link_name_id == kInvalidLinkName ||
      call.callee_signature->is_variadic ||
      call.callee_signature->has_unspecified_params ||
      !call.callee_signature->has_void_param_list ||
      !call.callee_signature->fixed_param_types.empty() ||
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

void verify_call_callee_signature(const LirModule& mod, const LirCallOp& call,
                                  bool structured_authority_complete) {
  if (!call.callee_signature.has_value()) return;

  const LirCallSignature& sig = *call.callee_signature;
  const bool direct_integer_result_contract =
      call.return_type.kind() == LirTypeKind::Integer &&
      call.direct_callee_link_name_id != kInvalidLinkName;
  if (direct_integer_result_contract && !sig.return_type_ref.has_value()) {
    fail_verify("LirCallOp.callee_signature.return_type_ref",
                "structured direct integer call requires a return type ref");
  }
  if (sig.return_type_ref.has_value()) {
    verify_call_return_type_ref_mirror(mod, *sig.return_type_ref);
    if (*sig.return_type_ref != call.return_type) {
      fail_verify("LirCallOp.callee_signature.return_type_ref",
                  "structured callee return type must match call return type");
    }
  }

  if (sig.has_void_param_list) {
    if (!sig.fixed_param_types.empty() || !sig.fixed_param_type_refs.empty()) {
      fail_verify("LirCallOp.callee_signature",
                  "void parameter list must not carry fixed parameter mirrors");
    }
    if (sig.is_variadic) {
      fail_verify("LirCallOp.callee_signature",
                  "void parameter list must not be variadic");
    }
  }

  if (sig.fixed_param_types.size() != sig.fixed_param_type_refs.size()) {
    fail_verify("LirCallOp.callee_signature.fixed_param_type_refs",
                "fixed parameter type mirrors must match fixed parameter count");
  }

  if (!structured_authority_complete) {
    for (size_t index = 0; index < sig.fixed_param_type_refs.size(); ++index) {
      verify_call_arg_type_ref_mirror(
          mod, sig.fixed_param_type_refs[index], sig.fixed_param_types[index], index);
    }
  }

  if (!sig.has_unspecified_params && !structured_authority_complete) {
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

  if (!op.result.value_id()) return;
  if (op.kind == LirCastKind::FPToSI || op.kind == LirCastKind::FPToUI) {
    if (op.from_type.kind() != LirTypeKind::Floating ||
        op.to_type.kind() != LirTypeKind::Integer) {
      fail_verify(
          "LirCastOp.from_type",
          "authoritative floating-to-integer cast requires floating-to-integer endpoint type refs");
    }
    const std::string_view from_type = op.from_type.str();
    if (from_type != "half" && from_type != "float" &&
        from_type != "double" && from_type != "x86_fp80" &&
        from_type != "fp128") {
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
    const std::string_view to_type = op.to_type.str();
    if (to_type != "half" && to_type != "float" && to_type != "double" &&
        to_type != "x86_fp80" && to_type != "fp128") {
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
    const auto floating_width = [](const LirTypeRef& type)
        -> std::optional<unsigned> {
      if (type.str() == "half") return 16;
      if (type.str() == "float") return 32;
      if (type.str() == "double") return 64;
      if (type.str() == "x86_fp80") return 80;
      if (type.str() == "fp128") return 128;
      return std::nullopt;
    };
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

void verify_bin_op_authority(const LirBinOp& op) {
  if (!op.result.value_id()) return;
  const std::optional<LirBinaryOpcode> opcode = op.opcode.typed();
  if (!opcode) return;
  const bool floating_opcode = is_floating_binary_opcode(*opcode);
  const bool floating_type = op.type_str.kind() == LirTypeKind::Floating;
  if (floating_opcode != floating_type) {
    fail_verify("LirBinOp.type_str",
                "authoritative floating binary opcode and type must agree");
  }
  if (!floating_opcode && op.type_str.kind() == LirTypeKind::Integer) {
    const auto verify_integer_operand_authority = [&op](
        const LirOperand& operand, std::string_view field) {
      if (!operand.has_authority() || operand.value_id()) return;
      if (const LirIntegerImmediate* immediate = operand.integer_immediate()) {
        const std::optional<unsigned> width = op.type_str.integer_bit_width();
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

bool has_complete_integer_boolean_flag_call_authority(const LirCallOp& call) {
  if (!is_integer_boolean_flag_call_claim(call) ||
      (call.intrinsic_kind != LirIntrinsicKind::Cttz &&
       call.intrinsic_kind != LirIntrinsicKind::Ctlz) ||
      !call.result.value_id() ||
      call.return_type.kind() != LirTypeKind::Integer ||
      !call.callee.link_name_id() ||
      call.direct_callee_link_name_id != *call.callee.link_name_id() ||
      !call.callee_signature.has_value() ||
      !call.callee_signature->return_type_ref.has_value() ||
      !call.zero_count_behavior.has_value()) {
    return false;
  }
  const LirCallSignature& signature = *call.callee_signature;
  if (signature.is_variadic || signature.has_unspecified_params ||
      signature.has_void_param_list || signature.fixed_param_types.size() != 2 ||
      signature.fixed_param_type_refs.size() != 2 ||
      call.arg_type_refs.size() != 2 || call.structured_args.size() != 2) {
    return false;
  }
  const LirTypeRef& integer_type = call.return_type;
  const LirTypeRef i1_type = LirTypeRef::integer(1);
  const std::int64_t expected_flag =
      *call.zero_count_behavior == LirZeroCountBehavior::Defined ? 0 : 1;
  return *signature.return_type_ref == integer_type &&
         signature.fixed_param_type_refs[0] == integer_type &&
         signature.fixed_param_type_refs[1] == i1_type &&
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
  if (!has_complete_integer_boolean_flag_call_authority(call)) {
    fail_verify("LirCallOp",
                "authoritative integer boolean-flag call requires complete native callee/signature/argument authority");
  }
  const LinkNameId callee_id = *call.callee.link_name_id();
  if (callee_id == kInvalidLinkName || mod.link_names.spelling(callee_id).empty()) {
    fail_verify("LirCallOp.callee",
                "authoritative integer boolean-flag callee must resolve in the module");
  }
  const LirCallSignature& signature = *call.callee_signature;
  if (call.callee.kind() != LirOperandKind::Global ||
      !call.callee_type_suffix.empty() ||
      signature.fixed_param_types[0] != call.return_type.str() ||
      signature.fixed_param_types[1] != "i1" ||
      call.structured_args[0].type != call.return_type.str() ||
      call.structured_args[1].type != "i1" ||
      call.return_ext_attr != LirExtAttr::None ||
      signature.return_ext_attr != LirExtAttr::None ||
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

bool has_complete_integer_count_call_authority(const LirCallOp& call) {
  if (call.intrinsic_kind != LirIntrinsicKind::Ctpop ||
      call.zero_count_behavior.has_value() || !call.result.value_id() ||
      call.return_type.kind() != LirTypeKind::Integer ||
      !call.callee.link_name_id() ||
      call.direct_callee_link_name_id != *call.callee.link_name_id() ||
      !call.callee_signature.has_value() ||
      !call.callee_signature->return_type_ref.has_value()) {
    return false;
  }
  const LirCallSignature& signature = *call.callee_signature;
  if (signature.is_variadic || signature.has_unspecified_params ||
      signature.has_void_param_list || signature.fixed_param_types.size() != 1 ||
      signature.fixed_param_type_refs.size() != 1 ||
      call.arg_type_refs.size() != 1 || call.structured_args.size() != 1) {
    return false;
  }
  return *signature.return_type_ref == call.return_type &&
         signature.fixed_param_type_refs[0] == call.return_type &&
         call.arg_type_refs[0] == call.return_type &&
         call.structured_args[0].type_ref == call.return_type;
}

void verify_integer_count_call_authority(const LirModule& mod,
                                         const LirCallOp& call) {
  if (call.intrinsic_kind != LirIntrinsicKind::Ctpop) return;
  if (!has_complete_integer_count_call_authority(call)) {
    fail_verify("LirCallOp",
                "authoritative integer count call requires complete native callee/signature/argument authority and no zero-count behavior");
  }
  const LinkNameId callee_id = *call.callee.link_name_id();
  if (callee_id == kInvalidLinkName || mod.link_names.spelling(callee_id).empty()) {
    fail_verify("LirCallOp.callee",
                "authoritative integer count callee must resolve in the module");
  }
  const LirCallSignature& signature = *call.callee_signature;
  if (call.callee.kind() != LirOperandKind::Global ||
      !call.callee_type_suffix.empty() ||
      signature.fixed_param_types[0] != call.return_type.str() ||
      call.structured_args[0].type != call.return_type.str() ||
      call.return_ext_attr != LirExtAttr::None ||
      signature.return_ext_attr != LirExtAttr::None ||
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
  if (!is_direct_void_fixed_integer_signature_claim(call)) return;

  if (call.structured_args.size() == 1) {
    const LirOperand& operand = call.structured_args[0].operand;
    if (operand.has_authority() && !operand.integer_immediate() &&
        !operand.value_id()) {
      fail_verify("LirCallOp.structured_args.operand",
                  "fixed integer argument has the wrong authority alternative");
    }
  }
  if (!is_direct_void_fixed_integer_immediate_claim(call)) return;

  const LirCallSignature& signature = *call.callee_signature;
  if (!signature.return_type_ref.has_value() ||
      signature.return_type_ref->kind() != LirTypeKind::Void ||
      *signature.return_type_ref != call.return_type) {
    fail_verify("LirCallOp.callee_signature.return_type_ref",
                "direct void immediate call requires exact void return type authority");
  }
  if (signature.fixed_param_types.size() != 1) {
    fail_verify("LirCallOp.callee_signature.fixed_param_types",
                "direct void immediate call requires one fixed parameter");
  }
  if (call.structured_args.size() != 1 || call.arg_type_refs.size() != 1) {
    fail_verify("LirCallOp.structured_args",
                "direct void immediate call requires one typed structured argument");
  }

  const LirTypeRef& parameter_type = signature.fixed_param_type_refs[0];
  const LirCallArg& argument = call.structured_args[0];
  require_module_type_ref(mod, parameter_type,
                          "LirCallOp.callee_signature.fixed_param_type_refs");
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
  if (!is_direct_void_fixed_integer_ssa_claim(call)) return;

  const LirCallSignature& signature = *call.callee_signature;
  if (!signature.return_type_ref.has_value() ||
      signature.return_type_ref->kind() != LirTypeKind::Void ||
      *signature.return_type_ref != call.return_type) {
    fail_verify("LirCallOp.callee_signature.return_type_ref",
                "direct void SSA call requires exact void return type authority");
  }
  if (signature.fixed_param_types.size() != 1) {
    fail_verify("LirCallOp.callee_signature.fixed_param_types",
                "direct void SSA call requires one fixed parameter");
  }
  if (call.structured_args.size() != 1 || call.arg_type_refs.size() != 1) {
    fail_verify("LirCallOp.structured_args",
                "direct void SSA call requires one typed structured argument");
  }

  const LirTypeRef& parameter_type = signature.fixed_param_type_refs[0];
  const LirCallArg& argument = call.structured_args[0];
  require_module_type_ref(mod, parameter_type,
                          "LirCallOp.callee_signature.fixed_param_type_refs");
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
  if (op.ptr.kind() != LirOperandKind::Global || !op.ptr.link_name_id()) {
    fail_verify("LirGepOp.ptr",
                "authoritative GEP requires global LinkNameId base authority");
  }
  verify_global_pointer_owner(mod, op.ptr, "LirGepOp.ptr", "GEP");
  if (op.indices.empty()) {
    fail_verify("LirGepOp.indices",
                "authoritative GEP requires at least one index");
  }

  for (const LirGepIndex& index : op.indices) {
    if (!index.is_authoritative()) {
      fail_verify("LirGepOp.indices",
                  "authoritative GEP cannot mix raw compatibility indices");
    }
    require_module_type_ref(mod, index.type_ref(),
                            "LirGepOp.indices.type");
    if (index.type_ref().kind() != LirTypeKind::Integer) {
      fail_verify("LirGepOp.indices.type",
                  "authoritative GEP index type must be integer");
    }

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

void verify_inst(const LirModule& mod, const LirInst& inst) {
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
    return;
  }
  if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    verify_pointer_operand(op->saved_ptr, "LirStackRestoreOp.saved_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    verify_result_operand(op->result, "LirAbsOp.result");
    verify_value_operand(op->arg, "LirAbsOp.arg");
    require_module_type_ref(mod, op->int_type, "LirAbsOp.int_type");
    verify_abs_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    verify_pointer_operand(op->addr, "LirIndirectBrOp.addr");
    if (op->targets.empty()) {
      fail_verify("LirIndirectBrOp.targets", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    verify_result_operand(op->result, "LirExtractValueOp.result");
    require_module_type_ref(mod, op->agg_type, "LirExtractValueOp.agg_type");
    verify_value_operand(op->agg, "LirExtractValueOp.agg");
    return;
  }
  if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertValueOp.result");
    require_module_type_ref(mod, op->agg_type, "LirInsertValueOp.agg_type");
    verify_value_operand(op->agg, "LirInsertValueOp.agg");
    require_module_type_ref(mod, op->elem_type, "LirInsertValueOp.elem_type");
    verify_value_operand(op->elem, "LirInsertValueOp.elem");
    return;
  }
  if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    verify_result_operand(op->result, "LirLoadOp.result");
    require_module_type_ref(mod, op->type_str, "LirLoadOp.type_str", true);
    verify_pointer_operand(op->ptr, "LirLoadOp.ptr");
    verify_global_load_authority(mod, *op);
    return;
  }
  if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    require_module_type_ref(mod, op->type_str, "LirStoreOp.type_str", true);
    verify_value_operand(op->val, "LirStoreOp.val");
    verify_pointer_operand(op->ptr, "LirStoreOp.ptr");
    verify_global_store_authority(mod, *op);
    return;
  }
  if (const auto* op = std::get_if<LirCastOp>(&inst)) {
    verify_result_operand(op->result, "LirCastOp.result");
    require_module_type_ref(mod, op->from_type, "LirCastOp.from_type");
    verify_value_operand(op->operand, "LirCastOp.operand");
    require_module_type_ref(mod, op->to_type, "LirCastOp.to_type");
    verify_cast_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    verify_result_operand(op->result, "LirGepOp.result");
    require_module_type_ref(mod, op->element_type, "LirGepOp.element_type");
    verify_pointer_operand(op->ptr, "LirGepOp.ptr");
    verify_authoritative_gep(mod, *op);
    return;
  }
  if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    const bool structured_authority_complete =
        has_complete_direct_void_integer_immediate_authority(*op) ||
        has_complete_direct_void_integer_ssa_authority(*op) ||
        has_complete_integer_boolean_flag_call_authority(*op) ||
        has_complete_integer_count_call_authority(*op);
    require_operand_kind(op->result, "LirCallOp.result",
                         {LirOperandKind::SsaValue}, true);
    verify_call_return_type_ref_mirror(mod, op->return_type);
    verify_pointer_operand(op->callee, "LirCallOp.callee");
    verify_call_callee_signature(mod, *op, structured_authority_complete);
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
    verify_direct_zero_arg_scalar_floating_result_call(mod, *op);
    verify_integer_boolean_flag_call_authority(mod, *op);
    verify_integer_count_call_authority(mod, *op);
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
    require_module_type_ref(mod, op->type_str, "LirBinOp.type_str", true);
    verify_value_operand(op->lhs, "LirBinOp.lhs");
    if (op->rhs.empty()) {
      if (op->opcode.typed() != LirBinaryOpcode::FNeg) {
        fail_verify("LirBinOp.rhs",
                    "must not be empty for non-unary binary instructions");
      }
    } else {
      verify_value_operand(op->rhs, "LirBinOp.rhs");
    }
    verify_bin_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    verify_result_operand(op->result, "LirCmpOp.result");
    (void)render_cmp_predicate(op->predicate, "LirCmpOp.predicate");
    require_module_type_ref(mod, op->type_str, "LirCmpOp.type_str");
    verify_value_operand(op->lhs, "LirCmpOp.lhs");
    verify_value_operand(op->rhs, "LirCmpOp.rhs");
    verify_cmp_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    verify_result_operand(op->result, "LirPhiOp.result");
    require_module_type_ref(mod, op->type_str, "LirPhiOp.type_str");
    if (op->incoming.empty()) {
      fail_verify("LirPhiOp.incoming", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    verify_result_operand(op->result, "LirSelectOp.result");
    require_module_type_ref(mod, op->type_str, "LirSelectOp.type_str");
    verify_value_operand(op->cond, "LirSelectOp.cond");
    verify_value_operand(op->true_val, "LirSelectOp.true_val");
    verify_value_operand(op->false_val, "LirSelectOp.false_val");
    verify_select_op_authority(*op);
    return;
  }
  if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertElementOp.result");
    require_module_type_ref(mod, op->vec_type, "LirInsertElementOp.vec_type");
    verify_value_operand(op->vec, "LirInsertElementOp.vec");
    require_module_type_ref(mod, op->elem_type, "LirInsertElementOp.elem_type");
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
    require_module_type_ref(mod, op->vec_type, "LirShuffleVectorOp.vec_type");
    verify_value_operand(op->vec1, "LirShuffleVectorOp.vec1");
    verify_value_operand(op->vec2, "LirShuffleVectorOp.vec2");
    require_module_type_ref(mod, op->mask_type, "LirShuffleVectorOp.mask_type");
    verify_value_operand(op->mask, "LirShuffleVectorOp.mask");
    return;
  }
  if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    verify_result_operand(op->result, "LirVaArgOp.result");
    verify_pointer_operand(op->ap_ptr, "LirVaArgOp.ap_ptr");
    require_module_type_ref(mod, op->type_str, "LirVaArgOp.type_str");
    return;
  }
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    verify_result_operand(op->result, "LirAllocaOp.result");
    require_module_type_ref(mod, op->type_str, "LirAllocaOp.type_str");
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
                                     const LirFunction& function) {
  std::unordered_set<uint32_t> definitions;
  std::unordered_map<uint32_t, const LirInst*> definition_insts;

  verify_selected_memcpy_pointer_authority(mod, function, definitions,
                                           definition_insts);
  verify_selected_memcpy_authority(function);

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
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        const LirValueId* value_id = store->val.value_id();
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
  if (const auto* br = std::get_if<LirBr>(&terminator)) {
    if (!br->successor.valid()) {
      fail_verify("LirBr.successor", "must carry a valid current-function LirBlockId");
    }
    const auto destination = std::find_if(
        function.blocks.begin(), function.blocks.end(), [&](const LirBlock& block) {
          return block.id == br->successor;
        });
    if (destination == function.blocks.end() ||
        std::count_if(function.blocks.begin(), function.blocks.end(),
                      [&](const LirBlock& block) { return block.id == br->successor; }) != 1) {
      fail_verify("LirBr.successor", "must identify exactly one current-function block");
    }
    if (br->target_label != destination->label) {
      fail_verify("LirBr.target_label",
                  "display label must match the successor-selected destination");
    }
    return;
  }
  if (const auto* cbr = std::get_if<LirCondBr>(&terminator)) {
    const LirOperand cond(cbr->cond_name);
    require_operand_kind(cond, "LirCondBr.cond_name",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::Immediate,
                          LirOperandKind::SpecialToken});
    return;
  }
  if (const auto* ret = std::get_if<LirRet>(&terminator)) {
    require_type_ref(ret->type_str, "LirRet.type_str", true);
    if (ret->type_str.kind() == LirTypeKind::Void) {
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
    if (!value.has_authority()) return;

    if (ret->type_str.kind() != LirTypeKind::Integer) {
      fail_verify("LirRet.type_str",
                  "authoritative scalar return requires integer type");
    }
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
    if (sw->selector_name.empty() || sw->selector_type.empty()) {
      fail_verify("LirSwitch", "must carry both selector name and selector type");
    }
    return;
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

        if (const auto mismatch =
                type_ref_struct_name_mismatch_detail(mod.struct_names,
                                                     decl.return_type);
            mismatch.has_value()) {
          fail_verify("LirExternDecl.return_type", *mismatch);
        }
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
    if (!global.llvm_type_ref.has_value()) continue;
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

      const StructNameId global_struct_name_id =
          find_declared_struct_name_id(mod, global.llvm_type);
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

      if (const auto mismatch =
              type_ref_struct_name_mismatch_detail(mod.struct_names, mirror);
          mismatch.has_value()) {
        fail_verify("LirGlobal.llvm_type_ref", *mismatch);
      }
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
      !mod.link_name_texts) {
    return kInvalidStructName;
  }
  const std::string_view tag = mod.link_name_texts->lookup(type.tag_text_id);
  if (tag.empty()) return kInvalidStructName;
  const std::string rendered =
      tag.rfind("%struct.", 0) == 0 || tag.rfind("%\"struct.", 0) == 0
          ? std::string(tag)
          : c4c::codegen::llvm_helpers::llvm_struct_type_str(std::string(tag));
  return mod.struct_names.find(rendered);
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
      std::ostringstream detail;
      detail << "return mirror for function '" << fn.name
             << "' names a different structured return type than "
             << expected_name;
      fail_verify(field, detail.str());
    }
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
      std::ostringstream detail;
      detail << "parameter " << index << " mirror for function '" << fn.name
             << "' names a different structured parameter type than "
             << expected_name;
      fail_verify(field, detail.str());
    }
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

void verify_function_signature_type_ref_shadows(const LirModule& mod) {
  // Despite the historical "shadow" name, structured signature mirrors are the
  // authority here. signature_text is only checked as retained output spelling.
  for (const auto& fn : mod.functions) {
    const std::string_view line = function_signature_line(fn);
    if (line.empty()) {
      fail_verify("LirFunction.signature_text", "missing define/declare signature line");
    }

    verify_function_signature_structured_param_shape(mod, fn);

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
    if (allow_empty && !operand.has_authority()) return operand.str();
    fail_verify(field, "must not be empty");
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
  verify_function_signature_type_ref_shadows(mod);
  for (const auto& function : mod.functions) {
    verify_function_value_ownership(mod, function);
    for (const auto& inst : function.alloca_insts) verify_inst(mod, inst);
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) verify_inst(mod, inst);
      verify_terminator(function, block.terminator);
    }
  }
}

}  // namespace c4c::codegen::lir
