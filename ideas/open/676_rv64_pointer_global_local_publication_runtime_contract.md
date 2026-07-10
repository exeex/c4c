# RV64 Pointer Global Local Publication Runtime Contract

Status: Open
Type: Focused contract/runtime proof
Parent: `ideas/open/675_post_wave_residual_baseline_failures.md`
Related:
- `build/agent_state/675_step1_candidate_delta/summary.md`
- `test_baseline.log`
- `test_baseline.new.log`
- `ideas/open/675_post_wave_residual_baseline_failures.md`
- `ideas/closed/664_riscv_object_emission_internal_probe.md`
Owning Layer: RV64 object runtime semantics and test-contract routing for a
now-succeeding expected-failure CLI row
Queue Order: 76
Proof Surface:
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`

## Goal

Decide whether the now-succeeding RV64 pointer/global-local publication object
is semantically valid, and route the baseline/test contract accordingly without
claiming implementation progress from expectation churn.

## Why This Exists

Step 1 of idea 675 showed that
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
no longer fails at the old RV64 object-route admission owner. The expected-fail
wrapper reports an unexpected success, and `llvm-objdump -d -r` shows direct
global address publication through stack slots followed by a live reload.

Before any expectation or baseline change, the emitted object needs a
runtime/semantic proof. If the object returns the expected global short value,
the owner is stale expected-failure/test-contract or baseline routing. If it
does not, the first implementation owner must be identified from the semantic
failure, not from the old local-memory admission diagnosis.

## In Scope

- Run or construct a focused RV64 runtime/semantic object proof for the
  generated object from
  `riscv64_pointer_global_local_publication_live_load_rejection.c`.
- Compare the generated object behavior with a known-good clang object or the
  repo's RV64 execution harness when available.
- If the emitted object is semantically valid, document the test-contract route
  needed to retire the stale expected-failure wrapper.
- If the emitted object is semantically invalid, record the exact first
  implementation owner with disassembly, relocation, and runtime evidence.
- Preserve the 675 Step 1 evidence and keep baseline comparison by stable test
  name.

## Out Of Scope

- Accepting `test_baseline.new.log` before both new-only rows are settled.
- Editing expectations, unsupported markers, allowlists, timeouts, runtime
  policy, or baseline accounting as part of this proof packet.
- Reopening idea 664 or older pointer/global-local owner notes unless the
  fresh runtime proof contradicts their closure evidence.
- Repairing
  `backend_cli_riscv64_call_arg_local_frame_address_materialization`; that is
  owned by the separate object-route materialization follow-up.

## Acceptance Criteria

- A focused runtime/semantic proof establishes whether the generated RV64
  object is valid for the global-local publication live-load case.
- Valid-object outcome: the stale expected-failure/test-contract owner is
  documented precisely enough for a later contract/baseline route, without
  changing expectations in this idea unless explicitly delegated.
- Invalid-object outcome: the first implementation owner is recorded with
  object bytes, relocations, and runtime behavior that explain the semantic
  mismatch.
- The candidate baseline remains unaccepted while this and the paired
  new-only row are unresolved.

## Reviewer Reject Signals

- Reject treating the old local-memory admission diagnosis as current owner
  without reproducing it in fresh evidence.
- Reject expectation rewrites, unsupported-marker changes, allowlist filtering,
  timeout changes, runtime-policy edits, or baseline accounting changes as
  proof that the object semantics are valid.
- Reject named-case-only code changes that make only this test row report
  success or failure without proving the emitted RV64 object semantics.
- Reject claiming compiler progress when the only result is classifying the
  expected-failure wrapper as stale.
- Reject broad RV64 object emission rewrites outside the pointer/global-local
  publication semantic proof needed here.
