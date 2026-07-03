# Current Packet

Status: Active
Source Idea Path: ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Large-Offset GPR Boundary

## Just Finished

Activated the runbook from `ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md`.

## Suggested Next

Execute Step 1: reproduce and classify the `src/20030209-1.c` large-offset GPR callee-saved frame-slot boundary before implementation.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- Do not fold FPR callee-saved work back into this GPR large-offset idea.
- Do not synthesize prepared frame facts in RV64 lowering.
- Do not treat expectation, allowlist, or unsupported-marker edits as progress.

## Proof

- `git diff --check -- plan.md todo.md` passed.
- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Inspect Large-Offset GPR Boundary'` completed.
- `scripts/plan_review_state.py show` matches Step 1.
