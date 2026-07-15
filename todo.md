# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_stack_restore_lifetime_consumer_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish the exact return handoff to 794

## Just Finished

- Plan Step 2: implemented the selected `LirStackRestoreOp` admission and
  saved-VLA-checkpoint transition, with native verifier rejection for malformed
  transition and saved-pointer/object/owner/type/liveness bindings.

## Suggested Next

- Execute Plan Step 3: publish the exact native stack-restore fields, rejected
  forms, and focused proof in the return handoff to 794.

## Watchouts

- `local_object_authority.live` remains a checkpoint-binding validity fact;
  the transition does not create per-VLA allocation lifetime state. Do not
  absorb dynamic-VLA count work, VLA GEP, other local/lifetime rows,
  Raw-BIR/importer/734 receipt, or presentation-derived facts.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`; focused selected
  restore producer/verifier positive and negative coverage passed. Proof log:
  `test_after.log`.
