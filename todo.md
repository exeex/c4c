# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by collapsing the
duplicated short-circuit branch-render cases into a single helper over
`ShortCircuitTarget`. The x86 consumer now decides whether a compare arm
renders its opposite lane from one shared path instead of maintaining separate
special-case render branches.

## Suggested Next

The next small Step 3 packet is to simplify
`detect_short_circuit_plan_from_control_flow()` in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` so the prepared
join-transfer indices map into `ShortCircuitTarget` construction through one
direct helper instead of separate `assign_short_circuit` and `assign_rhs`
closures.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only the
  prepared predecessor recorded by the selected join-transfer edge is
  authoritative for continuation entry, whether the rhs block branches directly
  to the join or through one empty carrier.
- Do not reintroduce arm-local booleans or continuation fallbacks that bypass
  the rhs branch block's own compare; continuation ownership now lives on the
  selected `ShortCircuitTarget`, and prepared predecessor ownership, not
  speculative CFG rescue, is the active Step 3 contract.
- Keep the new shared short-circuit render helper neutral about CFG shape: it
  may notice when one compare arm already renders the opposite lane, but it
  must continue to rely on prepared predecessor ownership carried by the
  selected continuation payload rather than on emitter-local topology guesses.
- Do not treat `source_true_incoming_label` or `source_false_incoming_label` as
  the x86 continuation ownership contract here; the prepared-control-flow tests
  intentionally rewrite those aliases while the authoritative predecessor still
  lives in `edge_transfers[source_*_transfer_index].predecessor_label`.
- The existing base and trailing binary compare-join proofs still exercise the
  same generalized helper path; keep future Step 3 work on that shared route
  instead of cloning testcase-shaped ownership logic.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof both passed; `test_after.log` is the canonical proof
log and was sufficient for this Step 3 short-circuit-render cleanup packet
after the duplicated branch assembly paths were folded into one helper.
