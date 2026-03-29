# LIR De-Stringify Legacy-Safe Refactor Todo

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Add verification and legacy-safe compatibility shims.
- Exact target for the next iteration: build printer-side and verification-side
  helpers around the new typed operand/type/opcode wrappers so malformed typed
  payloads can be rejected without breaking the current legacy LLVM text path.

## Completed Items

- Switched the active plan from
  `ideas/open/35_backend_ready_ir_contract_plan.md` to
  `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.
- Parked `35` after confirming that backend-ready IR work should not proceed on
  top of the current string-heavy LIR contract.
- Completed Step 1 audit of the current string-heavy LIR surface and recorded
  the main typed-operand, typed-type, enum-like, and textual-only buckets.
- Completed the first Step 2 primitive pass by adding compatibility-first
  `LirOperand`, `LirTypeRef`, `LirBinaryOpcodeRef`, and `LirCmpPredicateRef`
  wrappers and wiring the high-friction legacy LIR ops onto them without
  changing emitter or backend call sites.

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
- Step 2 implementation landed as compatibility shims instead of a hard cutover:
  `src/codegen/lir/operands.hpp` classifies basic operand kinds while
  preserving string mutability and printing for existing consumers, and
  `src/codegen/lir/types.hpp` classifies LLVM-ish types plus enum-backed binary
  and cmp semantics while still round-tripping unknown text for legacy sites.
- `src/codegen/lir/ir.hpp` now uses those wrappers for the string-heavy legacy
  op families audited in Step 1, which means later migrations can consume typed
  metadata without first rewriting every emitter/backend call site.
- `tests/backend/backend_lir_adapter_tests.cpp` now covers basic operand/type
  classification, known enum mapping, and unknown-text compatibility for the
  new wrappers.

## Next Intended Slice

- Add verifier/printer helpers that consume `LirOperand`, `LirTypeRef`,
  `LirBinaryOpcodeRef`, and `LirCmpPredicateRef` directly instead of relying on
  ad hoc string concatenation in each call site.
- Keep inline asm text and module-level declaration text untouched while
  Step 3 focuses on typed-core op validation and rendering.
- Decide whether `LirOperand` should gain explicit structured variants for
  typed call arguments and GEP indices, or whether that belongs in the later
  call-family migration slice.

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
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  wrapper layer grew enough string-compatibility operators to keep existing
  backend consumers compiling unchanged.
- `./build/backend_lir_adapter_tests` passed with added coverage for the new
  typed wrapper layer.

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

- None. The next task is Step 3 printer/verification shims on top of the new
  compatibility-first typed wrapper layer.
