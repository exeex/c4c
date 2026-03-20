#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "../llvm/hir_emitter.hpp"
#include "../llvm/hir_to_llvm_helpers.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <variant>

namespace c4c::codegen::lir {

// ── Module-level orchestration helpers ───────────────────────────────────────

std::vector<size_t> dedup_globals(const c4c::hir::Module& mod) {
  std::unordered_map<std::string, size_t> best; // name -> index
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    auto it = best.find(gv.name);
    if (it == best.end()) {
      best[gv.name] = i;
    } else {
      const bool cur_has_init = !std::holds_alternative<std::monostate>(mod.globals[it->second].init);
      const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
      if (new_has_init || !cur_has_init) it->second = i;
    }
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best.size());
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    auto it = best.find(mod.globals[i].name);
    if (it != best.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<size_t> dedup_functions(const c4c::hir::Module& mod) {
  std::unordered_map<std::string, size_t> best; // name -> index
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    auto it = best.find(fn.name);
    if (it == best.end()) {
      best[fn.name] = i;
    } else {
      const bool cur_is_def = !mod.functions[it->second].blocks.empty();
      const bool new_is_def = !fn.blocks.empty();
      if (new_is_def && !cur_is_def) it->second = i;
    }
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    if (!mod.functions[i].materialized) continue;
    auto it = best.find(mod.functions[i].name);
    if (it != best.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<std::string> build_type_decls(const c4c::hir::Module& mod) {
  using namespace c4c::codegen::llvm_backend::detail;
  std::vector<std::string> decls;

  if (!llvm_va_list_is_pointer_object(mod.target_triple)) {
    decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  }

  for (const auto& tag : mod.struct_def_order) {
    const auto it = mod.struct_defs.find(tag);
    if (it == mod.struct_defs.end()) continue;
    const auto& sd = it->second;
    const std::string sty = llvm_struct_type_str(tag);

    if (sd.fields.empty()) {
      decls.push_back(sty + " = type {}");
      continue;
    }
    if (sd.is_union) {
      decls.push_back(sty + " = type { [" +
                       std::to_string(sd.size_bytes) + " x i8] }");
    } else {
      std::ostringstream line;
      line << sty << " = type { ";
      bool first = true;
      int last_idx = -1;
      int cur_offset = 0;
      for (const auto& f : sd.fields) {
        if (f.llvm_idx == last_idx) continue;
        last_idx = f.llvm_idx;
        if (f.offset_bytes > cur_offset) {
          if (!first) line << ", ";
          first = false;
          line << "[" << (f.offset_bytes - cur_offset) << " x i8]";
          cur_offset = f.offset_bytes;
        }
        if (!first) line << ", ";
        first = false;
        line << llvm_field_ty(f);
        cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
      }
      if (sd.size_bytes > cur_offset) {
        if (!first) line << ", ";
        line << "[" << (sd.size_bytes - cur_offset) << " x i8]";
      }
      line << " }";
      decls.push_back(line.str());
    }
  }
  return decls;
}

// ── Module-level finalization ─────────────────────────────────────────────────
// After per-item lowering, the emitter holds accumulated intrinsic flags,
// extern call declarations, and spec entries.  This function transfers them
// into the LirModule so the printer / future backends can consume them.

static void finalize_module(LirModule& module,
                            const c4c::hir::Module& hir_mod,
                            const c4c::codegen::llvm_backend::HirEmitter& emitter) {
  // Intrinsic requirement flags
  if (emitter.needs_va_start())     module.need_va_start = true;
  if (emitter.needs_va_end())       module.need_va_end = true;
  if (emitter.needs_va_copy())      module.need_va_copy = true;
  if (emitter.needs_memcpy())       module.need_memcpy = true;
  if (emitter.needs_stacksave())    module.need_stacksave = true;
  if (emitter.needs_stackrestore()) module.need_stackrestore = true;
  if (emitter.needs_abs())          module.need_abs = true;

  // Extern call declarations (functions called but not defined in this TU)
  for (const auto& [name, ret_ty] : emitter.extern_call_decls()) {
    if (hir_mod.fn_index.count(name)) continue;
    LirExternDecl ed;
    ed.name = name;
    ed.return_type_str = ret_ty;
    module.extern_decls.push_back(std::move(ed));
  }

  // Specialization metadata
  for (const auto& e : emitter.spec_entries()) {
    module.spec_entries.push_back({e.spec_key, e.template_origin, e.mangled_name});
  }
}

// ── Main lowering entry point ────────────────────────────────────────────────

LirModule lower(const c4c::hir::Module& hir_mod) {
  using namespace c4c::codegen::llvm_backend::detail;

  // Module-level orchestration: owned by hir_to_lir, not HirEmitter.
  set_active_target_triple(hir_mod.target_triple);

  LirModule module;
  module.target_triple = hir_mod.target_triple;
  module.data_layout = !hir_mod.data_layout.empty()
      ? hir_mod.data_layout
      : llvm_default_datalayout(hir_mod.target_triple);
  module.type_decls = build_type_decls(hir_mod);

  auto global_indices = dedup_globals(hir_mod);
  auto fn_indices = dedup_functions(hir_mod);

  // Delegate per-item lowering to HirEmitter (will migrate in later phases).
  c4c::codegen::llvm_backend::HirEmitter emitter(hir_mod);
  emitter.lower_items(module, global_indices, fn_indices);

  // Module-level finalization: owned by hir_to_lir.
  finalize_module(module, hir_mod, emitter);

  return module;
}

}  // namespace c4c::codegen::lir
