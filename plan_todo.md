# LIR De-Stringify Legacy-Safe Refactor Todo

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Source Plan: plan.md

## Active Item

- Step 4: Migrate high-friction instruction families and consumers.
- Exact target for the next iteration: continue Step 4 past
  the landed backend-owned shared call operand migration into the remaining
  LIR-side construction/printing seams that still stitch together call
  compatibility text directly instead of consuming one shared structured call
  view end to end.
- Iteration focus: continue Step 4 in
  `src/codegen/lir/stmt_emitter.cpp`,
  `src/codegen/lir/lir_printer.cpp`, and any adjacent LIR-side call helpers so
  structured typed call metadata, including indirect callees and rewritten
  operands, can flow through one shared formatter/parser seam without each site
  rebuilding `callee_type_suffix` / `args_str` handling.
- Exact target for the next iteration after this slice: continue Step 4 into
  the remaining LIR-side call and serialization paths that still treat typed
  call metadata as ad hoc text, especially any residual `LirCallOp`
  construction or persistence seams outside the shared formatter/parser helper
  layer.

## Completed Items

- Completed the next Step 4 LIR-side shared call-formatting slice by adding
  `FormattedLirTypedCall`, `format_lir_typed_call(...)`, and
  `format_lir_call_site(...)` to `src/codegen/lir/call_args.hpp`, routing
  `src/codegen/lir/lir_printer.cpp` through the shared full-call formatter for
  canonical typed indirect-call rendering, replacing the remaining builtin and
  intrinsic typed-call argument string stitching in
  `src/codegen/lir/stmt_emitter.cpp` with shared structured call formatting,
  and adding focused regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp` for full-call helper
  round-tripping plus canonical printer output.
- Completed the next Step 4 backend-owned call representation cleanup slice by
  replacing `src/backend/lir_adapter.hpp`'s raw `BackendCallInst`
  `callee_type_suffix` / `args_str` storage with structured owned parameter and
  argument metadata plus a legacy render-shape flag, routing
  `src/backend/lir_adapter.cpp` through shared typed-call parsing with legacy
  direct-call parameter inference when the source omitted an explicit suffix,
  updating the x86 and AArch64 backend readers to consume the new shared
  backend call view instead of reparsing raw adapter text, and adding focused
  regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` for the
  backend-owned helper view plus canonical backend call rendering.
- Completed the next Step 4 adapter local-call typed-view cleanup slice by
  deleting `src/backend/lir_adapter.cpp`'s private typed-call parser wrapper,
  adding shared typed-call ownership and parameter-format helpers in
  `src/codegen/lir/call_args.hpp`, routing the adapter's single-arg and
  two-arg local-call normalization paths through `parse_lir_typed_call(...)`
  plus those shared helpers instead of hand-copying parsed call metadata, and
  updating `tests/backend/backend_lir_adapter_tests.cpp` to assert that
  spacing-tolerant typed two-argument local calls now normalize to canonical
  backend-owned call formatting.
- Completed the next Step 4 residual direct-call recognizer cleanup slice by
  routing the remaining x86 zero-arg extern/declared direct-call recognizers
  and the x86 two-arg direct-call shape decoders through
  `parse_lir_direct_global_typed_call(...)` instead of composing separate
  zero-arg/callee/typed-operand helpers, deleting the now-unused local x86 and
  AArch64 pair-decoder helpers, and adding spacing-tolerant x86 extern direct
  call regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`.
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
- Completed the first Step 3 verifier/printer slice by adding
  `src/codegen/lir/verify.hpp` and `src/codegen/lir/verify.cpp`, routing
  `src/codegen/lir/lir_printer.cpp` through verifier-backed render helpers,
  and adding targeted printer/verifier coverage to
  `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the first Step 4 backend-consumer migration slice in
  `src/backend/lir_adapter.cpp` by replacing the adapter's raw arithmetic and
  integer-`ne` cmp string dispatch with `typed()`-backed helper checks, and
  added direct typed-op regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp` for both the structured add
  contract and the countdown-loop normalization path.
- Completed the next Step 4 target-specific backend-consumer slice in
  `src/backend/x86/codegen/emit.cpp` by routing the minimal conditional-return
  parser and fail-branch selection through `LirCmpPredicate::typed()` and
  enum-backed dispatch instead of raw predicate strings, with new typed x86
  regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the analogous Step 4 target-specific backend-consumer slice in
  `src/backend/aarch64/codegen/emit.cpp` by routing the minimal
  conditional-return parser and fail-branch selection through
  `LirCmpPredicate::typed()` and enum-backed dispatch instead of raw predicate
  strings, with new typed AArch64 regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the next Step 4 pointer-difference emitter slice in
  `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp` by routing the bounded global
  char/int pointer-difference recognizers through `LirBinOp::opcode.typed()`
  and `LirCmpOp::predicate.typed()` instead of relying on raw string
  compatibility, with new typed pointer-difference regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the next Step 4 call-family cleanup slice in
  `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp` by centralizing minimal direct-call
  callee/argument decoding helpers, routing the x86 LIR direct-call fast paths
  through `LirOperand::kind()` and enum-backed add-opcode checks, and updating
  the typed direct-call fixtures in
  `tests/backend/backend_lir_adapter_tests.cpp` to construct the covered call
  and add surfaces through typed wrappers instead of plain string literals.
- Completed the next Step 4 shared call-argument decoding slice by adding
  `src/codegen/lir/call_args.hpp`, routing
  `src/backend/stack_layout/analysis.cpp`,
  `src/backend/stack_layout/alloca_coalescing.cpp`, and
  `src/backend/liveness.cpp` through shared typed call-argument value
  extraction instead of open-coded `%` scraping, and updating
  `src/backend/x86/codegen/emit.cpp` plus
  `src/backend/aarch64/codegen/emit.cpp` to reuse the same top-level argument
  splitter for direct-call pair parsing.
- Completed the next Step 4 adapter normalization slice in
  `src/backend/lir_adapter.cpp` by replacing the adapter's exact
  `LirCallOp::{args_str,callee_type_suffix}` prefix/suffix matching with
  structured typed call parsing built on `src/codegen/lir/call_args.hpp`, and
  added regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`
  for local-call normalization that tolerates compatibility whitespace instead
  of falling back to the unsupported-text path.
- Completed the next Step 4 target-backend direct-call cleanup slice by adding
  structured typed-call decoding helpers to
  `src/codegen/lir/call_args.hpp`, routing the remaining x86 and AArch64
  single-arg, two-arg, param-slot, and call-crossing direct-call fast paths
  through structured operand/type parsing instead of exact
  `callee_type_suffix` and `args_str` string matches, and adding emitter
  regression coverage in `tests/backend/backend_lir_adapter_tests.cpp` for
  spacing-tolerant typed call suffixes and arguments.
- Completed the next Step 4 zero-arg direct-call cleanup slice by adding a
  shared `lir_call_has_no_args(...)` helper in
  `src/codegen/lir/call_args.hpp`, routing the remaining x86 and AArch64
  zero-argument direct-call recognizers through structured call-shape parsing
  instead of exact empty-string / `"()"` checks, and adding whitespace-tolerant
  typed zero-arg direct-call emitter coverage in
  `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the next Step 4 shared typed-call shape cleanup slice by expanding
  `src/codegen/lir/call_args.hpp` with shared single-arg and two-arg typed call
  operand helpers, routing `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp` through those shared helpers instead
  of local typed-call shape decoders, and collapsing
  `src/backend/lir_adapter.cpp` onto the same shared typed-call parser instead
  of maintaining a separate parameter/argument decoder.
- Completed the next Step 4 call-callee classification cleanup slice by adding
  shared call-callee parsing helpers to `src/codegen/lir/call_args.hpp`,
  classifying direct globals vs LLVM intrinsics vs indirect calls in one place,
  routing the remaining x86/AArch64 direct-call recognizers through the shared
  helper instead of open-coded `@...` checks, and adding regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp` for direct/global,
  intrinsic, and indirect call-callee classification.
- Completed the next Step 4 shared call-argument rewrite slice by adding a
  reusable `rewrite_lir_call_args(...)` helper in
  `src/codegen/lir/call_args.hpp`, routing
  `src/backend/stack_layout/slot_assignment.cpp` through it so manual entry
  slot coalescing also rewrites typed `LirCallOp::args_str` operands to the
  canonical retained alloca name, and adding regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp` for typed call-argument
  rewriting during slot-assignment application.
- Completed the next Step 4 stmt-emitter shared typed-call formatting slice by
  adding owned typed-call formatting helpers to
  `src/codegen/lir/call_args.hpp`, routing
  `src/codegen/lir/stmt_emitter.cpp` call-argument preparation through shared
  structured `(type, operand)` records instead of ad hoc string concatenation,
  and adding round-trip regression coverage in
  `tests/backend/backend_lir_adapter_tests.cpp` that proves formatted typed
  call metadata still parses through the shared structured call decoder.
- Completed the next Step 4 backend direct-callee classification slice by
  adding `lir_call_has_direct_global_callee(...)` to
  `src/codegen/lir/call_args.hpp`, routing the remaining x86/AArch64
  direct-call fast-path callee checks away from raw `@name` string equality in
  `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp`, and adding fallback-path regression
  coverage in `tests/backend/backend_lir_adapter_tests.cpp` proving llvm
  intrinsic and indirect callees no longer stay on the direct-call asm path.
- Completed the next Step 4 shared direct-global typed-call decoding slice by
  adding `parse_lir_direct_global_typed_call(...)` to
  `src/codegen/lir/call_args.hpp`, routing the remaining x86/AArch64
  direct-call add-imm and call-crossing backend fast paths plus the x86
  param-slot and immediate two-arg LIR fast paths through one shared
  direct-global typed-call view instead of stitching together separate callee
  classification and typed-argument decoders, and adding focused helper
  regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the next Step 4 adapter shared typed-call metadata slice in
  `src/backend/lir_adapter.cpp` by deleting the adapter's private typed-call
  copy/format layer, reusing `ParsedLirTypedCallView` and
  `format_lir_typed_call_args(...)` from `src/codegen/lir/call_args.hpp` for
  local-call normalization, and adding spacing-tolerant two-argument adapter
  regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`.
- Completed the next Step 4 backend shared direct-call view cleanup slice by
  routing the residual x86/AArch64 zero-arg and x86 call-crossing direct-call
  fast-path recognizers through
  `parse_lir_direct_global_typed_call(...)` instead of composing separate
  callee-only and typed-argument helpers, and added focused helper regression
  coverage in `tests/backend/backend_lir_adapter_tests.cpp` for zero-arg and
  single-operand call-crossing direct-call parsing.
- Completed the next Step 4 shared backend analysis/state-tracking call slice
  by adding `collect_lir_value_names_from_call(...)` and
  `rewrite_lir_call_operands(...)` to `src/codegen/lir/call_args.hpp`, routing
  `src/backend/liveness.cpp`,
  `src/backend/stack_layout/analysis.cpp`,
  `src/backend/stack_layout/alloca_coalescing.cpp`, and
  `src/backend/stack_layout/slot_assignment.cpp` through that shared full-call
  seam instead of open-coding `callee` plus `args_str` handling, and adding
  focused regression coverage in `tests/backend/backend_lir_adapter_tests.cpp`
  for indirect-callee call value collection and full-call operand rewriting.

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
- Step 4 now centralizes the current compatibility-path call-argument parsing
  in `src/codegen/lir/call_args.hpp`, including top-level comma splitting that
  tolerates nested aggregate/function syntax and a shared fallback path for
  nested expression operands such as `getelementptr`.
- `src/codegen/lir/call_args.hpp` now also exposes structured typed-call views
  for parameter lists and decoded argument `(type, operand)` pairs, so backend
  consumers can prove typed-call compatibility without reimplementing
  parsing, plus shared full-call collection/rewrite helpers for the residual
  backend analysis/state-tracking sites that still had to touch `callee` and
  `args_str` together.
- Regression status for this slice:
  `backend_lir_adapter_tests` passes, the current full `ctest --test-dir build
  -j8 --output-on-failure` run still ends in the repo's known x86 backend
  fallback cluster, and the checked failure logs remain monotonic
  (`test_fail_before.log`: 189 failing tests, `test_fail_after.log`: 188
  failing tests).
  ad hoc suffix/argument parsing or exact-text comparisons.
- `src/codegen/lir/call_args.hpp` now also exposes shared helper entry points
  for owning parsed typed-call arguments and canonicalizing parameter-list
  formatting, so adapter-side local-call normalization no longer needs a
  private parse/copy layer just to re-emit structured call metadata.
- Verification for the current slice:
  `./build/backend_lir_adapter_tests` passes after the adapter cleanup. Full
  `ctest --test-dir build -j8 --output-on-failure | tee test_after.log`
  finishes with the same standing 188 failing tests out of 2560 that are
  already captured in the repo's `test_fail_before.log` / `test_fail_after.log`
  baseline, with the failures concentrated in the existing x86 backend
  fallback set.
  for the common single-arg and two-arg typed direct-call shapes used by the
  minimal x86/AArch64 emitter fast paths, which removes the duplicated
  `parse_lir_typed_call(...)` shape assertions that had drifted into those
  target backends and the adapter.
- `src/codegen/lir/call_args.hpp` now also centralizes call-callee shape
  decoding, so backend consumers can distinguish direct globals, LLVM
  intrinsics, and indirect calls without open-coding `@` prefix rules in each
  backend file.
- `src/codegen/lir/call_args.hpp` now also exposes a shared
  `rewrite_lir_call_args(...)` helper that preserves typed call-argument shape
  while swapping canonical operand names, so compatibility-path consumers do
  not need to hand-roll `args_str` editing when they still operate on legacy
  `LirCallOp` text storage.
- `src/codegen/lir/call_args.hpp` now also exposes owned typed-call formatting
  helpers, and `src/codegen/lir/stmt_emitter.cpp` uses those shared `(type,
  operand)` records before serializing `LirCallOp::args_str`, which shrinks the
  remaining number of call construction sites that only ever see printer-shaped
  text during emission.
- The analysis/liveness stack still uses compatibility-text scanning for GEP
  indices and other non-call textual payloads, but `LirCallOp::args_str` no
  longer has three separate ad hoc scanners with subtly different behavior.
- Full-suite regression check for this slice remained monotonic against the
  checked-in `test_fail_before.log` baseline: `test_fail_after.log` improved
  from 189 failing tests to 188 with no newly failing tests, and
  `c_testsuite_x86_backend_src_00100_c` is the only failure that dropped out of
  the set.
- The latest full-suite regression check for the direct-callee classification
  slice also remained monotonic against the checked-in
  `test_fail_before.log` baseline: `test_fail_after.log` improved from 189
  failing tests to 188 with no newly failing tests, and
  `c_testsuite_x86_backend_src_00100_c` is still the only failure that dropped
  out of the set.
- The latest full-suite regression check for the shared direct-call view
  cleanup slice also remained monotonic against the checked-in
  `test_fail_before.log` baseline: `test_fail_after.log` improved from 189
  failing tests to 188 with no newly failing tests, and
  `c_testsuite_x86_backend_src_00100_c` is still the only failure that dropped
  out of the set.
- The latest full-suite regression check for the residual direct-call
  recognizer cleanup slice also remained monotonic against the checked-in
  `test_fail_before.log` baseline: `test_fail_after.log` improved from 189
  failing tests to 188 with no newly failing tests, and
  `c_testsuite_x86_backend_src_00100_c` is still the only failure that dropped
  out of the set.
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
- Step 3 now localizes typed-surface validation in one place instead of letting
  each printer branch hand-roll string assumptions: `print_llvm` verifies the
  module up front and the printer renders core typed wrappers through explicit
  helper calls rather than concatenating raw wrapper fields ad hoc.
- The verifier keeps the current legacy LLVM text path alive by tolerating the
  existing compatibility payloads that still appear in-tree, including `null`,
  `blockaddress(...)`, immediate-zero pointer spellings, and a few historically
  tolerated `void`-typed load/store/bin surfaces, while still rejecting
  malformed structural fields such as non-SSA result operands or unknown typed
  opcode/predicate wrappers.

## Next Intended Slice

- Follow the Step 4 typed-dispatch cleanup into the remaining backend
  call-adjacent consumers that still rely on raw `LirCallOp` storage
  compatibility instead of structured call metadata, with indirect/intrinsic
  call-family surfaces still the highest-value targets now that direct-call
  callee classification and stmt-emitter typed-call formatting are shared.
- The next Step 4 slice should target the backend-specific fast paths that
  still parse `callee_type_suffix` and `args_str` directly even after callee
  classification has been shared, so typed argument/result shape checks can be
  collapsed further onto common structured call views.
- Keep GEP index text, inline asm text, and declaration text on the
  compatibility path for now; the next slice should focus on shrinking the
  remaining call-adjacent protocol surface before attempting broader
  `LirCallOp` storage changes.

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
- `cmake --build build -j8`
- `ctest --test-dir build -R backend_lir_adapter_tests --output-on-failure`
  passed.
- `ctest --test-dir build -j8 --output-on-failure > test_after.log`
  recorded `2372/2560` passing and `188` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_after.log`
  reported PASS with one resolved failure
  (`c_testsuite_x86_backend_src_00100_c`) and no newly failing tests.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  wrapper layer grew enough string-compatibility operators to keep existing
  backend consumers compiling unchanged.
- `./build/backend_lir_adapter_tests` passed with added coverage for the new
  typed wrapper layer.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  Step 3 verifier/printer helpers landed.
- `./build/backend_lir_adapter_tests` passed with new coverage for verified
  typed rendering and malformed typed-LIR rejection.
- `cmake --build build -j8` passed after wiring `src/codegen/lir/verify.cpp`
  into both `c4cll` and `backend_lir_adapter_tests`.
- `ctest --test-dir build -j --output-on-failure > test_fail_after_step3.log`
  recorded `2371/2560` passing and `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after.log --after test_fail_after_step3.log
  --allow-non-decreasing-passed` passed with zero new failures and no pass-count
  regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  shared typed-call shape helpers replaced the duplicated emitter-side and
  adapter-side decoders.
- `./build/backend_lir_adapter_tests` passed with added shared helper coverage
  for whitespace-tolerant single-arg and two-arg typed call decoding.
- `cmake --build build -j8` passed after routing
  `src/backend/lir_adapter.cpp`,
  `src/backend/x86/codegen/emit.cpp`, and
  `src/backend/aarch64/codegen/emit.cpp` through the shared typed-call shape
  helpers in `src/codegen/lir/call_args.hpp`.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  adding shared call-callee classification helpers and routing the remaining
  direct-call recognizers through them.
- `./build/backend_lir_adapter_tests` passed with added helper coverage for
  direct-global, LLVM-intrinsic, and indirect call-callee classification.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded `2372/2560` passing and `188` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures, zero new
  suspicious slow tests, and one resolved failure
  (`c_testsuite_x86_backend_src_00100_c`).
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_shared_call_decode.log` recorded `2372/2560` passing
  and `188` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after_step4_zero_arg_call_shape.log
  --after test_fail_after_step4_shared_call_decode.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  first Step 4 typed-dispatch adapter slice landed.
- `./build/backend_lir_adapter_tests` passed with new direct typed-op coverage
  for adapter binary decoding and countdown-loop normalization.
- `cmake --build build -j8` passed after routing `src/backend/lir_adapter.cpp`
  through typed opcode/predicate helpers.
- `ctest --test-dir build -j --output-on-failure > test_fail_after_step4.log`
  recorded `2371/2560` passing and `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after_step3.log --after test_fail_after_step4.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  x86 conditional-return typed-predicate emitter slice landed.
- `./build/backend_lir_adapter_tests` passed with new x86 typed cmp-predicate
  coverage for compare-and-branch lowering.
- `cmake --build build -j8` passed after routing
  `src/backend/x86/codegen/emit.cpp` through typed conditional-return
  predicates.
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_x86_typed.log` recorded `2371/2560` passing and `189`
  failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after_step4.log --after test_fail_after_step4_x86_typed.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after the
  AArch64 conditional-return typed-predicate emitter slice landed.
- `./build/backend_lir_adapter_tests` passed with new AArch64 typed
  cmp-predicate coverage for compare-and-branch lowering.
- `cmake --build build -j8` passed after routing
  `src/backend/aarch64/codegen/emit.cpp` through typed conditional-return
  predicates.
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_aarch64_typed.log` recorded `2371/2560` passing and
  `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after_step4_x86_typed.log --after
  test_fail_after_step4_aarch64_typed.log --allow-non-decreasing-passed`
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  routing the remaining x86/AArch64 direct-call fast paths through structured
  typed-call decoding helpers.
- `./build/backend_lir_adapter_tests` passed with added spacing-tolerant
  coverage for typed direct-call suffix/argument decoding in both target
  backends.
- `cmake --build build -j8` passed after extending
  `src/codegen/lir/call_args.hpp` with structured typed-call views and wiring
  the target backends onto them.
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_call_spacing.log` recorded `2371/2560` passing and
  `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after.log --after test_fail_after_step4_call_spacing.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
  passed with zero new failures and no pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the typed pointer-difference emitter cleanup in both targets.
- `./build/backend_lir_adapter_tests` passed with added typed char/int
  pointer-difference coverage for both x86 and AArch64 backend emission.
- `cmake --build build -j8` passed after routing the x86 and AArch64
  pointer-difference recognizers through typed opcode/predicate dispatch.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded `2371/2560` passing and `189` failing tests.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the adapter-side structured call parsing cleanup.
- `./build/backend_lir_adapter_tests` passed with added whitespace-tolerant
  local-call normalization coverage.
- `cmake --build build -j8` passed after routing `src/backend/lir_adapter.cpp`
  through structured typed call parsing instead of exact string slicing.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded `2371/2560` passing and `189` failing tests after the adapter
  cleanup slice.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures, zero pass-count
  regression, and zero new suspicious slow tests.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the shared `LirCallOp` argument-decoding helper and wiring the
  stack/liveness consumers plus x86/AArch64 pair parsers through it.
- `./build/backend_lir_adapter_tests` passed with new coverage for nested
  typed call-argument splitting/value collection and shared stack-layout
  call-argument use tracking.
- `cmake --build build -j8` passed after wiring the shared call-argument
  helper into `c4cll`.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded `2371/2560` passing and `189` failing tests after the shared
  call-argument cleanup slice.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression after the shared call-argument cleanup slice.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the call-family helper cleanup in the x86 and AArch64 emitters.
- `./build/backend_lir_adapter_tests` passed with the typed direct-call fixtures
  now constructed through explicit global-operand and enum-backed add wrappers.
- `cmake --build build -j8` passed after centralizing the minimal direct-call
  callee/argument decoding helpers and tightening the x86 LIR call fast paths
  onto typed callee/opcode access.
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_call_typed.log` recorded `2371/2560` passing and
  `189` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after.log --after test_fail_after_step4_call_typed.log
  --allow-non-decreasing-passed` passed with zero new failures and no
  pass-count regression.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the shared zero-arg typed-call helper and routing the remaining x86
  and AArch64 zero-arg direct-call recognizers through it.
- `./build/backend_lir_adapter_tests` passed with added whitespace-tolerant
  zero-arg typed direct-call coverage for both x86 and AArch64 backend
  emission.
- `cmake --build build -j8` passed after wiring the shared zero-arg call-shape
  helper into both backend emitters and the main compiler target.
- `ctest --test-dir build -j --output-on-failure >
  test_fail_after_step4_zero_arg_call_shape.log` recorded `2372/2560` passing
  and `188` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_after.log --after test_fail_after_step4_zero_arg_call_shape.log
  --allow-non-decreasing-passed` passed with zero new failures, one resolved
  failure (`c_testsuite_x86_backend_src_00100_c`), and no suspicious slow
  tests.
- `cmake --build build -j8 --target backend_lir_adapter_tests` passed after
  landing the adapter-side shared typed-call metadata cleanup.
- `ctest --test-dir build -R backend_lir_adapter_tests --output-on-failure`
  passed with added spacing-tolerant two-argument local-call normalization
  coverage.
- `cmake --build build -j8` passed after deleting the adapter-local typed-call
  copy/format shim in favor of shared `call_args.hpp` metadata and formatting.
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
  recorded `2372/2560` passing and `188` failing tests.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
  --before test_fail_before.log --after test_fail_after.log
  --allow-non-decreasing-passed` passed with zero new failures, one resolved
  failure (`c_testsuite_x86_backend_src_00100_c`), and no suspicious slow
  tests after the adapter shared typed-call metadata cleanup slice.

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

- None. The next task is the next Step 4 backend-consumer migration slice on
  top of the new verifier-backed typed wrapper layer.
