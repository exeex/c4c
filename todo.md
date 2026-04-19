# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Keep Downstream Phi Consumers Contract-Strict
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed a Step 3.2 (`Keep Downstream Phi Consumers Contract-Strict`) slice by
teaching the x86 loop-countdown prepared-module consumer to follow
authoritative loop-carry edge values and branch metadata without requiring
slot-shaped `storage_name` ownership on the prepared join transfer. The
countdown boundary proof now clears the loop-carry join `storage_name` fields
after prepare and still expects the same canonical asm.

## Suggested Next

Continue Step 3.2 by auditing the remaining downstream x86/local countdown and
prepared-join consumers that still key on `EdgeStoreSlot` or slot-shaped join
carrier metadata, then widen the contract-strict proof only where the consumer
can follow authoritative prepared edge values instead of local slot recovery.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- This packet only relaxed the minimal loop-countdown prepared consumer's
  dependence on prepared join `storage_name`; it still requires a supported
  entry handoff carrier block in the prepared BIR and still rejects drifted
  authoritative branch or edge-value metadata.
- The two-segment local countdown fallback tests still exercise explicit
  continuation join ownership with `EdgeStoreSlot`; widen those separately only
  if the consumer can follow authoritative prepared edge values without
  reopening local-slot fallback logic.
- `effective_prepared_join_transfer_carrier_kind(...)` remains a transitional
  compatibility surface for other prepared consumers and manual fixtures, so
  additional downstream cleanups should stay narrowly focused on consumers that
  no longer need slot-shaped join ownership.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepare_(liveness|phi_materialize)|x86_handoff_boundary_loop_countdown)' > test_after.log 2>&1`
ran for this Step 3.2 packet and preserved the canonical proof log in
`test_after.log`; `backend_prepare_liveness`,
`backend_prepare_phi_materialize`, and
`backend_x86_handoff_boundary_loop_countdown` passed with the loop-countdown
consumer no longer requiring slot-shaped prepared join storage metadata.
