# LIR De-Stringify Legacy-Safe Refactor Todo

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Define the first typed LIR primitives slice.
- Exact target for this iteration: turn the Step 1 audit into concrete type
  candidates for operands, type references, and enum-backed binary/cmp
  semantics before changing emitters or printers.

## Completed Items

- Switched the active plan from
  `ideas/open/35_backend_ready_ir_contract_plan.md` to
  `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.
- Parked `35` after confirming that backend-ready IR work should not proceed on
  top of the current string-heavy LIR contract.
- Completed Step 1 audit of the current string-heavy LIR surface and recorded
  the main typed-operand, typed-type, enum-like, and textual-only buckets.

## Notes

- Step 1 audit summary:
  `src/codegen/lir/ir.hpp` still mixes early typed nodes such as `LirLoad`,
  `LirStore`, `LirBinary`, `LirCmp`, and `LirCall` with later LLVM-shaped
  string protocol nodes such as `LirLoadOp`, `LirStoreOp`, `LirBinOp`,
  `LirCmpOp`, `LirCallOp`, `LirMemcpyOp`, `LirVa*Op`, `LirAbsOp`,
  `LirIndirectBrOp`, `LirExtractValueOp`, and `LirInsertValueOp`.
- Typed operand debt:
  `LirMemcpyOp::{dst,src,size}`, `LirVaStartOp::ap_ptr`,
  `LirVaEndOp::ap_ptr`, `LirVaCopyOp::{dst_ptr,src_ptr}`,
  `LirStackSaveOp::result`, `LirStackRestoreOp::saved_ptr`,
  `LirAbsOp::{result,arg}`, `LirIndirectBrOp::addr`,
  `LirExtractValueOp::{result,agg}`, `LirInsertValueOp::{result,agg,elem}`,
  `LirLoadOp::{result,ptr}`, `LirStoreOp::{val,ptr}`, `LirCastOp::{result,operand}`,
  `LirGepOp::{result,ptr}` plus parsed `indices`, `LirCallOp::callee` plus
  parsed `args_str`, `LirBinOp::{result,lhs,rhs}`, `LirCmpOp::{result,lhs,rhs}`,
  `LirPhiOp::incoming`, `LirSelectOp::{cond,true_val,false_val}`,
  vector element ops, `LirVaArgOp::ap_ptr`, `LirAllocaOp::count`, and
  `LirRet::value_str`.
- Typed type debt:
  `LirAbsOp::int_type`, `LirExtractValueOp::agg_type`,
  `LirInsertValueOp::{agg_type,elem_type}`, `LirLoadOp::type_str`,
  `LirStoreOp::type_str`, `LirCastOp::{from_type,to_type}`,
  `LirGepOp::element_type`, `LirCallOp::return_type`,
  `LirPhiOp::type_str`, `LirSelectOp::type_str`,
  vector ops `vec_type` and `elem_type`, `LirVaArgOp::type_str`,
  `LirAllocaOp::type_str`, `LirInlineAsmOp::ret_type`,
  `LirRet::type_str`, and `LirSwitch::selector_type`.
- Enum-like semantic debt:
  `LirBinOp::opcode` carries arithmetic/logical opcode selection,
  `LirCmpOp::{is_float,predicate}` carry comparison family and predicate,
  `LirCallOp` currently encodes direct-vs-indirect-vs-intrinsic call shape via
  `callee` and `callee_type_suffix` text instead of an explicit call kind, and
  `LirCastOp::kind` is already the model to copy for other migrated families.
- Legitimate long-term textual payloads:
  `LirInlineAsmOp::{asm_text,constraints}` should remain textual.
  Module/global text such as `LirGlobal::{llvm_type,init_text}` and
  declaration/signature text are lower priority and are not the immediate
  blockers for this runbook.
- Printer dependencies:
  `src/codegen/lir/lir_printer.cpp` renders the audited families by directly
  concatenating their raw string fields, so printer compatibility shims will be
  required during migration.
- Construction sites:
  `src/codegen/lir/stmt_emitter.cpp` builds `LirCallOp` with preformatted
  `args_str` and `callee_type_suffix`, emits builtin calls by embedding typed
  operands into argument text, and emits `LirBinOp`/`LirCmpOp` with raw opcode
  and predicate strings.
- Backend consumers depending on string protocols:
  `src/backend/lir_adapter.cpp` branches on `bin->opcode`, `cmp->predicate`,
  `load/store->type_str`, and exact `call->args_str` / `call->callee_type_suffix`
  shapes. `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp` do the same for cmp predicates, call
  signatures, and argument parsing.
- Analysis consumers depending on textual operand scraping:
  `src/backend/stack_layout/analysis.cpp`,
  `src/backend/stack_layout/alloca_coalescing.cpp`, and
  `src/backend/liveness.cpp` mine values out of `args_str`, GEP index text, and
  other string operands instead of consuming typed operand lists.
- Step 2 candidate primitives implied by the audit:
  `LirOperand` should distinguish SSA values, immediates, globals, labels, and
  special constants; `LirTypeRef` should replace raw LLVM type strings on core
  ops; `enum class LirBinaryOpcode` and `enum class LirCmpPredicate` should
  replace string dispatch first; and `LirCallOp` likely needs typed callee plus
  structured argument records before backend call-family cleanup can proceed.

## Next Intended Slice

- Sketch `operands.hpp` and `types.hpp` around the audited buckets without
  migrating construction sites yet.
- Start with the narrowest high-value enums and wrappers:
  binary opcode, cmp predicate, typed operand, typed type reference.
- Leave inline asm text and module-level declaration text untouched in the
  first primitive pass.

## Validation

- `cmake -S . -B build`
- `cmake --build build -j8`
- `ctest --test-dir build -j --output-on-failure > test_fail_before.log`
  recorded `2371/2560` passing and `189` failing tests.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded the same `2371/2560` passing and `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures and zero new
  suspicious slow tests.

## Open Questions To Resolve During This Runbook

- Should typed LIR operands distinguish immediates, SSA values, globals, labels,
  and special tokens like `zeroinitializer` as separate variants?
- Should typed LIR types wrap existing LLVM-ish type text temporarily, or map
  directly onto a richer shared type representation?
- Which instruction families should migrate first to maximize consumer cleanup:
  arithmetic/cmp, load/store, or call-adjacent ops?
- Which residual textual payloads are legitimate long-term text, such as inline
  asm source, versus temporary compatibility debt?

## Blockers

- None yet. The immediate task is the migration audit and typed primitive
  design, not backend-ready IR or emitter work.
