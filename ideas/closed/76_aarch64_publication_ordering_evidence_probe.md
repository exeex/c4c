# AArch64 Publication Ordering Evidence Probe

## Goal

Trace representative publication paths from prepared facts to AArch64 records
and decide whether publication materialization ordering is already shared
authority or needs a new prepared publication-order query.

## Why This Exists

The large-owner residue audit classified publication materialization ordering
across call edges, memory sources, and dispatch edges as `needs-more-evidence`.
The required probe is to trace one edge-copy publication, one call-boundary
publication, and one typed stack-source publication from prepared facts to
AArch64 records, then determine whether ordering is locally re-derived.

## Owned Files

- Audit/probe notes in `todo.md` or a later activated plan.
- Candidate implementation files only after the probe decides the route:
  - `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - prepared publication lookup surfaces if a new query is required.

## In Scope

- Trace one edge-copy publication from prepared source facts to AArch64
  publication records.
- Trace one call-boundary publication from prepared call/publication facts to
  AArch64 records.
- Trace one typed stack-source publication from prepared stack-source facts to
  AArch64 records.
- Decide whether ordering can be handled by existing prepared publication facts
  or whether a new publication-order query is required.

## Out Of Scope

- Implementing publication cleanup before the ordering authority is known.
- Reopening the entire dispatch-family contraction.
- Combining this probe with broad call, memory, or dispatch cleanup.
- Changing publication expectations or supported behavior as part of the
  evidence pass.

## Proof Expectations

- A written trace for the three required publication paths.
- If implementation follows, focused tests for edge-copy, call-boundary, and
  typed stack-source publication ordering.
- Regression guard logs before accepting any code slice.

## Reviewer Reject Signals

- The route skips the trace and immediately adds local ordering helpers.
- A patch hard-codes one publication path or named testcase.
- Existing prepared publication facts are bypassed or duplicated without
  explaining the missing query.
- Dispatch, call, and memory rewrites are bundled before the probe determines
  authority.
- Tests are weakened or unsupported expectations are downgraded without
  explicit approval.

## Closure Note

Closed after evidence-only Steps 1 through 4 and reviewer acceptance. The
three required traces found that edge-copy publication ordering is carried by
existing prepared edge-publication/source-fact surfaces, call-boundary ordering
is carried by prepared call-boundary effect plans, and the representative typed
stack-source publication is selected through existing edge-publication identity
and stack-source preparation. No new prepared publication-order query is
required for the reviewed paths.
