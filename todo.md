# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority

## Just Finished

- Step 2 published `LirCallOp.structured_args[0]` authority only for an
  unchanged native DirectScalar current-function parameter passed as fixed
  direct-call argument 0, with fail-closed verifier and focused malformed
  tuple coverage, including duplicate matching native-definition rejection.

## Suggested Next

- Record the selected producer authority and bounded Raw-BIR receiver boundary
  for the Step 3 handoff back to 734.

## Watchouts

- Do not restore, accept, overwrite, or co-commit the preserved 821/822
  material. Restore it only with `git apply
  review/828_preserved_821_822_frontend_slice.patch` when its owning route is
  authorized.
- The authority remains limited to structured argument 0 of a direct,
  non-variadic, specified call and rejects pointer, spilled/load-derived,
  aggregate/vector, later-argument, indirect, and other parameter forms.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build
  --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1). Per the
  delegated packet, no root test log was written or changed.
