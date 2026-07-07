# RV64 pr56982 Post-Carrier Runtime Mismatch

Status: Closed
Type: Capability investigation
Parent: `ideas/closed/571_rv64_inline_asm_carrier_lowering.md`
Owning Layer: RV64 object route runtime behavior after inline asm carrier lowering

## Goal

Diagnose and repair the RV64 object-route behavior that causes
`src/pr56982.c` to reach `RV64_BACKEND_RUNTIME_MISMATCH` with a c4c
segmentation fault after inline asm carrier lowering no longer blocks
compilation.

## Why This Exists

The 571 inline asm carrier evidence shows that `src/pr56982.c` no longer fails
first on the old generic `unsupported_instruction_fragment` carrier fallback
and no longer reports the narrower `unsupported_inline_asm_fragment`
diagnostic. Its RV64 object route now compiles far enough to run, then reports
a runtime mismatch with c4c segfaulting.

Evidence:

- `build/agent_state/571_rv64_inline_asm_carrier_lowering/classification.tsv`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/summary.md`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/src_pr56982.c/object-route.log`
- `build/agent_state/571_rv64_inline_asm_carrier_lowering/src_pr56982.c/dump-prepared-bir.txt`

## In Scope

- Reproduce the post-carrier `src/pr56982.c` runtime mismatch/segfault on the
  RV64 object route.
- Determine the first semantic owner after inline asm carrier lowering that
  explains the runtime mismatch or generated binary crash.
- Add focused backend or route-level proof for that owner before changing
  lowering behavior.
- Repair the identified RV64 lowering/runtime behavior without changing
  inline asm carrier diagnostics or unsupported classification.
- Preserve evidence that the failure is no longer an inline asm carrier
  compile diagnostic.

## Out Of Scope

- Reworking generic inline asm carrier lowering from idea 571.
- Broad same-module call ABI, pointer arithmetic, select, floating-point
  binary, or phi-select lowering unless the evidence proves one of those is
  the first semantic owner for `pr56982.c`.
- Runtime comparison rewrites, expected-output changes, unsupported-marker
  edits, or allowlist changes.
- Filename-specific handling for `src/pr56982.c`.
- Claiming progress only because the failure moves from one runtime symptom to
  another without identifying and repairing the semantic owner.

## Acceptance Criteria

- The first post-carrier owner for the `src/pr56982.c` runtime mismatch or
  c4c segfault is recorded with BIR/prepared-BIR/MIR or object-route evidence.
- A focused test or repeatable route-level proof demonstrates the repaired
  owner.
- `src/pr56982.c` no longer reports the same post-carrier runtime
  mismatch/segfault for the same reason, or the remaining failure is documented
  as a later distinct owner in a separate follow-up idea.
- No inline asm carrier fallback, unsupported marker, allowlist, or runtime
  comparison file is changed as the main proof.

## Closure Note

Closed after identifying the first post-carrier owner as RV64 object emission
reusing the prepared persistent `s1` home for the global symbol address `@env`
as a later call argument. The repair rematerializes `SymbolAddress` call
arguments before the generic `PriorPreservation` fallback. The representative
`src/pr56982.c` RV64 object route now exits `0`; the old
`RV64_BACKEND_RUNTIME_MISMATCH` with c4c segfault from stale symbol-address
reuse is gone and no later owner remained for this representative route.

Close gate used matching backend logs:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Regression guard passed with 346 passed before and 346 passed after.

## Reviewer Reject Signals

- Reject a slice that fixes only `src/pr56982.c` by filename, exact source
  shape, or testcase-specific output handling.
- Reject expectation rewrites, unsupported-marker additions, allowlist edits,
  or runtime comparison changes claimed as the repair.
- Reject treating an inline asm diagnostic rename or carrier-only refactor as
  progress on the runtime mismatch.
- Reject broad RV64 lowering rewrites that do not identify the first
  post-carrier semantic owner for the segfault/mismatch.
- Reject a route that leaves the same `RV64_BACKEND_RUNTIME_MISMATCH` with c4c
  segfaulting for the same reason while claiming the idea complete.
