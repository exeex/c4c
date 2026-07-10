# Byval Prepared Dump Contract Review

Status: Closed
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/closed/659_rv64_byval_prepared_call_boundary.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: prepared byval dump contract and snippet exposure
Queue Order: 69
Proof Surface: residual rows from the closed RV64 byval prepared
call-boundary route:
- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_dump_riscv64_byval_preserved_pointer_args`

## Goal

Review and repair the prepared-BIR dump contract for byval aggregate and
preserved pointer-argument call-boundary rows where runtime and route behavior
now pass but dump snippets remain stale or mismatched.

## Why This Exists

The RV64 byval prepared call-boundary route repaired the selected runtime and
route owner and passed the broad backend regression guard, but focused proof
left two dump rows red. The recorded evidence says the byval aggregate dump
expects value id 20 while current prepared output publishes the call-argument
stack move for value id 22, and the preserved-pointer dump expects an older
aggregate-address shape while current output records frame-slot call-argument
sources. That is dump-contract or expectation-review work, not permission to
reopen the completed runtime repair.

## In Scope

- Refresh focused prepared-BIR dump evidence for the two residual byval rows.
- Identify whether the first owner is stale snippet expectations, dump text
  emission over valid prepared facts, or missing dump publication.
- Repair the general dump contract or prepared fact exposure only when the
  focused evidence proves the owner.
- Keep route and runtime byval behavior as regression surfaces, not as the
  implementation target.

## Out Of Scope

- RV64 byval runtime or codegen-route repairs already completed by idea 659.
- Object-runtime `BinaryInst` unsupported-fragment work.
- Pointer-local, destination-publication, static object-data, callee-saved
  GPR, packed-member, AArch64, or LLVM torture work.
- Unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names whether each residual dump row is stale expectation,
  dump emission, or missing prepared publication.
- Any expectation update is justified by current prepared facts and paired with
  route/runtime proof, not claimed as compiler capability progress by itself.
- Any code repair exposes existing prepared facts faithfully without matching
  testcase names, value ids, or final assembly shape.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject treating text-only expectation rewrites as runtime or lowering
  capability progress.
- Reject named-case matching for either byval dump row, value-id-only fixes, or
  final-assembly-shape shortcuts.
- Reject unsupported-marker downgrades, allowlist edits, timeout changes,
  runtime policy changes, helper renames, or baseline accounting changes
  claimed as progress.
- Reject reopening the closed RV64 byval runtime/codegen-route owner without
  fresh proof that the passing route or runtime rows regressed.
- Reject leaving the same stale dump-contract mismatch behind a renamed helper
  or diagnostic.

## Closure Note

Closed after focused evidence classified both residual dump failures as stale
snippet expectations over current prepared facts, and the expectation contract
was aligned without claiming lowering or runtime progress. The focused dump
rows plus nearby byval route/runtime regression surfaces passed before and
after the patch: `6 passed / 0 failed / 6 total` in both `test_before.log` and
`test_after.log`. Strict regression guard failed only because the pass count was
unchanged; `--allow-non-decreasing-passed` passed with no new failures. No new
open idea is required for this source scope.
