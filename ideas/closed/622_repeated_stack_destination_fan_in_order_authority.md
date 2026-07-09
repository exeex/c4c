# Repeated Stack-Destination Fan-In Order Authority

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/607_destination_fan_in_authority_research.md`
- `ideas/closed/610_rv64_move_bundle_target_materialization.md`
- `docs/destination_fan_in_authority/`
Owning Layer: prepared/prealloc authority, with RV64 consumption only after
authority is explicit
Queue Order: 22
Prerequisites: destination fan-in research from idea 607 and existing
fail-closed RV64 move-bundle diagnostics
Proof Surface: repeated stack-destination move-bundle rows such as
`src/pr71631.c`

## Goal

Define and implement explicit authority for repeated stack-destination
move-bundle rows so RV64 can consume only proven legal destination fan-in or
ordering, while ambiguous rows remain fail-closed.

## Why This Exists

Idea 610 closed the RV64 consumer gap for authorized move-bundle target shapes.
Its close-readiness audit left `src/pr71631.c` outside the close gate because
the row family has repeated stack destinations and needs destination fan-in,
ordering, or mutual-exclusion authority. RV64 must not infer that policy from
an encodable move sequence.

## In Scope

- Classify the current repeated stack-destination residuals, including
  `src/pr71631.c`.
- Publish prepared/prealloc destination authority facts for legal repeated
  stack-destination bundles, or record precise missing-authority reasons when
  the producer cannot prove legality.
- Teach the RV64 prepared move-bundle consumer to accept only explicit,
  supported destination fan-in/order authority.
- Preserve precise fail-closed diagnostics for missing, unknown, ambiguous, or
  malformed authority.
- Add focused positive and negative backend coverage for the supported and
  rejected authority shapes.

## Out Of Scope

- Reopening idea 610's direct register-to-register move-bundle materialization
  route.
- Guessing destination order, exclusivity, or last-writer behavior in RV64.
- Select-publication source wiring, ABI/call-boundary moves, generic
  before-instruction fragments, terminator lowering, global data, runtime
  mismatch, expectations, unsupported markers, allowlists, timeout policy, or
  accounting.

## Acceptance Criteria

- Prepared diagnostics or object evidence expose a concrete destination
  fan-in/order authority kind, owner, destination stack slot, source homes, and
  rejection reason when authority is absent.
- At least one legal repeated stack-destination bundle advances through RV64
  lowering using explicit authority.
- `src/pr71631.c` is rerun and either advances past the repeated
  stack-destination blocker or reports the exact upstream authority still
  missing.
- Ambiguous repeated stack-destination bundles remain fail-closed with a
  specific diagnostic.
- Backend regression proof passes with matching before/after logs for the
  touched backend scope.

## Closure Note

Closed after Step 5 review. The implementation publishes
`StackDestinationRegisterFanIn` prepared/prealloc authority with owner,
destination stack slot, source homes, and candidate order evidence; RV64 lowers
only matching explicit authority facts and preserves fail-closed diagnostics
for missing, unsupported, mismatched, or malformed authority. Focused backend
coverage proves the legal select-shaped fan-in path and rejected authority
shapes. `src/pr71631.c` was not special-cased; remaining repeated
stack-destination families without producer facts stay blocked by explicit
missing-authority diagnostics or belong to separate non-select authority work.

Close-time backend guard used matching `-R '^backend_'` CTest logs. Strict
mode reported no new failures but rejected equal pass count; the explicit
non-decreasing regression mode passed with 347 passed, 0 failed before and
after.

## Reviewer Reject Signals

- Reject filename-, function-, block-, value-id-, offset-, or row-count-specific
  handling for `src/pr71631.c` or any single testcase.
- Reject RV64-side selection of destination order, mutual exclusion, or
  last-writer semantics without explicit prepared/prealloc authority facts.
- Reject accepting repeated stack destinations by weakening or bypassing the
  ambiguous move-bundle classifier.
- Reject diagnostic-only, helper-rename, expectation, unsupported-marker,
  allowlist, timeout, runtime, or accounting changes claimed as capability
  progress.
- Reject broad select, ABI, terminator, generic instruction, global-data, or
  runtime rewrites that do not prove repeated stack-destination authority.
- Reject retaining the exact old `unsupported_move_bundle_target_shape` or
  ambiguous destination failure behind a renamed helper or broader catch-all.
