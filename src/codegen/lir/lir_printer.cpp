#include "lir_printer.hpp"
#include "../llvm/hir_to_llvm_helpers.hpp"

#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::codegen::lir {

namespace {

std::string llvm_global_sym(const std::string& raw) {
  return "@" + c4c::codegen::llvm_backend::detail::quote_llvm_ident(raw);
}

// Render a single LirInst to text.
void render_inst(std::ostringstream& os, const LirInst& inst) {
  if (const auto* raw = std::get_if<LirRawLine>(&inst)) {
    os << raw->line << "\n";
  } else if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    os << "  call void @llvm.memcpy.p0.p0.i64(ptr " << op->dst
       << ", ptr " << op->src << ", i64 " << op->size
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
  } else if (const auto* op = std::get_if<LirBitfieldExtract>(&inst)) {
    const std::string unit_ty = "i" + std::to_string(op->storage_unit_bits);
    // Load the full storage unit
    const std::string unit = op->result + ".bf.unit";
    os << "  " << unit << " = load " << unit_ty << ", ptr " << op->unit_ptr << "\n";
    // Shift right to align bitfield to bit 0
    std::string shifted = unit;
    if (op->bit_offset > 0) {
      shifted = op->result + ".bf.shr";
      os << "  " << shifted << " = lshr " << unit_ty << " " << unit
         << ", " << op->bit_offset << "\n";
    }
    // Mask to bit_width bits
    const unsigned long long mask = (op->bit_width >= 64)
        ? ~0ULL : ((1ULL << op->bit_width) - 1);
    const std::string masked = op->result + ".bf.mask";
    os << "  " << masked << " = and " << unit_ty << " " << shifted
       << ", " << mask << "\n";
    std::string result = masked;
    // Sign-extend if needed
    if (op->is_signed && op->bit_width < op->storage_unit_bits) {
      const int shift_amt = op->storage_unit_bits - op->bit_width;
      const std::string shl_tmp = op->result + ".bf.shl";
      os << "  " << shl_tmp << " = shl " << unit_ty << " " << masked
         << ", " << shift_amt << "\n";
      result = op->result + ".bf.sext";
      os << "  " << result << " = ashr " << unit_ty << " " << shl_tmp
         << ", " << shift_amt << "\n";
    }
    // Truncate or extend to promoted type
    const std::string promoted_ty = "i" + std::to_string(op->promoted_bits);
    if (op->storage_unit_bits != op->promoted_bits) {
      if (op->storage_unit_bits > op->promoted_bits) {
        os << "  " << op->result << " = trunc " << unit_ty << " " << result
           << " to " << promoted_ty << "\n";
      } else {
        os << "  " << op->result << " = "
           << (op->is_signed ? "sext " : "zext ")
           << unit_ty << " " << result << " to " << promoted_ty << "\n";
      }
    } else {
      // No size change — alias the result
      // We need a copy since LLVM IR doesn't allow aliasing; use bitcast identity
      // Actually, just emit an 'add 0' to rename
      os << "  " << op->result << " = add " << unit_ty << " " << result << ", 0\n";
    }
  } else if (const auto* op = std::get_if<LirAlloca>(&inst)) {
    using namespace c4c::codegen::llvm_backend::detail;
    const std::string alloca_ty = llvm_alloca_ty(op->type);
    os << "  " << op->name << " = alloca " << alloca_ty;
    if (op->align > 1) os << ", align " << op->align;
    os << "\n";
  } else if (const auto* op = std::get_if<LirHoistedStore>(&inst)) {
    using namespace c4c::codegen::llvm_backend::detail;
    if (op->zeroinit) {
      os << "  store " << llvm_alloca_ty(op->type) << " zeroinitializer, ptr " << op->ptr << "\n";
    } else {
      os << "  store " << llvm_ty(op->type) << " " << op->val << ", ptr " << op->ptr << "\n";
    }
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
  } else if (const auto* op = std::get_if<LirBitfieldInsert>(&inst)) {
    const std::string unit_ty = "i" + std::to_string(op->storage_unit_bits);
    // Load current storage unit
    const std::string old_unit = op->scratch + ".bf.old";
    os << "  " << old_unit << " = load " << unit_ty << ", ptr " << op->unit_ptr << "\n";
    // Create mask to clear the bitfield bits
    const unsigned long long field_mask_val = (op->bit_width >= 64)
        ? ~0ULL : ((1ULL << op->bit_width) - 1);
    const unsigned long long clear_mask = ~(field_mask_val << op->bit_offset);
    const std::string cleared = op->scratch + ".bf.clr";
    os << "  " << cleared << " = and " << unit_ty << " " << old_unit
       << ", " << static_cast<long long>(clear_mask) << "\n";
    // Mask and shift new value into position
    const std::string new_masked = op->scratch + ".bf.vm";
    os << "  " << new_masked << " = and " << unit_ty << " " << op->new_val
       << ", " << field_mask_val << "\n";
    std::string new_shifted = new_masked;
    if (op->bit_offset > 0) {
      new_shifted = op->scratch + ".bf.vs";
      os << "  " << new_shifted << " = shl " << unit_ty << " " << new_masked
         << ", " << op->bit_offset << "\n";
    }
    // Combine and store
    const std::string combined = op->scratch + ".bf.comb";
    os << "  " << combined << " = or " << unit_ty << " " << cleared << ", " << new_shifted << "\n";
    os << "  store " << unit_ty << " " << combined << ", ptr " << op->unit_ptr << "\n";
  }
}

// Render a LirTerminator to text.
void render_terminator(std::ostringstream& os, const LirTerminator& term) {
  if (const auto* raw = std::get_if<LirRawTerminator>(&term)) {
    os << raw->line << "\n";
  } else if (const auto* br = std::get_if<LirBr>(&term)) {
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

// Find all @name and @"quoted name" references in a text block.
std::unordered_set<std::string> find_refs(const std::string& body) {
  std::unordered_set<std::string> refs;
  size_t pos = 0;
  while ((pos = body.find('@', pos)) != std::string::npos) {
    ++pos;
    if (pos < body.size() && body[pos] == '"') {
      // Quoted identifier: @"ns::foo"
      ++pos;  // skip opening "
      size_t start = pos;
      while (pos < body.size() && body[pos] != '"') ++pos;
      if (pos > start) {
        refs.insert("\"" + body.substr(start, pos - start) + "\"");
      }
      if (pos < body.size()) ++pos;  // skip closing "
    } else {
      // Bare identifier: @foo
      size_t start = pos;
      while (pos < body.size() && (std::isalnum(static_cast<unsigned char>(body[pos])) || body[pos] == '_' || body[pos] == '.'))
        ++pos;
      if (pos > start) refs.insert(body.substr(start, pos - start));
    }
  }
  return refs;
}

}  // namespace

std::string print_llvm(const LirModule& mod) {
  c4c::codegen::llvm_backend::detail::set_active_target_triple(mod.target_triple);
  // Pre-render function bodies for DCE reference scanning.
  std::vector<std::string> rendered_bodies;
  rendered_bodies.reserve(mod.functions.size());
  for (const auto& f : mod.functions) {
    rendered_bodies.push_back(render_fn(f));
  }

  // Dead-code elimination: remove unreferenced internal (static) functions.
  std::unordered_set<std::string> internal_fns;
  for (const auto& f : mod.functions) {
    if (f.is_internal) internal_fns.insert(f.name);
  }

  std::unordered_set<std::string> reachable;
  std::vector<std::string> worklist;

  // Seed: all non-internal function bodies are reachable roots.
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    if (!mod.functions[i].is_internal) {
      for (const auto& r : find_refs(rendered_bodies[i])) {
        if (internal_fns.count(r) && !reachable.count(r)) {
          reachable.insert(r);
          worklist.push_back(r);
        }
      }
    }
  }

  // Also treat global initializers as roots.
  for (const auto& g : mod.globals) {
    for (const auto& r : find_refs(g.raw_line)) {
      if (internal_fns.count(r) && !reachable.count(r)) {
        reachable.insert(r);
        worklist.push_back(r);
      }
    }
  }

  // Propagate: internal functions referenced by reachable internal functions.
  std::unordered_map<std::string, size_t> fn_idx_by_name;
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    fn_idx_by_name[mod.functions[i].name] = i;
  }
  while (!worklist.empty()) {
    std::string cur = std::move(worklist.back());
    worklist.pop_back();
    auto it = fn_idx_by_name.find(cur);
    if (it == fn_idx_by_name.end()) continue;
    for (const auto& r : find_refs(rendered_bodies[it->second])) {
      if (internal_fns.count(r) && !reachable.count(r)) {
        reachable.insert(r);
        worklist.push_back(r);
      }
    }
  }

  // Render final output.
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
  for (const auto& g : mod.globals) out << g.raw_line << "\n";

  if (!mod.type_decls.empty() || !mod.string_pool.empty() || !mod.globals.empty())
    out << "\n";

  // Intrinsic declarations.
  if (mod.need_va_start)    out << "declare void @llvm.va_start.p0(ptr)\n";
  if (mod.need_va_end)      out << "declare void @llvm.va_end.p0(ptr)\n";
  if (mod.need_va_copy)     out << "declare void @llvm.va_copy.p0.p0(ptr, ptr)\n";
  if (mod.need_memcpy)      out << "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n";
  if (mod.need_stacksave)   out << "declare ptr @llvm.stacksave()\n";
  if (mod.need_stackrestore) out << "declare void @llvm.stackrestore(ptr)\n";
  if (mod.need_abs) {
    out << "declare i32 @llvm.abs.i32(i32, i1 immarg)\n";
    out << "declare i64 @llvm.abs.i64(i64, i1 immarg)\n";
  }
  if (mod.need_va_start || mod.need_va_end || mod.need_va_copy ||
      mod.need_memcpy || mod.need_stacksave || mod.need_stackrestore ||
      mod.need_abs) out << "\n";

  // External function declarations.
  for (const auto& ed : mod.extern_decls) {
    out << "declare " << ed.return_type_str << " " << llvm_global_sym(ed.name) << "(...)\n";
  }
  if (!mod.extern_decls.empty()) out << "\n";

  // Function bodies.
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    // Skip unreachable internal functions.
    if (mod.functions[i].is_internal && !reachable.count(mod.functions[i].name)) continue;
    out << rendered_bodies[i];
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
