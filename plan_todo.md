# HIR → LIR Split — Execution State

## Current Step: Step 3 (Replace Raw LLVM-Text Fallback LIR Where It Hurts Most)

## Step 1 Audit: Legacy Dependencies in hir_to_lir.cpp

### Semantic lowering dependencies (real problems):
1. **`emitter.lower_globals(global_indices)`** — entire global lowering delegated to HirEmitter
2. **`emitter.emit_stmt(ctx, stmt)`** — entire statement/expression lowering delegated to HirEmitter
3. ~~**`emitter.lower_single_function(fn, sig)`**~~ — **DONE** (declaration lowering moved to hir_to_lir; method removed from HirEmitter)

### Module ownership / orchestration:
4. ~~`emitter.adopt_module()` / `emitter.release_module()` / `emitter.push_lir_function()`~~ — **DONE** (replaced with reference pattern)

### Type/helper reuse (printer-level formatting used during lowering):
5. `llvm_backend::detail::*` helpers used throughout: `llvm_ty()`, `llvm_ret_ty()`, `llvm_alloca_ty()`, `sanitize_llvm_ident()`, `llvm_struct_type_str()`, `llvm_field_ty()`, `llvm_global_sym()`, `llvm_visibility()`, `set_active_target_triple()`, `fp_literal()`, `is_float_base()`, `is_complex_base()`, `quote_llvm_ident()`, `llvm_default_datalayout()`, `llvm_va_list_is_pointer_object()`

### Summary:
- Items 1-2 are the remaining semantic dependencies
- Items 3-4 are eliminated
- Item 5 is acceptable for now (shared helpers, not semantic ownership)

## Completed Items
- [x] Step 1: Audit current legacy dependencies
- [x] Step 2a: Extract `emit_lbl()` and `block_lbl()` into hir_to_lir (items 3-4 from original audit)
- [x] Step 2b: Replace adopt/release module pattern with reference-based `set_module()`
- [x] Step 2c: Extract declaration lowering (`lower_single_function`) into hir_to_lir; removed dead method from HirEmitter
- [x] Step 3a: Replace LirRawLine alloca instructions in hoist_allocas() with typed LirAlloca
- [x] Step 3b: Replace LirRawLine store instructions in hoist_allocas() with typed LirHoistedStore (param stores + zeroinit stores)
- [x] Step 3c: Replace LirRawTerminator in inject_fallthrough_returns() with typed LirRet
- [x] Step 3d: Replace all LirRawTerminator usage in hir_emitter.cpp with typed LIR terminators (LirBr, LirCondBr, LirRet, LirSwitch, LirUnreachable); removed dead `emit_term()` method
- [x] Step 3e-i: Replace extractvalue/insertvalue LirRawLine with typed LirExtractValueOp/LirInsertValueOp (31 call sites)
- [x] Step 3e-ii: Replace load/store LirRawLine with typed LirLoadOp/LirStoreOp (46 call sites: 29 loads, 17 stores)
- [x] Step 3e-iii: Replace cast LirRawLine with typed LirCastOp (41 call sites: zext, sext, trunc, fpext, fptrunc, fptosi, fptoui, sitofp, uitofp, ptrtoint, inttoptr, bitcast)
- [x] Step 3e-iv: Replace GEP LirRawLine with typed LirGepOp (28 call sites: struct member, array decay, ptr arithmetic, complex real/imag, vaarg aarch64)
- [x] Step 3e-v: Replace call LirRawLine with typed LirCallOp (16 call sites: 2 general function calls + 14 intrinsic calls)

## Active Slice
- (none — ready for next iteration)

## Next Intended Slice
- Step 3e-vi: Replace remaining emit_instr categories (alloca, icmp/fcmp, select, binary ops, phi, etc.)
- Or: Extract `lower_globals` into hir_to_lir (Step 2 semantic dependency)

## Raw fallback usage remaining
- hir_to_lir.cpp: no raw terminator usage remains
- hir_emitter.cpp: emit_instr() still uses LirRawLine for ~112 instructions (162 converted: 31 extractvalue/insertvalue + 46 load/store + 41 cast + 28 GEP + 16 call)
- LirRawTerminator: only produced by no remaining call sites; type kept in variant for now

## Blockers
- None
