# HIR → LIR Split — Execution State

## Current Step: Step 4 (Printer Purity Audit) — in progress

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
- [x] Step 3e-vi: Replace remaining direct LirRawLine alloca_insts in legacy emitter with LirAlloca/LirHoistedStore (9 sites: param spills, agg temps, local decl allocas, VLA ptr slots, zeroinit stores)
- [x] Step 3e-vii: Replace emit_instr LirRawLine with typed ops — LirBinOp (40 sites), LirCmpOp (18 sites), LirPhiOp (4 sites), LirSelectOp (1 site), LirInsertElementOp (2 sites), LirExtractElementOp (1 site), LirShuffleVectorOp (2 sites), LirVaArgOp (2 sites)
- [x] Step 3e-viii: Replace remaining emit_instr alloca calls with typed LirAllocaOp and inline asm calls with typed LirInlineAsmOp; removed dead `emit_instr()` method
- [x] Step 3-final: Remove dead LirRawLine and LirRawTerminator types from LIR variant and printer (zero producers remained)

## Active Slice
- Step 4c: Move dead static function elimination from printer to lowering — DONE

## Completed (Step 4)
- [x] Step 4a: Expand LirBitfieldExtract into typed LIR ops (LirLoadOp, LirBinOp, LirCastOp) in emit_bitfield_load; remove compound type from ir.hpp and printer
- [x] Step 4a: Expand LirBitfieldInsert into typed LIR ops (LirLoadOp, LirBinOp, LirStoreOp) in emit_bitfield_store; remove compound type from ir.hpp and printer
- [x] Step 4b: Replace LirAlloca (TypeSpec-based) with LirAllocaOp (string-based) and LirHoistedStore with LirStoreOp at all 11 production sites; pre-compute LLVM type strings in lowering; remove dead types from ir.hpp variant and printer
- [x] Step 4c: Move dead internal function elimination (DCE) from lir_printer.cpp into hir_to_lir.cpp as eliminate_dead_internals() module-level pass; printer now purely renders all functions it receives

## Next Intended Slice
- Step 5: Reduce llvm_codegen.cpp to orchestration only
- Step 2 remaining: extract lower_globals and emit_stmt semantic ownership from HirEmitter

## Raw fallback usage remaining
- **LirRawLine: REMOVED** — type deleted from variant, no producers existed
- **LirRawTerminator: REMOVED** — type deleted from variant, no producers existed
- **LirBitfieldExtract: REMOVED** — expanded to typed LIR ops in lowering
- **LirBitfieldInsert: REMOVED** — expanded to typed LIR ops in lowering
- **LirAlloca: REMOVED** — replaced with LirAllocaOp (string-based, type computed in lowering)
- **LirHoistedStore: REMOVED** — replaced with LirStoreOp (string-based, type computed in lowering)
- All LIR instructions and terminators are now fully typed; printer has no TypeSpec→LLVM type conversions
- **Printer DCE: REMOVED** — dead internal function elimination moved to lowering (eliminate_dead_internals)

## Blockers
- None
