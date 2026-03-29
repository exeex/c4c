#include "lir_printer.hpp"
#include "../shared/llvm_helpers.hpp"

#include <cctype>
#include <sstream>
#include <vector>

namespace c4c::codegen::lir {

namespace {

std::string llvm_global_sym(const std::string& raw) {
  return "@" + c4c::codegen::llvm_helpers::quote_llvm_ident(raw);
}

// Render a single LirInst to text.
void render_inst(std::ostringstream& os, const LirInst& inst) {
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    os << "  " << op->result << " = alloca " << op->type_str;
    if (!op->count.empty()) os << ", i64 " << op->count;
    if (op->align > 0) os << ", align " << op->align;
    os << "\n";
  } else if (const auto* op = std::get_if<LirInlineAsmOp>(&inst)) {
    os << "  ";
    if (!op->result.empty()) os << op->result << " = ";
    os << "call " << op->ret_type << " asm ";
    if (op->side_effects) os << "sideeffect ";
    os << "\"" << op->asm_text << "\", \"" << op->constraints << "\"("
       << op->args_str << ")\n";
  } else if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    os << "  call void @llvm.memcpy.p0.p0.i64(ptr " << op->dst
       << ", ptr " << op->src << ", i64 " << op->size
       << ", i1 " << (op->is_volatile ? "true" : "false") << ")\n";
  } else if (const auto* op = std::get_if<LirMemsetOp>(&inst)) {
    os << "  call void @llvm.memset.p0.i64(ptr " << op->dst
       << ", i8 " << op->byte_val << ", i64 " << op->size
       << ", i1 " << (op->is_volatile ? "true" : "false") << ")\n";
  } else if (const auto* op = std::get_if<LirVaStartOp>(&inst)) {
    os << "  call void @llvm.va_start.p0(ptr " << op->ap_ptr << ")\n";
  } else if (const auto* op = std::get_if<LirVaEndOp>(&inst)) {
    os << "  call void @llvm.va_end.p0(ptr " << op->ap_ptr << ")\n";
  } else if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
    os << "  call void @llvm.va_copy.p0.p0(ptr " << op->dst_ptr
       << ", ptr " << op->src_ptr << ")\n";
  } else if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) {
    os << "  " << op->result << " = call ptr @llvm.stacksave()\n";
  } else if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    os << "  call void @llvm.stackrestore(ptr " << op->saved_ptr << ")\n";
  } else if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    os << "  " << op->result << " = call " << op->int_type
       << " @llvm.abs." << op->int_type << "(" << op->int_type
       << " " << op->arg << ", i1 true)\n";
  } else if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    os << "  indirectbr ptr " << op->addr << ", [";
    for (size_t i = 0; i < op->targets.size(); ++i) {
      if (i) os << ", ";
      os << "label " << op->targets[i];
    }
    os << "]\n";
  } else if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    os << "  " << op->result << " = extractvalue " << op->agg_type << " "
       << op->agg << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    os << "  " << op->result << " = insertvalue " << op->agg_type << " "
       << op->agg << ", " << op->elem_type << " " << op->elem
       << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    os << "  " << op->result << " = load " << op->type_str << ", ptr " << op->ptr << "\n";
  } else if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    os << "  store " << op->type_str << " " << op->val << ", ptr " << op->ptr << "\n";
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
    os << "  " << op->result << " = " << opname << " " << op->from_type
       << " " << op->operand << " to " << op->to_type << "\n";
  } else if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    os << "  " << op->result << " = getelementptr ";
    if (op->inbounds) os << "inbounds ";
    os << op->element_type << ", ptr " << op->ptr;
    for (const auto& idx : op->indices) os << ", " << idx;
    os << "\n";
  } else if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    os << "  ";
    if (!op->result.empty()) {
      os << op->result << " = ";
    }
    os << "call " << op->return_type << " ";
    if (!op->callee_type_suffix.empty()) {
      os << op->callee_type_suffix << " ";
    }
    os << op->callee << "(" << op->args_str << ")\n";
  } else if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    os << "  " << op->result << " = " << op->opcode << " " << op->type_str << " ";
    if (op->rhs.empty()) {
      // Unary op (fneg): "fneg type lhs"
      os << op->lhs;
    } else {
      os << op->lhs << ", " << op->rhs;
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    os << "  " << op->result << " = " << (op->is_float ? "fcmp " : "icmp ")
       << op->predicate << " " << op->type_str << " " << op->lhs
       << ", " << op->rhs << "\n";
  } else if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    os << "  " << op->result << " = phi " << op->type_str;
    for (size_t i = 0; i < op->incoming.size(); ++i) {
      os << (i == 0 ? " " : ", ");
      os << "[ " << op->incoming[i].first << ", %" << op->incoming[i].second << " ]";
    }
    os << "\n";
  } else if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    os << "  " << op->result << " = select i1 " << op->cond
       << ", " << op->type_str << " " << op->true_val
       << ", " << op->type_str << " " << op->false_val << "\n";
  } else if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    os << "  " << op->result << " = insertelement " << op->vec_type << " " << op->vec
       << ", " << op->elem_type << " " << op->elem << ", " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
    os << "  " << op->result << " = extractelement " << op->vec_type << " " << op->vec
       << ", " << op->index_type << " " << op->index << "\n";
  } else if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
    os << "  " << op->result << " = shufflevector " << op->vec_type << " " << op->vec1
       << ", " << op->vec_type << " " << op->vec2 << ", " << op->mask_type
       << " " << op->mask << "\n";
  } else if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    os << "  " << op->result << " = va_arg ptr " << op->ap_ptr
       << ", " << op->type_str << "\n";
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

// Render a LirFunction to LLVM IR text.
std::string render_fn(const LirFunction& f) {
  if (f.is_declaration) return f.signature_text;
  std::ostringstream fout;
  fout << f.signature_text;
  fout << "{\n";
  for (size_t i = 0; i < f.blocks.size(); ++i) {
    const auto& blk = f.blocks[i];
    fout << blk.label << ":\n";
    // Alloca instructions are hoisted to the start of the entry block.
    if (i == 0) {
      for (const auto& inst : f.alloca_insts) render_inst(fout, inst);
    }
    for (const auto& inst : blk.insts) render_inst(fout, inst);
    render_terminator(fout, blk.terminator);
  }
  fout << "}\n\n";
  return fout.str();
}

}  // namespace

std::string print_llvm(const LirModule& mod) {
  c4c::codegen::llvm_helpers::set_active_target_triple(mod.target_triple);

  std::ostringstream out;

  if (!mod.data_layout.empty()) out << "target datalayout = \"" << mod.data_layout << "\"\n";
  if (!mod.target_triple.empty()) out << "target triple = \"" << mod.target_triple << "\"\n";
  if (!mod.data_layout.empty() || !mod.target_triple.empty()) out << "\n";

  // Type declarations (struct/union definitions).
  for (const auto& td : mod.type_decls) out << td << "\n";

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
    out << llvm_global_sym(g.name) << " = " << g.linkage_vis << g.qualifier
        << g.llvm_type;
    if (!g.is_extern_decl) out << " " << g.init_text;
    if (g.align_bytes > 1) out << ", align " << g.align_bytes;
    out << "\n";
  }

  if (!mod.type_decls.empty() || !mod.string_pool.empty() || !mod.globals.empty())
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
    out << "declare " << ed.return_type_str << " " << llvm_global_sym(ed.name) << "(...)\n";
  }
  if (!mod.extern_decls.empty()) out << "\n";

  // Function bodies.
  // Dead internal functions have already been removed by the lowering pass
  // (eliminate_dead_internals); the printer renders everything it receives.
  for (const auto& f : mod.functions) {
    out << render_fn(f);
  }

  // Specialization metadata.
  if (!mod.spec_entries.empty()) {
    out << "\n";
    int md_id = 0;
    for (const auto& e : mod.spec_entries) {
      out << "!" << md_id << " = !{!\"" << e.spec_key << "\", !\""
          << e.template_origin << "\", !\"" << e.mangled_name << "\"}\n";
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
