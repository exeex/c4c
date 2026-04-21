# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Step 2.1 attempted a generic x86-local pointer-binary repair in
`render_prepared_local_slot_guard_chain_if_supported`, but the delegated proof
still left `c_testsuite_x86_backend_src_00204_c` on the same idea-60 scalar
restriction. Follow-up inspection narrowed the blocker further: after
prepared-module lowering, the reduced `match` route no longer exposes explicit
pointer `BinaryInst` nodes for `f++`, `p++`, or `p - 1`; instead the prepared
route still carries named pointer updates like `%t22`, `%t24`, and `%t34`
directly into pointer stores. That means this packet is blocked outside the
attempted x86-only binary-dispatch seam and stopped without a shippable code
repair.

## Suggested Next

Expand the next idea-60 packet into the shared prepared pointer-value /
addressing surface for the reduced `match` route: inspect how `%t22`, `%t24`,
and `%t34` are represented after prepared-module lowering, then repair the
missing prepared pointer rematerialization / value-home / addressing contract
that x86 local-slot rendering still cannot consume generically. Keep the next
proof on the same backend handoff subset and only return to x86-only edits once
the prepared route exposes a durable pointer source the renderer can follow.

## Watchouts

- Do not keep extending x86-local pointer matchers just because the reduced C
  source shows `f++`, `p++`, and `p - 1`. The prepared route reaching x86 no
  longer presents those updates as standalone pointer binaries, so another
  emitter-local lane would be route drift.
- Keep the blocker in idea 60 unless the expanded prepared-pointer inspection
  shows the route has fallen back before scalar emission into a different leaf.
- Preserve the reduced `match` ownership notes: prepared compare-join
  short-circuit metadata is already past the old idea-61 handoff seam, so the
  remaining gap is specifically generic pointer-value consumption after prepare.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the idea-60 scalar
restriction. Supporting investigation confirmed that `match` lowers to a
pointer-backed loop/return BIR route before prepare, but the prepared-module
surface reaching x86 no longer prints explicit pointer `BinaryInst` updates for
the reduced `%t22` / `%t24` / `%t34` values. A local x86 pointer-binary
experiment compiled but did not change the proof result and was reverted, so
the worktree remains clean. Canonical proof log: `test_after.log`.
