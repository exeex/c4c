# Backend Baseline History Umbrella Triage

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md`
Handoff Directory: `docs/backend_baseline_history_triage/`
Related:
- `test_baseline.log`
- `test_before.log`
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
- `build/agent_state/657_step4_representative_proof/summary.md`
- `build/agent_state/agent_logs/agents_codex_iter_21_844333.log`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md`

## Goal

Use the current backend baseline logs and reverse-chronological evidence
history to classify the noisy red backend/LLVM failure surface and generate
ordered follow-up ideas that repair the current baseline problem without
overfitting named tests.

## Why This Exists

The latest active 657 runbook was deactivated because `test_baseline.log`
contains too many failures to use as clean acceptance evidence. Directly fixing
one named runtime case would be premature: the log surface spans RV64 dump,
codegen, object runtime, prepared BIR/CLI, AArch64 backend internals, and two
LLVM torture rows. Recent evidence is also time-sensitive: the 04:12 Step 4
summary for 657 still reports a representative `loop-2e.c` runtime mismatch,
while the later 04:18 Step 3 representative summary reports the same
representative object-runtime comparison passing. This umbrella must prevent
route drift by treating newer file timestamps as authoritative until proven
otherwise.

## Current Evidence

- `test_baseline.log` from 2026-07-10 04:20:43 is the newest broad baseline
  evidence. It reports `91% tests passed, 32 tests failed out of 368` for
  backend coverage and also lists two `llvm_gcc_c_torture` failures.
- `test_before.log` from 2026-07-10 04:18:29 is the matching earlier backend
  proof baseline. Its failed backend set matches the 32 backend failures in
  `test_baseline.log`; `test_baseline.log` additionally records the two LLVM
  torture failures.
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
  from 2026-07-10 04:18:53 is newer than
  `build/agent_state/657_step4_representative_proof/summary.md` from
  2026-07-10 04:12:24 and therefore supersedes it for the representative
  `loop-2e.c` status unless fresh evidence says otherwise.
- The newest 657 evidence says focused dump/object-runtime contracts passed
  and representative `loop-2e.c` RV64 object-runtime comparison passed against
  clang, but broad backend proof remained red with the known noisy subset.
- The failed baseline rows cluster around RV64 prepared destination/parameter
  publication, pointer-local route/runtime cases, byval/prepared call
  boundary, prepared object data/static storage, callee-saved GPR runtime,
  packed local member offsets, internal prepared BIR/CLI tests, one AArch64
  instruction dispatch test, and two LLVM torture rows.

## In Scope

- Create or refresh handoff docs under
  `docs/backend_baseline_history_triage/`.
- Walk log and evidence history in reverse file timestamp order before trusting
  older summaries.
- Classify each current failure family by first owning layer and evidence age.
- Distinguish stale or superseded representative summaries from current
  baseline failures.
- Generate ordered follow-up ideas under `ideas/open/` with each idea naming
  one owning layer.
- Record dependency rules between follow-up ideas, especially where one
  producer/publication repair should precede multiple consumer/runtime fixes.

## Out Of Scope

- Implementing backend, frontend, test, expectation, allowlist, timeout, or
  runtime-policy changes inside this umbrella idea.
- Accepting, rewriting, or weakening `test_baseline.log` as proof of progress.
- Mixing prepared producer repair, RV64 consumer lowering, AArch64 internals,
  CLI dump formatting, and LLVM torture diagnosis in one implementation idea.
- Reopening 657 as the first repair target unless reverse-chronological
  evidence proves it is still the first baseline owner.

## Priority Model

Follow-up ordering must be driven by evidence freshness first, then first
owning layer, then breadth across the failed baseline rows. Start from the
newest files and walk backward:

1. `test_baseline.log` and `test_before.log`
2. newest `build/agent_state/*/summary.md` and related logs
3. latest agent log handoff
4. older source ideas or summaries only when newer evidence does not already
   settle the owner

Prefer one broad producer/publication owner that explains multiple failures
over a named testcase repair. Prefer fail-closed classification over changing
expectations when the owning layer is still ambiguous.

## Required Follow-Up Ideas

Generate these families unless fresh evidence proves a better split:

- RV64 prepared destination / parameter-home publication failures, including
  stack-passed parameter home and scalar frame-slot destination rows.
- RV64 pointer-local and pointer-step route/runtime failures, including loop,
  Duff fallthrough, and i16 local-array select/store rows.
- RV64 byval and prepared call-boundary aggregate failures.
- Prepared object data / static storage runtime failures.
- Callee-saved GPR and packed local member runtime failures.
- Internal prepared BIR/CLI/AArch64 failures that are not owned by RV64
  runtime lowering.
- LLVM torture rows `20040709-2.c` and `20040709-3.c`, only after deciding
  whether they share a backend owner or need a separate frontend/runtime idea.

## Acceptance Criteria

- The handoff directory contains current evidence, reverse-chronological
  history, ownership classification, and follow-up ordering documents.
- The documents agree that `test_baseline.log` is the broad current evidence
  and explicitly state which newer evidence supersedes older summaries.
- Every failed row from the current baseline is assigned to an owning family or
  marked unassigned with a concrete next probe.
- Follow-up ideas are generated under `ideas/open/`, ordered by dependency and
  breadth, and each names exactly one owning layer.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, or default harness
  contracts.

## Closure Note Requirements

The closure note must state which logs and evidence timestamps were used,
which handoff docs were written, which follow-up ideas were generated, why
they are ordered that way, and which baseline rows remain unassigned or
intentionally deferred.

## Reviewer Reject Signals

- Reject direct implementation or test expectation changes inside this
  umbrella idea.
- Reject output that only lists counts without ownership classification,
  timestamp ordering, and follow-up ideas.
- Reject treating the older 657 Step 4 representative mismatch as
  authoritative without reconciling the newer 657 Step 3 representative pass.
- Reject stale evidence left as authoritative after newer log or summary files
  are chosen.
- Reject follow-up ideas that mix first owning layers such as RV64 pointer
  lowering, prepared BIR/CLI, AArch64 internals, and LLVM torture diagnosis.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, or weaker runtime checks as progress.
