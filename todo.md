# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority

## Just Finished

- Step 1 selected the native DirectScalar current-function parameter used
  unchanged as fixed direct-call argument 0. Closed 828 then recoverably
  isolated the unrelated 821/822 dirty frontend slice; its committed artifact
  is `review/828_preserved_821_822_frontend_slice.patch` at `7b0f8671e`.

## Suggested Next

- Implement only the explicit `LirCallOp.structured_args[0]`
  `FixedDirectCallArgument0` authority and its fail-closed producer/verifier
  coverage, then run the stipulated fresh build and focused frontend proof.

## Watchouts

- Do not restore, accept, overwrite, or co-commit the preserved 821/822
  material. Restore it only with `git apply
  review/828_preserved_821_822_frontend_slice.patch` when its owning route is
  authorized.
- Do not admit later arguments, indirect/variadic/unspecified calls, pointer,
  spilled/load-derived, aggregate/vector, or any other parameter form.

## Proof

- Required Step 2 acceptance proof: fresh `cmake --build --preset default`,
  then `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'`. The 828 matching baseline/after 1/1 result
  establishes worktree isolation only, not Step 2 semantic acceptance.
