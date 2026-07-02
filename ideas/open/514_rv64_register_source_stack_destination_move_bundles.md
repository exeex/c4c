# RV64 Register-Source Stack-Destination Prepared Move Bundles

Source Parent: ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md
Owning Layer: RV64 object emission consuming prepared move-bundle authority, with producer classification checked before materialization

## Goal

Handle or precisely reclassify prepared move bundles whose destination is a
stack value but whose explicit source home is a register, including multi-move
bundles that currently share the `consumer_stack_to_stack` reason string.

## Why This Exists

Idea 513 intentionally accepted only coherent same-scalar stack-slot to
stack-slot moves. Step 5 evidence showed that two representative rows still
fail at `unsupported_move_bundle_target_shape`, but their first failures are
not stack-to-stack moves under explicit prepared facts:

- `src/20010518-1.c`: a multi-move bundle with sources in registers `a0`/`a1`
  targeting one stack value.
- `src/pr27073.c`: a single move from `%p.count` in register `a4` to a stack
  destination despite the `consumer_stack_to_stack` reason string.

Those cases need a semantic owner that decides whether register-source to
stack-destination moves should be materialized directly, rewritten into a
clearer prepared move class, or rejected earlier with a more precise producer
diagnostic.

## In Scope

- Reproduce the register-source to stack-destination prepared move bundles for
  `src/20010518-1.c` and `src/pr27073.c`.
- Determine whether the explicit prepared facts authorize register-to-stack
  materialization or whether producer classification is wrong.
- If authorized, add a narrow RV64 materialization path for explicit
  register-source to stack-destination value moves.
- If not authorized, repair the producer or diagnostic so the failure no
  longer masquerades as stack-to-stack materialization.
- Add focused backend coverage for accepted and rejected register-source to
  stack-destination prepared move shapes.

## Out Of Scope

- Inferring register or stack locations from row names, argument indexes,
  source order, ABI formulas, or textual BIR matching.
- General parallel-copy lowering, broad multi-move bundle scheduling, or cycle
  resolution beyond what the explicit prepared facts require.
- Reopening idea 513's stack-slot to stack-slot materializer.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes claimed as capability progress.

## Acceptance Criteria

- The owned register-source to stack-destination shape is either materialized
  from explicit prepared facts or rejected with a diagnostic that identifies the
  real unsupported owner.
- `src/20010518-1.c` and `src/pr27073.c` no longer rely on the generic
  `consumer_stack_to_stack` stack-slot materializer to explain register-source
  failures.
- Focused coverage proves the accepted path or the refined reject path without
  depending on row names.
- Validation includes a fresh build, focused representative proof, focused
  backend coverage, and the relevant backend subset.

## Reviewer Reject Signals

Reject any route or slice that:

- treats `20010518-1.c` or `pr27073.c` as named special cases
- assumes a source stack slot when the prepared home or storage plan says the
  source is a register
- accepts multi-move bundles by silently dropping moves or choosing one source
  by order
- changes only reason strings, expectations, unsupported markers, allowlists,
  or scan accounting while leaving the unsupported owner ambiguous
- broadens into general parallel-copy scheduling without first proving the
  minimal register-source to stack-destination contract

## Lifecycle Handoff

The active runbook that covered Steps 1-6 was retired after producing a bounded
diagnostic/reclassification handoff. Fresh backend validation was recorded in
`todo.md` before the switch, and the representative object-route probes now
fail with precise `unsupported_move_bundle_target_shape` diagnostics instead
of relying on the older stack-to-stack explanation.

Closure was not accepted during this lifecycle pass because the close gate
requires matching canonical regression logs and `test_after.log` was absent
while the delegation explicitly prohibited touching regression logs. The
remaining capability owner is split to
`ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md`.
