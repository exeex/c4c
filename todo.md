# Current Packet

Status: Active
Source Idea Path: ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Div/Rem Lowering Boundary

## Just Finished

Lifecycle activation created the runbook for Step 1
(`Reconstruct Div/Rem Lowering Boundary`) from
`ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md`.

## Suggested Next

Executor packet for Step 1: extract the `30` routed `integer_div_rem` rows from
the Step 4/Step 5 classification artifacts, run the supervisor-selected
representative baseline allowlist, and identify the exact RV64 object-emission
hook for semantic div/rem lowering.

## Watchouts

- Use the refreshed coherent 2026-07-03 row artifacts, not the stale 137-row
  or mixed-time 179-row evidence from the earlier classification run.
- Keep screened-out F128, producer/prepared, ABI/call, evidence-gap,
  shift-right, pointer-cast, scalar-FP, and heterogeneous scalar-integer rows
  out of this implementation plan.
- Reject testcase-name dispatch, opcode-text-only matching, expectation
  rewrites, unsupported downgrades, and allowlist-only progress.
- Leave `review/557_step13_vector_local_memory_review.md` untouched.

## Proof

- Lifecycle-only activation; no CTest proof required.
- Required activation checks before handoff:
  - `git diff --check -- plan.md todo.md`
  - `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Reconstruct Div/Rem Lowering Boundary'`
