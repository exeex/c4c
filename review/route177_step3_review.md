# Route 177 Step 3 Review

Active source idea path: `ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md`

Chosen base commit: `c4318503f` (`[plan] Activate AArch64 f64 readback dispatch debt plan`). This is the activation checkpoint for idea 177's active runbook; later lifecycle commits `89593caaa` and `1ca7a5bb7` are Step 1/Step 2 todo-only execution state, not a source-idea or route-reset checkpoint.

Commit count since base: 2 committed changes, plus the current uncommitted Step 3 diff.

## Findings

1. Medium: Step 3 exceeded the narrow Step 2 handoff, but the expansion is aligned with the source idea rather than route drift. `todo.md` Step 2 selected only the local `PreparedValueHomeLookups` fallback, while the implementation also adds `emit_fp_select_chain_value_to_register(...)` and routes selected F32/F64 direct-global call arguments through it. That is broader than the immediate Step 2 wording, but the active idea and plan explicitly allow a source-selection or ABI handoff repair for selected f64 global readback. The added helper is gated through `materialize_direct_global_select_chain_call_argument(...)`, requires direct-global select-chain dependency evidence, targets prepared FPR homes, and feeds the existing ABI move instead of moving call ABI placement policy. See `src/backend/mir/aarch64/codegen/select_materialization.cpp:263` and `src/backend/mir/aarch64/codegen/select_materialization.cpp:561`.

2. Low: The new FP select-chain helper duplicates some existing FP select materialization shape and should remain a watch item, but it is not acceptance-blocking. The helper mirrors the existing `fcsel` lowering shape in `fp_value_materialization.cpp`, with the added prepared-home fallback needed for the direct-global call-argument path. It does not match on the dispatch test name, global name, or emitted assembly text. The raw same-block BIR select scan at `src/backend/mir/aarch64/codegen/select_materialization.cpp:295` is more local than ideal, but it is value/type based and still guarded by the direct-global call-argument route at the call site, so it reads as a local semantic fallback rather than a named-test shortcut.

3. Low: Proof is narrow but sufficient for this packet under the active idea. The source idea names the monolithic `backend_aarch64_instruction_dispatch` CTest as the close-level acceptance signal, and Step 3 reports that exact build plus CTest passing in `test_after.log`. No public call, source-selection, or AArch64 dispatch interfaces changed, and the diff touches no test expectations. Broader backend validation is not required before accepting this slice, though it may be reasonable later if more select-materialization packets accumulate.

## Checks

- Expectation weakening: none found. Current tracked diff touches only `src/backend/mir/aarch64/codegen/select_materialization.cpp` and `todo.md`.
- Named-test shortcuts: none found.
- Brittle emitted-text matching in implementation: none found. The implementation emits assembly; it does not inspect printed assembly snippets.
- ABI/layout policy leakage: none found. The implementation consumes prepared value homes and preserves the existing `fmov d0, d20` call ABI move rather than relocating ABI placement into Route 5 or generic BIR records.
- Insufficient proof: no blocker. `test_after.log` shows `cmake --build build --target backend_aarch64_instruction_dispatch_test` and `ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure` passed.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `on track`

Technical-debt judgment: `watch`

Validation sufficiency: `narrow proof sufficient`

Reviewer recommendation: `continue current route`

Acceptance should not require reducing this to the Step 2 value-home fallback. The fallback alone was the selected first repair attempt, but the current diff demonstrates that selected FP direct-global call arguments also need local select-chain materialization into the prepared FPR home. Acceptance also should not require splitting/refactoring before commit; record the helper duplication/local fallback shape as watch debt and continue to Step 4 proof/handoff.
