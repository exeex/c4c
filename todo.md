Status: Active
Source Idea Path: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Pin The Prepared Classifier Boundary

# Current Packet

## Just Finished

Activation initialized from `ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md`; no executor packet has run yet.

## Suggested Next

Execute Step 1 in `plan.md`: pin the prepared classifier boundary for `src/20001026-1.c`, identify the exact move-bundle shape and rejecting helper or diagnostic path, and record evidence under `build/agent_state`.

## Watchouts

- Do not treat this as integer div/rem lowering until the prepared classifier boundary is crossed.
- Do not change runtime comparison, expected output, unsupported markers, or allowlists.
- Do not special-case `src/20001026-1.c` by name.
- Keep generic `unsupported_instruction_fragment` rows in idea 570, not this active plan.

## Proof

Lifecycle activation proof only:

- `git diff --check -- plan.md todo.md`
- `scripts/plan_review_state.py show`
