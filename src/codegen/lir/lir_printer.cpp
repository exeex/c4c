#include "ir.hpp"
#include "call_args_ops.hpp"
#include "../shared/llvm_helpers.hpp"

#include <cctype>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace c4c::codegen::lir {

namespace {

std::string llvm_global_sym(const std::string& raw) {
  return "@" + c4c::codegen::llvm_helpers::quote_llvm_ident(raw);
}

std::string_view resolve_link_name(const c4c::LinkNameTable& link_names,
                                   LinkNameId id) {
  if (id == c4c::kInvalidLinkName) return {};
  const std::string_view spelling = link_names.spelling(id);
  return spelling.empty() ? std::string_view{} : spelling;
}

std::string render_signature_with_link_name(std::string_view signature_text,
                                            std::string_view resolved_name) {
  if (resolved_name.empty()) return std::string(signature_text);

  // signature_text is retained final LLVM/output header spelling. The
  // function's LinkNameId is the semantic identity; this string replacement is
  // the compatibility rendering bridge until the header itself is fully
  // structured.
  const std::size_t header_pos = signature_text.rfind("define ");
  const std::size_t decl_pos = signature_text.rfind("declare ");
  const std::size_t start_pos =
      header_pos == std::string_view::npos ? decl_pos
                                           : (decl_pos == std::string_view::npos
                                                  ? header_pos
                                                  : std::max(header_pos, decl_pos));
  if (start_pos == std::string_view::npos) return std::string(signature_text);

  const std::size_t at_pos = signature_text.find('@', start_pos);
  if (at_pos == std::string_view::npos) return std::string(signature_text);

  const std::size_t paren_pos = signature_text.find('(', at_pos);
  if (paren_pos == std::string_view::npos) return std::string(signature_text);

  std::string rendered(signature_text);
  rendered.replace(at_pos, paren_pos - at_pos, llvm_global_sym(std::string(resolved_name)));
  return rendered;
}

std::string_view render_ext_attr(LirExtAttr attr);

std::string render_type_ref_for_signature(const LirTypeRef& type) {
  return type.str();
}

std::string render_global_type(const LirModule& mod, const LirGlobal& global) {
  if (!global.llvm_type_ref.has_value()) return global.llvm_type;
  const LirTypeRef& type = *global.llvm_type_ref;
  if (type.has_struct_name_id()) {
    const std::string_view name = mod.struct_names.spelling(type.struct_name_id());
    if (!name.empty()) return std::string(name);
  }
  return type.render_llvm();
}

std::string render_extern_return_type(const LirModule& mod,
                                      const LirExternDecl& decl) {
  if (decl.return_type.empty()) return decl.return_type_str;
  if (decl.return_type.has_struct_name_id()) {
    const std::string_view name =
        mod.struct_names.spelling(decl.return_type.struct_name_id());
    if (!name.empty()) return std::string(name);
  }
  return decl.return_type.render_llvm();
}

std::string render_signature_store_param(const LirTypeRef& type,
                                         bool is_byval) {
  if (!is_byval) return render_type_ref_for_signature(type);
  return "ptr byval(" + render_type_ref_for_signature(type) + ")";
}

std::optional<std::string> render_selected_extern_param_list_from_store(
    const LirModule& mod,
    const LirExternDecl& decl) {
  if (!decl.function_signature_ref.valid()) return std::nullopt;
  const LirFunctionSignatureStoreEntry* signature =
      mod.find_function_signature(decl.function_signature_ref);
  if (!signature ||
      signature->fixed_param_is_byval.size() !=
          signature->fixed_param_type_refs.size()) {
    return std::nullopt;
  }

  bool has_selected_aggregate_byval = false;
  for (std::size_t index = 0; index < signature->fixed_param_type_refs.size();
       ++index) {
    if (signature->fixed_param_is_byval[index] &&
        signature->fixed_param_type_refs[index].has_struct_name_id()) {
      has_selected_aggregate_byval = true;
      break;
    }
  }
  if (!has_selected_aggregate_byval) return std::nullopt;

  std::ostringstream out;
  out << "(";
  bool need_comma = false;
  if (!signature->has_void_param_list) {
    for (std::size_t index = 0; index < signature->fixed_param_type_refs.size();
         ++index) {
      if (need_comma) out << ", ";
      out << render_signature_store_param(signature->fixed_param_type_refs[index],
                                          signature->fixed_param_is_byval[index]);
      need_comma = true;
    }
    if (signature->is_variadic) {
      if (need_comma) out << ", ";
      out << "...";
    }
  }
  out << ")";
  return out.str();
}

std::string render_phi_boundary_type_ref(const LirPhiOp& op) {
  if (!op.boundary_value_type) {
    throw LirVerifyError(LirVerifyErrorKind::Malformed,
                         "LirPhiOp.boundary_value_type is required for PHI rendering");
  }
  const auto selected =
      LirPhiBoundaryValueType::from_type_ref(op.boundary_value_type->type);
  if (!selected || selected->kind != op.boundary_value_type->kind ||
      op.boundary_value_type->type != op.type_str) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirPhiOp.boundary_value_type must mirror LirPhiOp.type_str for PHI rendering");
  }
  if (op.boundary_value_type->kind == LirPhiBoundaryValueKind::Scalar) {
    if (op.boundary_value_type->type.kind() == LirTypeKind::Integer) {
      return render_integer_type_ref(op.boundary_value_type->type,
                                     "LirPhiOp.boundary_value_type");
    }
    if (op.boundary_value_type->type.kind() == LirTypeKind::Floating) {
      return render_floating_type_ref(op.boundary_value_type->type,
                                      "LirPhiOp.boundary_value_type");
    }
  }
  return require_type_ref(op.boundary_value_type->type,
                          "LirPhiOp.boundary_value_type");
}

std::string render_cast_endpoint_type_ref(const LirTypeRef& type,
                                          std::string_view field) {
  if (type.kind() == LirTypeKind::Integer) {
    return render_integer_type_ref(type, field);
  }
  if (type.kind() == LirTypeKind::Floating) {
    return render_floating_type_ref(type, field);
  }
  return require_type_ref(type, field);
}

std::optional<std::pair<std::string, std::string>>
render_required_insert_element_types_from_store(const LirModule& mod,
                                                const LirInsertElementOp& op) {
  if (!op.requires_native_vector_authority || !op.native_vector_authority ||
      !op.native_vector_authority->vector_ref) {
    return std::nullopt;
  }
  const LirVectorStoreEntry* vector =
      mod.find_vector(*op.native_vector_authority->vector_ref);
  if (!vector || vector->lane_count == 0 || vector->element_type.empty()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirInsertElementOp.native_vector_authority.vector_ref must reference a complete vector store fact");
  }
  const std::string elem_type =
      require_type_ref(vector->element_type,
                       "LirInsertElementOp.native_vector_authority.vector_ref.element_type");
  return std::pair<std::string, std::string>{
      "<" + std::to_string(vector->lane_count) + " x " + elem_type + ">",
      elem_type};
}

std::optional<std::string>
render_extract_element_vector_type_from_store(const LirModule& mod,
                                              const LirExtractElementOp& op) {
  if (!op.native_vector_authority || !op.native_vector_authority->vector_ref) {
    return std::nullopt;
  }
  const LirVectorStoreEntry* vector =
      mod.find_vector(*op.native_vector_authority->vector_ref);
  if (!vector || vector->lane_count == 0 || vector->element_type.empty()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirExtractElementOp.native_vector_authority.vector_ref must reference a complete vector store fact");
  }
  const std::string elem_type =
      require_type_ref(vector->element_type,
                       "LirExtractElementOp.native_vector_authority.vector_ref.element_type");
  return "<" + std::to_string(vector->lane_count) + " x " + elem_type + ">";
}

std::optional<std::pair<std::string, std::string>>
render_required_shuffle_vector_types_from_store(const LirModule& mod,
                                                const LirShuffleVectorOp& op) {
  if (!op.requires_native_vector_authority || !op.native_vector_authority ||
      !op.native_vector_authority->vector_ref) {
    return std::nullopt;
  }
  const LirVectorStoreEntry* vector =
      mod.find_vector(*op.native_vector_authority->vector_ref);
  if (!vector || vector->lane_count == 0 || vector->element_type.empty()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirShuffleVectorOp.native_vector_authority.vector_ref must reference a complete vector store fact");
  }
  const std::string elem_type =
      require_type_ref(vector->element_type,
                       "LirShuffleVectorOp.native_vector_authority.vector_ref.element_type");
  return std::pair<std::string, std::string>{
      "<" + std::to_string(vector->lane_count) + " x " + elem_type + ">",
      "<" + std::to_string(vector->lane_count) + " x i32>"};
}

std::string render_extract_value_aggregate_type(const LirExtractValueOp& op) {
  if (!op.requires_native_result_authority) {
    return require_type_ref(op.agg_type, "LirExtractValueOp.agg_type");
  }
  const auto* fields = op.agg_type.anonymous_struct_field_types();
  if (!fields || fields->empty()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirExtractValueOp.agg_type requires ordered native field types for native aggregate rendering");
  }
  return op.agg_type.render_llvm();
}

std::pair<std::string, std::string> render_insert_value_native_types(
    const LirInsertValueOp& op) {
  if (!op.requires_native_result_authority) {
    return {require_type_ref(op.agg_type, "LirInsertValueOp.agg_type"),
            require_type_ref(op.elem_type, "LirInsertValueOp.elem_type")};
  }
  if (!op.aggregate_result_type) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirInsertValueOp.aggregate_result_type requires native aggregate authority for native aggregate rendering");
  }
  const auto* fields = op.aggregate_result_type->anonymous_struct_field_types();
  if (!fields || fields->empty()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirInsertValueOp.aggregate_result_type requires ordered native field types for native aggregate rendering");
  }
  if (op.index < 0 || static_cast<std::size_t>(op.index) >= fields->size()) {
    throw LirVerifyError(
        LirVerifyErrorKind::Malformed,
        "LirInsertValueOp.index must select an aggregate field for native aggregate rendering");
  }
  return {op.aggregate_result_type->render_llvm(),
          (*fields)[static_cast<std::size_t>(op.index)].render_llvm()};
}

std::string_view signature_header_line(const LirFunction& function) {
  std::string_view signature = function.signature_text;
  while (!signature.empty()) {
    const std::size_t line_end = signature.find('\n');
    const std::string_view line =
        line_end == std::string_view::npos ? signature : signature.substr(0, line_end);
    if (line.rfind(function.is_declaration ? "declare " : "define ", 0) == 0) {
      return line;
    }
    if (line_end == std::string_view::npos) break;
    signature.remove_prefix(line_end + 1);
  }
  return {};
}

std::string signature_prefix_comments(const LirFunction& function) {
  if (function.is_declaration) return {};
  std::string_view signature = function.signature_text;
  std::string prefix;
  while (!signature.empty()) {
    const std::size_t line_end = signature.find('\n');
    const std::string_view line =
        line_end == std::string_view::npos ? signature : signature.substr(0, line_end);
    if (line.rfind("define ", 0) == 0) break;
    if (!line.empty() && line.front() == ';') {
      prefix.append(line);
      prefix.push_back('\n');
    }
    if (line_end == std::string_view::npos) break;
    signature.remove_prefix(line_end + 1);
  }
  return prefix;
}

std::string_view signature_suffix_after_param_list(const LirFunction& function) {
  const std::string_view line = signature_header_line(function);
  const std::size_t open_paren = line.find('(');
  if (open_paren == std::string_view::npos) return {};
  const std::size_t close_paren = line.rfind(')');
  if (close_paren == std::string_view::npos || close_paren < open_paren) return {};
  return line.substr(close_paren + 1);
}

std::optional<std::string> render_function_signature_from_store(
    const LirModule& mod,
    const LirFunction& function,
    std::string_view resolved_name) {
  if (!function.function_signature_ref.valid()) {
    return std::nullopt;
  }
  const LirFunctionSignatureStoreEntry* signature =
      mod.find_function_signature(function.function_signature_ref);
  if (!signature || !signature->return_type_ref.has_value()) {
    return std::nullopt;
  }

  std::ostringstream out;
  out << signature_prefix_comments(function);
  out << (function.is_declaration ? "declare " : "define ")
      << (function.is_internal ? "internal " : "")
      << render_ext_attr(signature->return_ext_attr)
      << render_type_ref_for_signature(*signature->return_type_ref) << " "
      << llvm_global_sym(std::string(resolved_name.empty() ? function.name
                                                           : resolved_name))
      << "(";
  bool need_comma = false;
  if (!signature->has_void_param_list) {
    for (std::size_t index = 0; index < signature->fixed_param_type_refs.size();
         ++index) {
      if (need_comma) out << ", ";
      out << render_type_ref_for_signature(signature->fixed_param_type_refs[index]);
      if (!function.is_declaration) {
        if (index >= function.signature_params.size()) return std::nullopt;
        out << " " << function.signature_params[index].name;
      }
      need_comma = true;
    }
    if (signature->is_variadic) {
      if (need_comma) out << ", ";
      out << "...";
    }
  }
  out << ")";
  if (!function.is_declaration) {
    out << signature_suffix_after_param_list(function);
  }
  return out.str();
}

std::string resolve_direct_call_callee(const LirCallOp& call,
                                       const c4c::LinkNameTable& link_names) {
  if (!parse_lir_direct_global_callee(call.callee.str()).has_value()) {
    return call.callee.str();
  }

  const std::string_view resolved_name =
      resolve_link_name(link_names, call.direct_callee_link_name_id);
  if (resolved_name.empty()) {
    return call.callee.str();
  }
  return llvm_global_sym(std::string(resolved_name));
}

std::optional<std::string> render_call_signature_suffix_from_store(
    const LirModule& mod,
    LirFunctionSignatureRef signature_ref) {
  if (!signature_ref.valid()) return std::nullopt;
  const LirFunctionSignatureStoreEntry* signature =
      mod.find_function_signature(signature_ref);
  if (!signature) return std::nullopt;

  std::ostringstream out;
  out << "(";
  bool need_comma = false;
  if (!signature->has_void_param_list) {
    for (const LirTypeRef& param_type : signature->fixed_param_type_refs) {
      if (need_comma) out << ", ";
      out << render_type_ref_for_signature(param_type);
      need_comma = true;
    }
    if (signature->is_variadic) {
      if (need_comma) out << ", ";
      out << "...";
    }
  }
  out << ")";
  return out.str();
}

std::string resolve_extern_decl_name(const LirExternDecl& decl,
                                     const c4c::LinkNameTable& link_names) {
  const std::string_view resolved_name =
      resolve_link_name(link_names, decl.link_name_id);
  return resolved_name.empty() ? decl.name : std::string(resolved_name);
}

std::string_view render_ext_attr(LirExtAttr attr) {
  switch (attr) {
    case LirExtAttr::SignExt:
      return "signext ";
    case LirExtAttr::ZeroExt:
      return "zeroext ";
    case LirExtAttr::None:
      return {};
  }
  return {};
}

// Render a single LirInst to text.
std::string resolve_direct_label_address(const LirFunction& function,
                                         const LirOperand& operand,
                                         const c4c::LinkNameTable& link_names) {
  const auto* value = operand.value_id();
  if (operand.kind() != LirOperandKind::DirectConstant || !value) return {};
  const auto constant = std::find_if(
      function.direct_label_address_constants.begin(),
      function.direct_label_address_constants.end(), [&](const auto& item) {
        return item.value == *value;
      });
  if (constant == function.direct_label_address_constants.end()) return {};
  const auto block = std::find_if(function.blocks.begin(), function.blocks.end(),
                                  [&](const auto& item) {
                                    return item.id == constant->target;
                                  });
  const std::string_view owner = resolve_link_name(link_names, constant->owner);
  if (block == function.blocks.end() || owner.empty()) return {};
  return "blockaddress(" + llvm_global_sym(std::string(owner)) + ", %" +
         block->label + ")";
}

void render_inst(std::ostringstream& os, const LirModule& mod,
                 const LirFunction& function, const LirInst& inst,
                 const c4c::LinkNameTable& link_names) {
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    const auto& result =
        require_operand_kind(op->result, "LirAllocaOp.result",
                             {LirOperandKind::SsaValue});
    const auto type = op->local_object_authority &&
                              op->type_str.kind() == LirTypeKind::Integer
                          ? render_integer_type_ref(op->type_str,
                                                    "LirAllocaOp.type_str")
                          : require_type_ref(op->type_str,
                                             "LirAllocaOp.type_str");
    os << "  " << result << " = alloca " << type;
    if (!op->count.empty()) {
      os << ", i64 "
         << require_operand_kind(op->count, "LirAllocaOp.count",
                                 {LirOperandKind::SsaValue,
                                  LirOperandKind::Immediate},
                                 true);
    }
    if (op->align > 0) os << ", align " << op->align;
    os << "\n";
  } else if (const auto* op = std::get_if<LirInlineAsmOp>(&inst)) {
    os << "  ";
    if (!op->result.empty()) {
      os << require_operand_kind(op->result, "LirInlineAsmOp.result",
                                 {LirOperandKind::SsaValue}, true)
         << " = ";
    }
    os << "call " << require_type_ref(op->ret_type, "LirInlineAsmOp.ret_type", true) << " asm ";
    if (op->side_effects) os << "sideeffect ";
    os << "\"" << op->asm_text << "\", \"" << op->constraints << "\"("
       << op->args_str << ")";
    if (op->insn_r) {
      os << " ; insn.r opcode=" << op->insn_r->opcode
         << " funct3=" << op->insn_r->funct3
         << " funct7=" << op->insn_r->funct7
         << " rd=$" << op->insn_r->operand_indices[0]
         << " rs1=$" << op->insn_r->operand_indices[1]
         << " rs2=$" << op->insn_r->operand_indices[2];
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    os << "  call void @llvm.memcpy.p0.p0.i64(ptr "
       << require_operand_kind(op->dst, "LirMemcpyOp.dst",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", ptr "
       << require_operand_kind(op->src, "LirMemcpyOp.src",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", i64 "
       << require_operand_kind(op->size, "LirMemcpyOp.size",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", i1 " << (op->is_volatile ? "true" : "false") << ")\n";
  } else if (const auto* op = std::get_if<LirMemsetOp>(&inst)) {
    os << "  call void @llvm.memset.p0.i64(ptr "
       << require_operand_kind(op->dst, "LirMemsetOp.dst",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", i8 "
       << require_operand_kind(op->byte_val, "LirMemsetOp.byte_val",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", i64 "
       << require_operand_kind(op->size, "LirMemsetOp.size",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", i1 " << (op->is_volatile ? "true" : "false") << ")\n";
  } else if (const auto* op = std::get_if<LirVaStartOp>(&inst)) {
    os << "  call void @llvm.va_start.p0(ptr "
       << require_operand_kind(op->ap_ptr, "LirVaStartOp.ap_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ")\n";
  } else if (const auto* op = std::get_if<LirVaEndOp>(&inst)) {
    os << "  call void @llvm.va_end.p0(ptr "
       << require_operand_kind(op->ap_ptr, "LirVaEndOp.ap_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ")\n";
  } else if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
    os << "  call void @llvm.va_copy.p0.p0(ptr "
       << require_operand_kind(op->dst_ptr, "LirVaCopyOp.dst_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", ptr "
       << require_operand_kind(op->src_ptr, "LirVaCopyOp.src_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ")\n";
  } else if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirStackSaveOp.result",
                               {LirOperandKind::SsaValue})
       << " = call ptr @llvm.stacksave()\n";
  } else if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    os << "  call void @llvm.stackrestore(ptr "
       << require_operand_kind(op->saved_ptr, "LirStackRestoreOp.saved_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ")\n";
  } else if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    const auto& result =
        require_operand_kind(op->result, "LirAbsOp.result",
                             {LirOperandKind::SsaValue});
    const auto type = render_integer_type_ref(op->int_type,
                                              "LirAbsOp.int_type");
    os << "  " << result << " = call " << type
       << " @llvm.abs." << type << "(" << type << " "
       << require_operand_kind(op->arg, "LirAbsOp.arg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", i1 true)\n";
  } else if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    const std::string direct = resolve_direct_label_address(function, op->addr,
                                                            link_names);
    os << "  indirectbr ptr "
       << (direct.empty() ? require_operand_kind(op->addr, "LirIndirectBrOp.addr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
                          : direct)
       << ", [";
    for (size_t i = 0; i < op->targets.size(); ++i) {
      if (i) os << ", ";
      os << "label %" << op->targets[i];
    }
    os << "]\n";
  } else if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirExtractValueOp.result",
                               {LirOperandKind::SsaValue})
       << " = extractvalue "
       << render_extract_value_aggregate_type(*op) << " "
       << require_operand_kind(op->agg, "LirExtractValueOp.agg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    const auto [agg_type, elem_type] = render_insert_value_native_types(*op);
    os << "  "
       << require_operand_kind(op->result, "LirInsertValueOp.result",
                               {LirOperandKind::SsaValue})
       << " = insertvalue "
       << agg_type << " "
       << require_operand_kind(op->agg, "LirInsertValueOp.agg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << elem_type << " "
       << require_operand_kind(op->elem, "LirInsertValueOp.elem",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    const auto type = op->requires_native_result_authority &&
                              op->type_str.kind() == LirTypeKind::Integer
                          ? render_integer_type_ref(op->type_str,
                                                    "LirLoadOp.type_str")
                          : require_type_ref(op->type_str,
                                             "LirLoadOp.type_str", true);
    os << "  "
       << require_operand_kind(op->result, "LirLoadOp.result",
                               {LirOperandKind::SsaValue})
       << " = load " << type
       << ", ptr "
       << require_operand_kind(op->ptr, "LirLoadOp.ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << "\n";
  } else if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    const std::string direct = resolve_direct_label_address(function, op->val,
                                                            link_names);
    const auto type = op->requires_native_store_authority &&
                              op->type_str.kind() == LirTypeKind::Integer
                          ? render_integer_type_ref(op->type_str,
                                                    "LirStoreOp.type_str")
                          : require_type_ref(op->type_str,
                                             "LirStoreOp.type_str", true);
    os << "  store " << type
       << " "
       << (direct.empty() ? require_operand_kind(op->val, "LirStoreOp.val",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
                          : direct)
       << ", ptr "
       << require_operand_kind(op->ptr, "LirStoreOp.ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << "\n";
  } else if (const auto* op = std::get_if<LirCastOp>(&inst)) {
    const char* opname = nullptr;
    switch (op->kind) {
      case LirCastKind::Trunc:    opname = "trunc"; break;
      case LirCastKind::ZExt:     opname = "zext"; break;
      case LirCastKind::SExt:     opname = "sext"; break;
      case LirCastKind::FPTrunc:  opname = "fptrunc"; break;
      case LirCastKind::FPExt:    opname = "fpext"; break;
      case LirCastKind::FPToSI:   opname = "fptosi"; break;
      case LirCastKind::FPToUI:   opname = "fptoui"; break;
      case LirCastKind::SIToFP:   opname = "sitofp"; break;
      case LirCastKind::UIToFP:   opname = "uitofp"; break;
      case LirCastKind::PtrToInt: opname = "ptrtoint"; break;
      case LirCastKind::IntToPtr: opname = "inttoptr"; break;
      case LirCastKind::Bitcast:  opname = "bitcast"; break;
    }
    os << "  "
       << require_operand_kind(op->result, "LirCastOp.result",
                               {LirOperandKind::SsaValue})
       << " = " << opname << " "
       << render_cast_endpoint_type_ref(op->from_type, "LirCastOp.from_type")
       << " "
       << require_operand_kind(op->operand, "LirCastOp.operand",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << " to "
       << render_cast_endpoint_type_ref(op->to_type, "LirCastOp.to_type")
       << "\n";
  } else if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    const std::string direct = resolve_direct_label_address(function, op->ptr,
                                                            link_names);
    os << "  "
       << require_operand_kind(op->result, "LirGepOp.result",
                               {LirOperandKind::SsaValue})
       << " = getelementptr ";
    if (op->inbounds) os << "inbounds ";
    os << require_type_ref(op->element_type, "LirGepOp.element_type")
       << ", ptr "
       << (direct.empty()
               ? require_operand_kind(op->ptr, "LirGepOp.ptr",
                                      {LirOperandKind::SsaValue,
                                       LirOperandKind::Global})
               : direct);
    for (const auto& idx : op->indices) {
      os << ", ";
      if (idx.is_authoritative()) {
        os << (idx.type_ref().kind() == LirTypeKind::Integer
                   ? render_integer_type_ref(idx.type_ref(),
                                             "LirGepOp.indices.type")
                   : require_type_ref(idx.type_ref(),
                                      "LirGepOp.indices.type"))
           << " "
           << require_operand_kind(
                  idx.value(), "LirGepOp.indices.value",
                  {LirOperandKind::SsaValue, LirOperandKind::Immediate});
      } else {
        os << idx.presentation();
      }
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    os << "  ";
    if (!op->result.empty()) {
      os << require_operand_kind(op->result, "LirCallOp.result",
                                 {LirOperandKind::SsaValue}, true)
         << " = ";
    }
    os << "call " << render_ext_attr(op->return_ext_attr)
       << require_type_ref(op->return_type, "LirCallOp.return_type", true) << " ";
    LirCallOp validated = *op;
    validated.callee = require_operand_kind(op->callee, "LirCallOp.callee",
                                            {LirOperandKind::SsaValue,
                                             LirOperandKind::Global});
    validated.callee = resolve_direct_call_callee(validated, link_names);
    if (std::optional<std::string> suffix =
            render_call_signature_suffix_from_store(mod,
                                                    validated.callee_signature_ref);
        suffix.has_value()) {
      validated.callee_type_suffix = std::move(*suffix);
    }
    os << format_lir_call_site(validated) << "\n";
  } else if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    const LirTypeRef* type = &op->type_str;
    if (op->compact_scalar_type) {
      const auto selected_scalar =
          LirCompactScalarType::from_type_ref(op->compact_scalar_type->type);
      if (!selected_scalar || op->compact_scalar_type->type != op->type_str) {
        throw LirVerifyError(
            LirVerifyErrorKind::Malformed,
            "LirBinOp.compact_scalar_type: must mirror one selected integer or floating scalar binop type");
      }
      type = &op->compact_scalar_type->type;
    }
    const auto rendered_type =
        type->kind() == LirTypeKind::Integer
            ? render_integer_type_ref(*type, "LirBinOp.compact_scalar_type")
            : (type->kind() == LirTypeKind::Floating
                   ? render_floating_type_ref(*type,
                                              "LirBinOp.compact_scalar_type")
                   : require_type_ref(*type, "LirBinOp.compact_scalar_type",
                                      true));
    os << "  "
       << require_operand_kind(op->result, "LirBinOp.result",
                               {LirOperandKind::SsaValue})
       << " = " << render_binary_opcode(op->opcode, "LirBinOp.opcode") << " "
       << rendered_type << " ";
    if (op->rhs.empty()) {
      // Unary op (fneg): "fneg type lhs"
      os << require_operand_kind(op->lhs, "LirBinOp.lhs",
                                 {LirOperandKind::SsaValue,
                                  LirOperandKind::Global,
                                  LirOperandKind::Immediate,
                                  LirOperandKind::SpecialToken});
    } else {
      os << require_operand_kind(op->lhs, "LirBinOp.lhs",
                                 {LirOperandKind::SsaValue,
                                  LirOperandKind::Global,
                                  LirOperandKind::Immediate,
                                  LirOperandKind::SpecialToken})
         << ", "
         << require_operand_kind(op->rhs, "LirBinOp.rhs",
                                 {LirOperandKind::SsaValue,
                                  LirOperandKind::Global,
                                  LirOperandKind::Immediate,
                                  LirOperandKind::SpecialToken});
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    const auto type = op->is_float
                          ? render_floating_type_ref(op->type_str,
                                                     "LirCmpOp.type_str")
                          : (op->type_str.kind() == LirTypeKind::Integer
                                 ? render_integer_type_ref(op->type_str,
                                                           "LirCmpOp.type_str")
                                 : require_type_ref(op->type_str,
                                                    "LirCmpOp.type_str"));
    os << "  " << op->result << " = " << (op->is_float ? "fcmp " : "icmp ")
       << render_cmp_predicate(op->predicate, "LirCmpOp.predicate") << " "
       << type << " "
       << require_operand_kind(op->lhs, "LirCmpOp.lhs",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", "
       << require_operand_kind(op->rhs, "LirCmpOp.rhs",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirPhiOp.result",
                               {LirOperandKind::SsaValue})
       << " = phi "
       << render_phi_boundary_type_ref(*op);
    for (size_t i = 0; i < op->incoming.size(); ++i) {
      os << (i == 0 ? " " : ", ");
      os << "[ " << op->incoming[i].value.str() << ", %" << op->incoming[i].label << " ]";
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    const auto type = render_integer_type_ref(op->type_str,
                                              "LirSelectOp.type_str");
    os << "  "
       << require_operand_kind(op->result, "LirSelectOp.result",
                               {LirOperandKind::SsaValue})
       << " = select i1 "
       << require_operand_kind(op->cond, "LirSelectOp.cond",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << type << " "
       << require_operand_kind(op->true_val, "LirSelectOp.true_val",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << type << " "
       << require_operand_kind(op->false_val, "LirSelectOp.false_val",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    const auto store_types = render_required_insert_element_types_from_store(mod, *op);
    const std::string vec_type =
        store_types ? store_types->first
                    : require_type_ref(op->vec_type, "LirInsertElementOp.vec_type");
    const std::string elem_type =
        store_types ? store_types->second
                    : require_type_ref(op->elem_type, "LirInsertElementOp.elem_type");
    os << "  "
       << require_operand_kind(op->result, "LirInsertElementOp.result",
                               {LirOperandKind::SsaValue})
       << " = insertelement " << vec_type << " "
       << require_operand_kind(op->vec, "LirInsertElementOp.vec",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << elem_type << " "
       << require_operand_kind(op->elem, "LirInsertElementOp.elem",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", "
       << require_operand_kind(op->index, "LirInsertElementOp.index",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
    const std::string vec_type =
        render_extract_element_vector_type_from_store(mod, *op)
            .value_or(require_type_ref(op->vec_type, "LirExtractElementOp.vec_type"));
    os << "  "
       << require_operand_kind(op->result, "LirExtractElementOp.result",
                               {LirOperandKind::SsaValue})
       << " = extractelement " << vec_type << " "
       << require_operand_kind(op->vec, "LirExtractElementOp.vec",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", "
       << require_type_ref(op->index_type, "LirExtractElementOp.index_type")
       << " "
       << require_operand_kind(op->index, "LirExtractElementOp.index",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
    const auto store_types = render_required_shuffle_vector_types_from_store(mod, *op);
    const std::string vec_type =
        store_types ? store_types->first
                    : require_type_ref(op->vec_type, "LirShuffleVectorOp.vec_type");
    const std::string mask_type =
        store_types ? store_types->second
                    : require_type_ref(op->mask_type, "LirShuffleVectorOp.mask_type");
    os << "  "
       << require_operand_kind(op->result, "LirShuffleVectorOp.result",
                               {LirOperandKind::SsaValue})
       << " = shufflevector " << vec_type << " "
       << require_operand_kind(op->vec1, "LirShuffleVectorOp.vec1",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << vec_type << " "
       << require_operand_kind(op->vec2, "LirShuffleVectorOp.vec2",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << mask_type << " "
       << require_operand_kind(op->mask, "LirShuffleVectorOp.mask",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    const auto type = op->requires_native_memory_va_authority &&
                              op->result_authority.has_value() &&
                              op->result_type_authority.has_value() &&
                              op->type_str.kind() == LirTypeKind::Integer
                          ? render_integer_type_ref(op->type_str,
                                                    "LirVaArgOp.type_str")
                          : require_type_ref(op->type_str,
                                             "LirVaArgOp.type_str");
    os << "  "
       << require_operand_kind(op->result, "LirVaArgOp.result",
                               {LirOperandKind::SsaValue})
       << " = va_arg ptr "
       << require_operand_kind(op->ap_ptr, "LirVaArgOp.ap_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", " << type << "\n";
  }
}

// Render a LirTerminator to text.
void render_terminator(std::ostringstream& os, const LirTerminator& term) {
  if (const auto* br = std::get_if<LirBr>(&term)) {
    os << "  br label %" << br->target_label << "\n";
  } else if (const auto* cbr = std::get_if<LirCondBr>(&term)) {
    os << "  br i1 " << cbr->cond_name << ", label %" << cbr->true_label
       << ", label %" << cbr->false_label << "\n";
  } else if (const auto* ret = std::get_if<LirRet>(&term)) {
    if (!ret->value_str.has_value()) {
      os << "  ret void\n";
    } else {
      const std::string type =
          ret->value_str->has_authority() &&
                  ret->type_str.kind() == LirTypeKind::Integer
              ? render_integer_type_ref(ret->type_str, "LirRet.type_str")
              : require_type_ref(ret->type_str, "LirRet.type_str");
      os << "  ret " << type
         << " "
         << require_operand_kind(*ret->value_str, "LirRet.value_str",
                                 {LirOperandKind::SsaValue,
                                  LirOperandKind::Global,
                                  LirOperandKind::Immediate,
                                  LirOperandKind::SpecialToken,
                                  LirOperandKind::RawText})
         << "\n";
    }
  } else if (const auto* sw = std::get_if<LirSwitch>(&term)) {
    os << "  switch " << sw->selector_type_ref.str() << " " << sw->selector_name
       << ", label %" << sw->default_label << " [\n";
    for (const auto& [val, label] : sw->cases) {
      os << "    " << sw->selector_type_ref.str() << " " << val << ", label %" << label << "\n";
    }
    os << "  ]\n";
  } else if (std::get_if<LirUnreachable>(&term)) {
    os << "  unreachable\n";
  }
  // LirIndirectBr is handled via LirIndirectBrOp instruction, not terminator.
}

std::string render_fn(const LirModule& mod, const LirFunction& f,
                      std::string_view resolved_name,
                      const c4c::LinkNameTable& link_names) {
  const std::string signature =
      render_function_signature_from_store(mod, f, resolved_name)
          .value_or(render_signature_with_link_name(f.signature_text,
                                                   resolved_name));
  if (f.is_declaration) return signature;
  std::ostringstream fout;
  fout << signature;
  fout << "{\n";
  for (size_t i = 0; i < f.blocks.size(); ++i) {
    const auto& blk = f.blocks[i];
    fout << blk.label << ":\n";
    // Alloca instructions are hoisted to the start of the entry block.
    if (i == 0) {
      for (const auto& inst : f.alloca_insts)
        render_inst(fout, mod, f, inst, link_names);
    }
    for (const auto& inst : blk.insts)
      render_inst(fout, mod, f, inst, link_names);
    render_terminator(fout, blk.terminator);
  }
  fout << "}\n\n";
  return fout.str();
}

}  // namespace

std::string render_struct_decl_llvm(const LirModule& mod,
                                    const LirStructDecl& decl) {
  const std::string_view name = mod.struct_names.spelling(decl.name_id);
  if (name.empty()) {
    throw LirVerifyError(LirVerifyErrorKind::Malformed,
                         "LirStructDecl.name_id: must resolve to a struct name");
  }

  std::ostringstream out;
  out << name << " = type ";
  if (decl.is_opaque) {
    out << "opaque";
    return out.str();
  }

  const char* open = decl.is_packed ? "<{" : "{";
  const char* close = decl.is_packed ? "}>" : "}";
  out << open;
  if (!decl.fields.empty()) {
    out << " ";
    for (std::size_t i = 0; i < decl.fields.size(); ++i) {
      if (i) out << ", ";
      (void)require_type_ref(decl.fields[i].type, "LirStructDecl.fields.type");
      out << decl.fields[i].type.render_llvm();
    }
    out << " ";
  }
  out << close;
  return out.str();
}

std::string print_llvm(const LirModule& mod) {
  verify_module(mod);
  c4c::codegen::llvm_helpers::set_active_target_profile(mod.target_profile);
  const std::string target_triple = c4c::llvm_target_triple(mod.target_profile);

  std::ostringstream out;

  if (!mod.data_layout.empty()) out << "target datalayout = \"" << mod.data_layout << "\"\n";
  if (!target_triple.empty()) out << "target triple = \"" << target_triple << "\"\n";
  if (!mod.data_layout.empty() || !target_triple.empty()) out << "\n";

  // Canonical aggregate-store facts are the printed authority once present.
  // Legacy structured declarations remain the no-owner compatibility path.
  if (!mod.aggregate_store.empty()) {
    std::unordered_set<c4c::StructNameId> store_decl_ids;
    store_decl_ids.reserve(mod.aggregate_store.size());
    for (const auto& entry : mod.aggregate_store) {
      const LirStructDecl* decl = mod.find_struct_decl(entry.name_id);
      if (!decl) {
        throw LirVerifyError(
            LirVerifyErrorKind::Malformed,
            "LirAggregateStoreEntry.name_id: must resolve to a structured declaration");
      }
      store_decl_ids.insert(entry.name_id);
      out << render_struct_decl_llvm(mod, *decl) << "\n";
    }
    for (const auto& decl : mod.struct_decls) {
      if (store_decl_ids.find(decl.name_id) != store_decl_ids.end()) continue;
      out << render_struct_decl_llvm(mod, decl) << "\n";
    }
  } else {
    for (const auto& decl : mod.struct_decls) {
      out << render_struct_decl_llvm(mod, decl) << "\n";
    }
  }

  // String pool constants.
  for (const auto& sc : mod.string_pool) {
    if (sc.byte_length < 0) {
      // Pre-formatted wide string: raw_bytes holds "type init"
      out << sc.pool_name << " = private unnamed_addr constant " << sc.raw_bytes << "\n";
    } else {
      out << sc.pool_name << " = private unnamed_addr constant ["
          << sc.byte_length << " x i8] c\"" << sc.raw_bytes << "\\00\"\n";
    }
  }

  // Global variable definitions.
  for (const auto& g : mod.globals) {
    const std::string_view resolved_name = resolve_link_name(mod.link_names, g.link_name_id);
    out << llvm_global_sym(resolved_name.empty() ? g.name : std::string(resolved_name))
        << " = " << g.linkage_vis << g.qualifier
        << render_global_type(mod, g);
    if (!g.is_extern_decl) out << " " << g.init_text;
    if (g.align_bytes > 1) out << ", align " << g.align_bytes;
    out << "\n";
  }

  if (!mod.struct_decls.empty() || !mod.string_pool.empty() || !mod.globals.empty())
    out << "\n";

  // Intrinsic declarations.
  if (mod.need_va_start)    out << "declare void @llvm.va_start.p0(ptr)\n";
  if (mod.need_va_end)      out << "declare void @llvm.va_end.p0(ptr)\n";
  if (mod.need_va_copy)     out << "declare void @llvm.va_copy.p0.p0(ptr, ptr)\n";
  if (mod.need_memcpy)      out << "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n";
  if (mod.need_memset)      out << "declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)\n";
  if (mod.need_stacksave)   out << "declare ptr @llvm.stacksave()\n";
  if (mod.need_stackrestore) out << "declare void @llvm.stackrestore(ptr)\n";
  if (mod.need_abs) {
    out << "declare i32 @llvm.abs.i32(i32, i1 immarg)\n";
    out << "declare i64 @llvm.abs.i64(i64, i1 immarg)\n";
  }
  if (mod.need_ptrmask)     out << "declare ptr @llvm.ptrmask.p0.i64(ptr, i64)\n";
  if (mod.need_va_start || mod.need_va_end || mod.need_va_copy ||
      mod.need_memcpy || mod.need_memset || mod.need_stacksave || mod.need_stackrestore ||
      mod.need_abs || mod.need_ptrmask) out << "\n";

  // External function declarations.
  for (const auto& ed : mod.extern_decls) {
    const std::string params =
        render_selected_extern_param_list_from_store(mod, ed).value_or("(...)");
    out << "declare " << render_ext_attr(ed.return_ext_attr)
        << render_extern_return_type(mod, ed) << " "
        << llvm_global_sym(resolve_extern_decl_name(ed, mod.link_names))
        << params << "\n";
  }
  if (!mod.extern_decls.empty()) out << "\n";

  // Function bodies.
  // Dead internal functions have already been removed by the lowering pass
  // (eliminate_dead_internals); the printer renders everything it receives.
  for (const auto& f : mod.functions) {
    out << render_fn(mod, f, resolve_link_name(mod.link_names, f.link_name_id),
                     mod.link_names);
  }

  // Specialization metadata.
  if (!mod.spec_entries.empty()) {
    out << "\n";
    int md_id = 0;
    for (const auto& e : mod.spec_entries) {
      const std::string_view resolved_name =
          resolve_link_name(mod.link_names, e.mangled_link_name_id);
      out << "!" << md_id << " = !{!\"" << e.spec_key << "\", !\""
          << e.template_origin << "\", !\""
          << (resolved_name.empty() ? e.mangled_name : std::string(resolved_name))
          << "\"}\n";
      ++md_id;
    }
    out << "!c4c.specializations = !{";
    for (int i = 0; i < md_id; ++i) {
      if (i) out << ", ";
      out << "!" << i;
    }
    out << "}\n";
  }

  return out.str();
}

}  // namespace c4c::codegen::lir
