# Semantic Frame-Slot Materialization Probe Decomposition

Status: Closed
Type: Decomposition and focused-probe planning idea
Superseded Runbook: `ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md`
Closed By: lifecycle review after Step 4

## Completion Notes

Idea 482 is complete as a decomposition and focused-probe planning slice.

The route was reset because ideas 475 through 481 repeatedly moved the first
missing producer lower inside the same `%t23` failure family without shrinking
the owned capability family. This idea established the blocked-family baseline,
split non-duplicative focused probe candidates, bound each accepted candidate
to one backend seam, and executed the first narrow seam.

Step 4 added the focused scalar compare result frame-slot destination probe:

- Fixture: `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c`
- CTest: `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- Artifact: `build/agent_state/482_step4_scalar_compare_destination_probe/decision.md`

The probe proves that semantic instruction-result identity plus frame-slot
destination/layout facts are isolatable without copying the `930930-1`
monolith. It intentionally keeps materialization authority, interval/source
facts, branch-stack-load authority, and RV64 consumption fail-closed.

## Follow-Up State

`ideas/open/481_semantic_result_frame_slot_materialization_point_producer.md`
is resumed as the active source idea with a focused first packet based on the
new scalar-compare destination probe.

The optional probe families from decomposition remain classified but are not
required to keep 482 active:

- storage-only move rejection for `authority=none`;
- select-result stack-destination boundary;
- explicit synthetic materialization-point positive probe, if representable.

Those should be selected only if the resumed 481 route needs them as separate
focused evidence. They must not be folded into a monolithic testcase-shaped
route.

## Closed Scope

Closed as complete for:

- blocked failure-family baseline;
- focused non-duplicative probe selection;
- backend seam binding;
- first focused probe execution and route handoff.

Not closed as:

- materialization-authority producer completion;
- downstream source-fact, interval, branch-stack-load, or RV64 consumer
  completion;
- proof that raw identity plus destination/layout is materialization authority.

## Reviewer Reject Signals

Reject reopening this decomposition as progress if the change:

- resumes monolithic `930930-1` chasing without a focused seam;
- treats raw BIR adjacency, final homes, storage, offsets, object ids, value
  names, testcase names, or dump order as materialization authority;
- claims the scalar compare destination probe proves downstream authority;
- folds storage-only movement, select-result boundary, and materialization
  authority into one testcase-shaped shortcut;
- rewrites expectations, unsupported markers, allowlists, pass/fail accounting,
  runtime comparison, or baseline logs as substitute capability progress.
