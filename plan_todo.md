# HIR → LIR Split — Execution State

## Current Step: Step 6g (move StmtEmitter from llvm/ to lir/) — DONE

## Step 1 Audit: Legacy Dependencies in hir_to_lir.cpp

### Semantic lowering dependencies (real problems):
1. ~~**`emitter.lower_globals(global_indices)`**~~ — **DONE** (global lowering moved to hir_to_lir; LirGlobal structured with typed fields; raw_line removed)
2. ~~**`emitter.emit_stmt(ctx, stmt)`**~~ — **DONE** (StmtEmitter relocated to lir/ namespace; no longer a cross-directory dependency)
3. ~~**`emitter.lower_single_function(fn, sig)`**~~ — **DONE** (declaration lowering moved to hir_to_lir; method removed from HirEmitter)

### Module ownership / orchestration:
4. ~~`emitter.adopt_module()` / `emitter.release_module()` / `emitter.push_lir_function()`~~ — **DONE** (replaced with reference pattern)

### Type/helper reuse (printer-level formatting used during lowering):
5. `llvm_backend::detail::*` helpers used throughout: `llvm_ty()`, `llvm_ret_ty()`, `llvm_alloca_ty()`, `sanitize_llvm_ident()`, `llvm_struct_type_str()`, `llvm_field_ty()`, `llvm_global_sym()`, `llvm_visibility()`, `set_active_target_triple()`, `fp_literal()`, `is_float_base()`, `is_complex_base()`, `quote_llvm_ident()`, `llvm_default_datalayout()`, `llvm_va_list_is_pointer_object()`

### Summary:
- Items 1-4 eliminated: all semantic lowering now owned by lir/ namespace
- Item 5 is acceptable for now (shared helpers, not semantic ownership)
- StmtEmitter is now `c4c::codegen::lir::StmtEmitter` in `src/codegen/lir/stmt_emitter.hpp/.cpp`

## Completed Items
- [x] Step 1: Audit current legacy dependencies
- [x] Step 2a: Extract `emit_lbl()` and `block_lbl()` into hir_to_lir (items 3-4 from original audit)
- [x] Step 2b: Replace adopt/release module pattern with reference-based `set_module()`
- [x] Step 2c: Extract declaration lowering (`lower_single_function`) into hir_to_lir; removed dead method from HirEmitter
- [x] Step 2d: Move global variable lowering from HirEmitter to hir_to_lir; structured LirGlobal (linkage_vis, qualifier, llvm_type, init_text, align_bytes, is_extern_decl); removed raw_line; emit_const_init/emit_const_struct_fields made public on HirEmitter
- [x] Step 3a: Replace LirRawLine alloca instructions in hoist_allocas() with typed LirAlloca
- [x] Step 3b: Replace LirRawLine store instructions in hoist_allocas() with typed LirHoistedStore (param stores + zeroinit stores)
- [x] Step 3c: Replace LirRawTerminator in inject_fallthrough_returns() with typed LirRet
- [x] Step 3d: Replace all LirRawTerminator usage in hir_emitter.cpp with typed LIR terminators (LirBr, LirCondBr, LirRet, LirSwitch, LirUnreachable); removed dead `emit_term()` method
- [x] Step 3e-i: Replace extractvalue/insertvalue LirRawLine with typed LirExtractValueOp/LirInsertValueOp (31 call sites)
- [x] Step 3e-ii: Replace load/store LirRawLine with typed LirLoadOp/LirStoreOp (46 call sites: 29 loads, 17 stores)
- [x] Step 3e-iii: Replace cast LirRawLine with typed LirCastOp (41 call sites: zext, sext, trunc, fpext, fptrunc, fptosi, fptoui, sitofp, uitofp, ptrtoint, inttoptr, bitcast)
- [x] Step 3e-iv: Replace GEP LirRawLine with typed LirGepOp (28 call sites: struct member, array decay, ptr arithmetic, complex real/imag, vaarg aarch64)
- [x] Step 3e-v: Replace call LirRawLine with typed LirCallOp (16 call sites: 2 general function calls + 14 intrinsic calls)
- [x] Step 3e-vi: Replace remaining direct LirRawLine alloca_insts in legacy emitter with LirAlloca/LirHoistedStore (9 sites: param spills, agg temps, local decl allocas, VLA ptr slots, zeroinit stores)
- [x] Step 3e-vii: Replace emit_instr LirRawLine with typed ops — LirBinOp (40 sites), LirCmpOp (18 sites), LirPhiOp (4 sites), LirSelectOp (1 site), LirInsertElementOp (2 sites), LirExtractElementOp (1 site), LirShuffleVectorOp (2 sites), LirVaArgOp (2 sites)
- [x] Step 3e-viii: Replace remaining emit_instr alloca calls with typed LirAllocaOp and inline asm calls with typed LirInlineAsmOp; removed dead `emit_instr()` method
- [x] Step 3-final: Remove dead LirRawLine and LirRawTerminator types from LIR variant and printer (zero producers remained)

## Completed (Step 4)
- [x] Step 4a: Expand LirBitfieldExtract into typed LIR ops (LirLoadOp, LirBinOp, LirCastOp) in emit_bitfield_load; remove compound type from ir.hpp and printer
- [x] Step 4a: Expand LirBitfieldInsert into typed LIR ops (LirLoadOp, LirBinOp, LirStoreOp) in emit_bitfield_store; remove compound type from ir.hpp and printer
- [x] Step 4b: Replace LirAlloca (TypeSpec-based) with LirAllocaOp (string-based) and LirHoistedStore with LirStoreOp at all 11 production sites; pre-compute LLVM type strings in lowering; remove dead types from ir.hpp variant and printer
- [x] Step 4c: Move dead internal function elimination (DCE) from lir_printer.cpp into hir_to_lir.cpp as eliminate_dead_internals() module-level pass; printer now purely renders all functions it receives

## Completed (Step 5)
- [x] Step 5: llvm_codegen.cpp is already a pure path switcher (legacy/lir/compare); no lowering logic present

## Completed (Step 6)
- [x] Step 6a: Remove dead HirEmitter methods — `emit()`, `lower_to_lir()`, `lower_globals()`, `emit_global()`, `emit_function()` all removed; llvm_codegen.cpp now routes all codegen paths through lir::lower+lir::print_llvm directly; removed hir_emitter.hpp include from llvm_codegen.cpp
- [x] Step 6b: Remove dead `HirEmitter::hoist_allocas()`, `find_modified_params()`, `fn_has_vla_locals()` — all three only called from removed `emit_function()`; live copies exist in hir_to_lir.cpp; also removed unused `<unordered_set>` include
- [x] Step 6c: Remove dead HirEmitter data members — `need_llvm_stacksave_` (never written), `inferred_ret_fn_ptr_sigs_` (never used), `spec_entries_`/`SpecEntry` (never written); made `emit_lbl`/`block_lbl` private; removed dead `needs_stacksave()` getter and `spec_entries()` getter+loop from hir_to_lir.cpp (stacksave already detected via `any_vla`, spec_entries already collected directly)
- [x] Step 6d: Move intrinsic flags and extern_call_decls from HirEmitter to LirModule — emit_stmt now writes `module_->need_*` directly; `record_extern_call_decl` delegates to `module_->record_extern_decl()`; removed 6 bool members, `extern_call_decls_` map, and 7 getter methods from HirEmitter; `finalize_module` simplified to just convert dedup map to vector
- [x] Step 6e: Move string pool from HirEmitter to LirModule — `str_pool_` dedup map and `str_idx_` counter moved to `LirModule::str_pool_map`/`str_pool_idx`; `intern_str()` implemented on LirModule; HirEmitter::intern_str() now delegates to `module_->intern_str()`; wide string pool name generation uses `module_->str_pool_idx`; removed 2 data members from HirEmitter
- [x] Step 6f: Move const-init lowering from HirEmitter to standalone `lir::ConstInitEmitter` — created `const_init_emitter.hpp/.cpp`; moved `emit_const_init`, `emit_const_struct_fields`, `emit_const_scalar_expr`, `try_const_eval_*` (~660 lines) out of HirEmitter; hir_to_lir.cpp now uses ConstInitEmitter directly for global lowering; deleted `hir_to_llvm_const_init.cpp`; removed const-init declarations from hir_emitter.hpp; HirEmitter public interface reduced to: `set_module()`, `emit_stmt()`
- [x] Step 6g: Move HirEmitter from `src/codegen/llvm/` to `src/codegen/lir/stmt_emitter.hpp/.cpp` — renamed class to `StmtEmitter`, namespace to `c4c::codegen::lir`; hir_to_lir.cpp now uses `StmtEmitter` directly without cross-directory include; `src/codegen/llvm/` reduced to orchestration (`llvm_codegen.hpp/cpp`) and shared helpers (`hir_to_llvm_helpers.hpp`)
- [x] Step 6h: Move `hir_to_llvm_helpers.hpp` from `src/codegen/llvm/` to `src/codegen/shared/llvm_helpers.hpp` — all 4 consumers updated to new path; old file deleted; `src/codegen/llvm/` now contains only orchestration (llvm_codegen.hpp/cpp); fixed stale namespace closing comment typo

## Active Slice
- Step 6h: Move llvm_helpers to shared/ — DONE

## Next Intended Slice
- Begin decomposing StmtEmitter into smaller focused units (expr_emitter, type_resolver, etc.)
- Or: rename namespace `llvm_backend::detail` to something more neutral (e.g. `codegen::llvm_helpers`)

## Current file layout

### src/codegen/llvm/ (orchestration only)
- `llvm_codegen.hpp/cpp` — path switcher, calls lir::lower + lir::print_llvm

### src/codegen/lir/ (all lowering + printing)
- `ir.hpp` — LIR data model
- `hir_to_lir.hpp/cpp` — module-level lowering orchestration
- `stmt_emitter.hpp/cpp` — statement/expression lowering (~4100 lines)
- `const_init_emitter.hpp/cpp` — constant initializer lowering
- `lir_printer.hpp/cpp` — LLVM IR text rendering

### src/codegen/shared/
- `fn_lowering_ctx.hpp` — FnCtx, BlockMeta shared state
- `llvm_helpers.hpp` — LLVM type string helpers (llvm_ty, llvm_alloca_ty, sanitize_llvm_ident, etc.)

## Raw fallback usage remaining
- **All raw fallback types: REMOVED** — LirRawLine, LirRawTerminator, LirBitfieldExtract, LirBitfieldInsert, LirAlloca (TypeSpec), LirHoistedStore, LirGlobal::raw_line all deleted
- All LIR instructions, terminators, and globals are now fully typed; printer has no TypeSpec→LLVM type conversions
- **Printer DCE: REMOVED** — dead internal function elimination moved to lowering (eliminate_dead_internals)

## Blockers
- None
