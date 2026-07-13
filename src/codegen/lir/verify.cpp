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

void verify_call_callee_signature(const LirModule& mod, const LirCallOp& call) {
  if (!call.callee_signature.has_value()) return;

  const LirCallSignature& sig = *call.callee_signature;
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

  for (size_t index = 0; index < sig.fixed_param_type_refs.size(); ++index) {
    verify_call_arg_type_ref_mirror(
        mod, sig.fixed_param_type_refs[index], sig.fixed_param_types[index], index);
  }

  if (!sig.has_unspecified_params) {
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
    require_operand_kind(op->result, "LirCallOp.result",
                         {LirOperandKind::SsaValue}, true);
    verify_call_return_type_ref_mirror(mod, op->return_type);
    verify_pointer_operand(op->callee, "LirCallOp.callee");
    verify_call_callee_signature(mod, *op);
    if (!op->arg_type_refs.empty()) {
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
    if (op->result.empty() && op->return_type != "void") {
      fail_verify("LirCallOp.result",
                  "must hold an SSA result for non-void calls");
    }
    if (!op->result.empty() && op->return_type == "void") {
      fail_verify("LirCallOp.return_type",
                  "void calls must not carry a result operand");
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
    return;
  }
  if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    verify_result_operand(op->result, "LirCmpOp.result");
    (void)render_cmp_predicate(op->predicate, "LirCmpOp.predicate");
    require_module_type_ref(mod, op->type_str, "LirCmpOp.type_str");
    verify_value_operand(op->lhs, "LirCmpOp.lhs");
    verify_value_operand(op->rhs, "LirCmpOp.rhs");
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
    require_module_type_ref(mod, op->ret_type, "LirInlineAsmOp.ret_type", true);
    if (op->result.empty() && op->ret_type != "void") {
      fail_verify("LirInlineAsmOp.result",
                  "must hold an SSA result for non-void inline asm");
    }
    if (!op->result.empty() && op->ret_type == "void") {
      fail_verify("LirInlineAsmOp.ret_type",
                  "void inline asm must not carry a result operand");
    }

    const std::size_t semantic_constraint_count =
        count_inline_asm_constraints(op->original_constraint_text);
    if ((!op->ordinary_inputs.empty() || !op->ordinary_results.empty()) &&
        semantic_constraint_count == 0) {
      fail_verify("LirInlineAsmOp.original_constraint_text",
                  "structured values require original semantic constraints");
    }

    const auto verify_bindings =
        [&](const std::vector<LirInlineAsmValueBinding>& bindings,
            std::string_view field, LirInlineAsmValueRole ordinary_role) {
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
            if (binding.constraint_index >= semantic_constraint_count) {
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
                    LirInlineAsmValueRole::Output);

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

void verify_function_value_ownership(const LirFunction& function) {
  std::unordered_set<uint32_t> definitions;

  const auto collect_definition = [&](const LirInst& inst) {
    const LirOperand* result = modeled_result_operand(inst);
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
    }
  }
}

void verify_terminator(const LirTerminator& terminator) {
  if (const auto* cbr = std::get_if<LirCondBr>(&terminator)) {
    const LirOperand cond(cbr->cond_name);
    require_operand_kind(cond, "LirCondBr.cond_name",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::Immediate,
                          LirOperandKind::SpecialToken});
    return;
  }
  if (const auto* ret = std::get_if<LirRet>(&terminator)) {
    if (!ret->value_str.has_value() && !ret->type_str.empty() &&
        ret->type_str != "void") {
      fail_verify("LirRet.type_str",
                  "must be void when the return has no value");
    }
    if (ret->value_str.has_value() && ret->type_str.empty()) {
      fail_verify("LirRet.type_str",
                  "must not be empty when the return has a value");
    }
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

void verify_function_signature_structured_param_shape(const LirFunction& fn) {
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

    verify_function_signature_structured_param_shape(fn);

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
    verify_function_value_ownership(function);
    for (const auto& inst : function.alloca_insts) verify_inst(mod, inst);
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) verify_inst(mod, inst);
      verify_terminator(block.terminator);
    }
  }
}

}  // namespace c4c::codegen::lir
