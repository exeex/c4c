Status: Active
Source Idea Path: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce Local Address Evidence

# Current Packet

## Just Finished

Activated `ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md`
into `plan.md` and initialized this execution scratchpad for Step 1.

## Suggested Next

Execute Step 1 by reproducing one retained representative, preferably
`src/20000519-1.c`, through semantic BIR, prepared BIR, and the RV64 object
route in the same workspace snapshot. Record the exact command, failing access,
semantic BIR address record, prepared evidence location, and RV64 rejection
point here.

## Watchouts

- Do not reconstruct local-memory facts from RV64 target-specific instruction
  shapes.
- Do not combine this route with direct-call metadata repair.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not use named-case shortcuts for retained torture representatives.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.

## Proof

Activation validation:

```sh
git diff --check -- plan.md todo.md ideas/open && scripts/plan_review_state.py show
```

Result: passed; plan-review state reset and aligned to Step 1,
`Reproduce Local Address Evidence`.
