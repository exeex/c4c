# RV64 Pointer Global Local Publication Runtime Contract

Status: Closed
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

## Closure Notes

Closed after the runtime proof and positive contract update completed the
source idea's proof route.

- Step 1 evidence in
  `build/agent_state/676_step1_runtime_contract/summary.md` established a
  repo-native RV64 link/run path for
  `build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`.
  The object links with the system RISC-V toolchain, runs under
  `qemu-riscv64`, and returns `0`, matching the expected zero-initialized
  global short value. The same harness observed the nearby known-good global
  publication object returning `7`, so the proof path is semantically useful
  rather than a no-op.
- Step 2 evidence in
  `build/agent_state/676_step2_contract_update/summary.md` documents the stale
  expected-failure wrapper being converted into the positive RV64 object-route
  contract for the live-load case. The updated contract requires successful
  object emission, RV64 ELF machine bytes, and the stable live pointer reload
  plus short load/store/reload byte sequence.
- Focused proof passed for the updated live-load row and the nearby existing
  publication object row.
- Close-time backend proof keeps
  `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  green. The only remaining backend failures in the matching close-gate scope
  are the known residual rows
  `backend_cli_riscv64_call_arg_local_frame_address_materialization` and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

`test_baseline.new.log` remains unaccepted. In particular, row 159
(`backend_cli_riscv64_call_arg_local_frame_address_materialization`) is still
unresolved and is carried by the dependency-order follow-up in
`ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md`.

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
