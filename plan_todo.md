# LIR De-Stringify Legacy-Safe Refactor Todo

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Source Plan: plan.md

## Active Item

- Step 4: Migrate high-friction instruction families and consumers.
- Exact target for the next iteration: continue Step 4 past
  the landed shared typed-call shape helpers into the remaining call-adjacent
  protocol surface that still depends on raw `LirCallOp::{args_str,
  callee_type_suffix}` storage, with the highest-value next slice being any
  backend or emitter path that still needs structured indirect/intrinsic call
  metadata rather than just shared direct-call argument decoding.

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
  ad hoc suffix/argument parsing or exact-text comparisons.
- `src/codegen/lir/call_args.hpp` now also exposes shared helper entry points
  for the common single-arg and two-arg typed direct-call shapes used by the
  minimal x86/AArch64 emitter fast paths, which removes the duplicated
  `parse_lir_typed_call(...)` shape assertions that had drifted into those
  target backends and the adapter.
- The analysis/liveness stack still uses compatibility-text scanning for GEP
  indices and other non-call textual payloads, but `LirCallOp::args_str` no
  longer has three separate ad hoc scanners with subtly different behavior.
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
  call-family surfaces still the highest-value targets before attempting
  broader `LirCallOp` storage changes.
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
