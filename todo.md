# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 2/3 control-flow contract packet in
`src/backend/prealloc/legalize.cpp` and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by teaching shared
prepare to publish `source_branch_block_label`,
`source_true/source_false_transfer_index`, and incoming labels for the
ordinary phi-lowered compare-join base case, then tightening the joined-branch
ownership test so it now fails if x86 falls back to predecessor-label matching
instead of consuming the prepared true/false transfer mapping.

## Suggested Next

The next small Step 3 packet is broadening the same ownership assertion to one
trailing-join arithmetic family such as xor or and, so the prepared
true/false transfer mapping is proven beyond the base join after this contract
publication slice.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only empty
  branch-only carriers on the way to the prepared join are in scope.
- Shared lowering now publishes ordinary compare-join true/false transfer
  ownership for the base phi-lowered lane, but x86 still keeps a label-based
  fallback for routes that do not yet expose those indices; remove that
  fallback only when the remaining families are covered by the same contract.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof both passed; `test_after.log` is the canonical proof
log and was sufficient for this handoff-boundary Step 3 ownership packet.
