#include "ir.hpp"
#include "call_args_ops.hpp"
#include "../shared/llvm_helpers.hpp"

#include <cctype>
#include <sstream>
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

  // signature_text is retained final LLVM header spelling. The function's
  // LinkNameId is the semantic identity; this string replacement is the
  // compatibility rendering bridge until the header itself is fully structured.
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

std::string resolve_direct_call_callee(const LirCallOp& call,
                                       const c4c::LinkNameTable& link_names) {
  if (!parse_lir_direct_global_callee(call.callee).has_value()) {
    return call.callee;
  }

  const std::string_view resolved_name =
      resolve_link_name(link_names, call.direct_callee_link_name_id);
  if (resolved_name.empty()) {
    return call.callee;
  }
  return llvm_global_sym(std::string(resolved_name));
}

std::string resolve_extern_decl_name(const LirExternDecl& decl,
                                     const c4c::LinkNameTable& link_names) {
  const std::string_view resolved_name =
      resolve_link_name(link_names, decl.link_name_id);
  return resolved_name.empty() ? decl.name : std::string(resolved_name);
}

// Render a single LirInst to text.
void render_inst(std::ostringstream& os, const LirInst& inst,
                 const c4c::LinkNameTable& link_names) {
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    const auto& result =
        require_operand_kind(op->result, "LirAllocaOp.result",
                             {LirOperandKind::SsaValue});
    const auto& type = require_type_ref(op->type_str, "LirAllocaOp.type_str");
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
       << op->args_str << ")\n";
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
    const auto& type = require_type_ref(op->int_type, "LirAbsOp.int_type");
    os << "  " << result << " = call " << type
       << " @llvm.abs." << type << "(" << type << " "
       << require_operand_kind(op->arg, "LirAbsOp.arg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", i1 true)\n";
  } else if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    os << "  indirectbr ptr "
       << require_operand_kind(op->addr, "LirIndirectBrOp.addr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", [";
    for (size_t i = 0; i < op->targets.size(); ++i) {
      if (i) os << ", ";
      os << "label " << op->targets[i];
    }
    os << "]\n";
  } else if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirExtractValueOp.result",
                               {LirOperandKind::SsaValue})
       << " = extractvalue "
       << require_type_ref(op->agg_type, "LirExtractValueOp.agg_type") << " "
       << require_operand_kind(op->agg, "LirExtractValueOp.agg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirInsertValueOp.result",
                               {LirOperandKind::SsaValue})
       << " = insertvalue "
       << require_type_ref(op->agg_type, "LirInsertValueOp.agg_type") << " "
       << require_operand_kind(op->agg, "LirInsertValueOp.agg",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << require_type_ref(op->elem_type, "LirInsertValueOp.elem_type")
       << " "
       << require_operand_kind(op->elem, "LirInsertValueOp.elem",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirLoadOp.result",
                               {LirOperandKind::SsaValue})
       << " = load " << require_type_ref(op->type_str, "LirLoadOp.type_str", true)
       << ", ptr "
       << require_operand_kind(op->ptr, "LirLoadOp.ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << "\n";
  } else if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    os << "  store " << require_type_ref(op->type_str, "LirStoreOp.type_str", true)
       << " "
       << require_operand_kind(op->val, "LirStoreOp.val",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
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
       << require_type_ref(op->from_type, "LirCastOp.from_type") << " "
       << require_operand_kind(op->operand, "LirCastOp.operand",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << " to " << require_type_ref(op->to_type, "LirCastOp.to_type") << "\n";
  } else if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirGepOp.result",
                               {LirOperandKind::SsaValue})
       << " = getelementptr ";
    if (op->inbounds) os << "inbounds ";
    os << require_type_ref(op->element_type, "LirGepOp.element_type")
       << ", ptr "
       << require_operand_kind(op->ptr, "LirGepOp.ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global});
    for (const auto& idx : op->indices) os << ", " << idx;
    os << "\n";
  } else if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    os << "  ";
    if (!op->result.empty()) {
      os << require_operand_kind(op->result, "LirCallOp.result",
                                 {LirOperandKind::SsaValue}, true)
         << " = ";
    }
    os << "call " << require_type_ref(op->return_type, "LirCallOp.return_type", true) << " ";
    LirCallOp validated = *op;
    validated.callee = require_operand_kind(op->callee, "LirCallOp.callee",
                                            {LirOperandKind::SsaValue,
                                             LirOperandKind::Global});
    validated.callee = resolve_direct_call_callee(validated, link_names);
    os << format_lir_call_site(validated) << "\n";
  } else if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirBinOp.result",
                               {LirOperandKind::SsaValue})
       << " = " << render_binary_opcode(op->opcode, "LirBinOp.opcode") << " "
       << require_type_ref(op->type_str, "LirBinOp.type_str", true) << " ";
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
    os << "  " << op->result << " = " << (op->is_float ? "fcmp " : "icmp ")
       << render_cmp_predicate(op->predicate, "LirCmpOp.predicate") << " "
       << require_type_ref(op->type_str, "LirCmpOp.type_str") << " "
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
       << " = phi " << require_type_ref(op->type_str, "LirPhiOp.type_str");
    for (size_t i = 0; i < op->incoming.size(); ++i) {
      os << (i == 0 ? " " : ", ");
      os << "[ " << op->incoming[i].first << ", %" << op->incoming[i].second << " ]";
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    const auto& type = require_type_ref(op->type_str, "LirSelectOp.type_str");
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
    os << "  "
       << require_operand_kind(op->result, "LirInsertElementOp.result",
                               {LirOperandKind::SsaValue})
       << " = insertelement "
       << require_type_ref(op->vec_type, "LirInsertElementOp.vec_type") << " "
       << require_operand_kind(op->vec, "LirInsertElementOp.vec",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << ", " << require_type_ref(op->elem_type, "LirInsertElementOp.elem_type")
       << " "
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
    os << "  "
       << require_operand_kind(op->result, "LirExtractElementOp.result",
                               {LirOperandKind::SsaValue})
       << " = extractelement "
       << require_type_ref(op->vec_type, "LirExtractElementOp.vec_type") << " "
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
    const auto& vec_type =
        require_type_ref(op->vec_type, "LirShuffleVectorOp.vec_type");
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
       << ", " << require_type_ref(op->mask_type, "LirShuffleVectorOp.mask_type")
       << " "
       << require_operand_kind(op->mask, "LirShuffleVectorOp.mask",
                               {LirOperandKind::SsaValue,
                                LirOperandKind::Global,
                                LirOperandKind::Immediate,
                                LirOperandKind::SpecialToken})
       << "\n";
  } else if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    os << "  "
       << require_operand_kind(op->result, "LirVaArgOp.result",
                               {LirOperandKind::SsaValue})
       << " = va_arg ptr "
       << require_operand_kind(op->ap_ptr, "LirVaArgOp.ap_ptr",
                               {LirOperandKind::SsaValue, LirOperandKind::Global})
       << ", " << require_type_ref(op->type_str, "LirVaArgOp.type_str") << "\n";
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
      os << "  ret " << ret->type_str << " " << *ret->value_str << "\n";
    }
  } else if (const auto* sw = std::get_if<LirSwitch>(&term)) {
    os << "  switch " << sw->selector_type << " " << sw->selector_name
       << ", label %" << sw->default_label << " [\n";
    for (const auto& [val, label] : sw->cases) {
      os << "    " << sw->selector_type << " " << val << ", label %" << label << "\n";
    }
    os << "  ]\n";
  } else if (std::get_if<LirUnreachable>(&term)) {
    os << "  unreachable\n";
  }
  // LirIndirectBr is handled via LirIndirectBrOp instruction, not terminator.
}

std::string render_fn(const LirFunction& f, std::string_view resolved_name,
                      const c4c::LinkNameTable& link_names) {
  const std::string signature =
      render_signature_with_link_name(f.signature_text, resolved_name);
  if (f.is_declaration) return signature;
  std::ostringstream fout;
  fout << signature;
  fout << "{\n";
  for (size_t i = 0; i < f.blocks.size(); ++i) {
    const auto& blk = f.blocks[i];
    fout << blk.label << ":\n";
    // Alloca instructions are hoisted to the start of the entry block.
    if (i == 0) {
      for (const auto& inst : f.alloca_insts) render_inst(fout, inst, link_names);
    }
    for (const auto& inst : blk.insts) render_inst(fout, inst, link_names);
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
      out << require_type_ref(decl.fields[i].type, "LirStructDecl.fields.type");
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

  // Type declarations (struct/union definitions).
  for (const auto& decl : mod.struct_decls) {
    out << render_struct_decl_llvm(mod, decl) << "\n";
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
        << g.llvm_type;
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
    out << "declare " << ed.return_type_str << " "
        << llvm_global_sym(resolve_extern_decl_name(ed, mod.link_names))
        << "(...)\n";
  }
  if (!mod.extern_decls.empty()) out << "\n";

  // Function bodies.
  // Dead internal functions have already been removed by the lowering pass
  // (eliminate_dead_internals); the printer renders everything it receives.
  for (const auto& f : mod.functions) {
    out << render_fn(f, resolve_link_name(mod.link_names, f.link_name_id), mod.link_names);
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
