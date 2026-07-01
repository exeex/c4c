Status: Active
Source Idea Path: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Create Focused Follow-Up Ideas

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by creating five focused follow-up source ideas
from the row-backed infrastructure handoff:

- `ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md`
- `ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md`
- `ideas/open/510_rv64_selected_object_data_emission.md`
- `ideas/open/511_static_local_object_data_contract_publication.md`
- `ideas/open/512_stack_passed_parameter_home_publication.md`

The follow-ups preserve the Step 3 split: ideas 508-510 are RV64 consumer
work for coherent prepared facts, while ideas 511-512 are producer-contract
work where RV64 must remain fail-closed until authority is published. Each
idea links back to idea 424, cites the handoff doc and representative artifact
paths, records owning layer, scope, acceptance criteria, and concrete reviewer
reject signals.

## Suggested Next

Execute Step 5: validate and hand off the infrastructure review for closure
or deactivation decision.

## Watchouts

- Step 5 should confirm idea 424 is complete as a review/handoff route, not as
  implementation of the five new follow-up ideas.
- Keep the owner split intact: RV64 emission candidates are
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

Step 4 lifecycle proof is written to `test_after.log`:

- `scripts/plan_review_state.py set-step --step-id 4 --step-title 'Create Focused Follow-Up Ideas'`
- `git diff --check -- todo.md ideas/open`
