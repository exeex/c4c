Status: Active
Source Idea Path: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Write Row-Backed Infrastructure Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by writing the durable row-backed infrastructure
handoff:

- `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

The document records bucket counts (`unsupported_stack_frame` 84,
`unsupported_global_data` 40, `unsupported_param_home` 4), cites the Step 2
representative artifact directories, separates coherent RV64 emission gaps
from producer-contract gaps, records parked F128/FPR scope, and lists reject
signals against RV64 reconstruction, expectation/unsupported-marker changes,
and testcase-shaped shortcuts.

## Suggested Next

Execute Step 4: create separate follow-up ideas for the row-backed owners in
the handoff doc:

- RV64 callee-saved GPR save-slot emission.
- RV64 general fixed prepared stack-frame emission.
- RV64 coherent selected object-data emission.
- Producer static-local object-data contract publication.
- Producer stack-passed parameter-home publication.

## Watchouts

- Step 4 should preserve the owner split: RV64 emission candidates are
  callee-saved save slots, general fixed-frame emission, and coherent selected
  object-data emission; producer-contract candidates are static-local object
  labels/extents and stack-passed parameter-home publication.
- Do not turn producer gaps into RV64 inference from testcase shape, source
  names, raw global symbols, or argument indexes.
- Keep F128 local-memory, F128 parameter homes, expectation rewrites,
  unsupported-marker changes, runtime comparison, and pass/fail accounting out
  of this review.
- `src/20000603-1.c` also exposes an FPR save slot, but this packet's first
  stack-frame owner is the coherent prepared save-slot emission gap, not a
  broad FPR/F128 ABI route.

## Proof

Step 3 docs proof was written to `test_after.log`:

- `scripts/plan_review_state.py set-step --step-id 3 --step-title 'Write Row-Backed Infrastructure Handoff'`
- `git diff --check -- todo.md docs/rv64_gcc_torture_post_contract`
