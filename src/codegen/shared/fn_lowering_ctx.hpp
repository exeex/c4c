#pragma once

// ── Per-function lowering context ─────────────────────────────────────────────
//
// FnCtx holds mutable state that accumulates while lowering a single HIR
// function into LIR blocks.  It is consumed by both hir_to_lir (which owns
// FnCtx lifetime and alloca hoisting) and HirEmitter (which uses it for
// statement / expression lowering).

#include "../lir/ir.hpp"
#include "../../frontend/hir/ir.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::codegen {

struct BlockMeta {
  std::optional<std::string> break_label;
  std::optional<std::string> continue_label;
};

struct FnCtx {
  const c4c::hir::Function* fn = nullptr;
  int tmp_idx = 0;
  bool last_term = false;
  // local_id.value → alloca slot (e.g. "%lv.x")
  std::unordered_map<uint32_t, std::string> local_slots;
  // local_id.value → C TypeSpec
  std::unordered_map<uint32_t, TypeSpec> local_types;
  // local_id.value → true if this local is a VLA allocated dynamically at runtime
  std::unordered_map<uint32_t, bool> local_is_vla;
  // param_index → SSA name (e.g. "%p.x")
  std::unordered_map<uint32_t, std::string> param_slots;
  // Structured LIR blocks (replaces body_lines).
  std::vector<lir::LirBlock> lir_blocks;
  size_t current_block_idx = 0;
  lir::LirBlock& cur_block() { return lir_blocks[current_block_idx]; }
  // Hoisted alloca instructions (rendered before entry block body).
  std::vector<lir::LirInst> alloca_insts;
  // legacy per-block metadata (kept for compatibility; mostly unused now)
  std::unordered_map<uint32_t, BlockMeta> block_meta;
  // body_block -> continue branch target label
  std::unordered_map<uint32_t, std::string> continue_redirect;
  // user label name → LLVM label
  std::unordered_map<std::string, std::string> user_labels;
  // local_id.value / param_index / global_id.value → fn-ptr signature metadata.
  std::unordered_map<uint32_t, c4c::hir::FnPtrSig> local_fn_ptr_sigs;
  std::unordered_map<uint32_t, c4c::hir::FnPtrSig> param_fn_ptr_sigs;
  std::unordered_map<uint32_t, c4c::hir::FnPtrSig> global_fn_ptr_sigs;
  // Per-function stacksave pointer for VLA scope rewinds.
  std::optional<std::string> vla_stack_save_ptr;
  // Block currently being emitted (for backward-goto detection).
  uint32_t current_block_id = 0;
};

}  // namespace c4c::codegen
