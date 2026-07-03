Status: Active
Source Idea Path: ideas/open/547_bir_local_memory_call_metadata_boundary_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Local-Memory Evidence

# Current Packet

## Just Finished

Activated `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`
into `plan.md` and initialized this execution scratchpad for Step 1.

## Suggested Next

Execute Step 1 by reconstructing the current `unsupported_local_memory_access`
row evidence from existing RV64 gcc torture backend artifacts, then record row
counts, representative cases, group names, and unresolved owner questions here.

## Watchouts

- Do not implement RV64 lowering in this review runbook.
- Do not treat call-metadata suspicions as implementation-ready before row-level
  evidence exists.
- Do not weaken tests, unsupported markers, expected output, or pass/fail
  accounting.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.

## Proof

Activation validation:

```sh
git diff --check -- plan.md todo.md && scripts/plan_review_state.py reset && scripts/plan_review_state.py set-step --step-id 1 --step-title 'Reconstruct Local-Memory Evidence'
```

Result: passed with no output.
