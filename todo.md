# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 packet by publishing
the materialized compare-join entry compare/test shape on the prepared param-
zero branch contract: `src/backend/prealloc/prealloc.hpp` now carries a
`CompareShape::SelfTest` field through the param-zero branch helper and the
materialized compare-join branch plan, `src/backend/mir/x86/codegen/
prepared_module_emit.cpp` now renders the compare-join branch prefix from that
prepared contract instead of hardcoding the param-zero `test`/`jcc` setup
inline, and `tests/backend/backend_x86_handoff_boundary_test.cpp` now proves
the same branch-shape contract survives the normal and
`PreparedJoinTransferKind::EdgeStoreSlot` render-contract paths.

## Suggested Next

Stay in Step 3 and keep tightening prepared branch consumption by reusing the
same prepared param-zero compare-shape contract anywhere adjacent x86
branch-only consumers still reconstruct the entry compare prefix from local
branch metadata instead of one shared prepared branch seam.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- This packet publishes only the existing param-zero self-test shape; do not
  justify new compare families, new matcher lanes, or value-location work from
  the new `CompareShape` field without another bounded Step 3 packet.
- The compare-join path now consumes prepared entry-branch shape data, but the
  same route should stay semantic and contract-driven rather than turning the
  new field into a testcase-shaped branch enum.
- The broader `^backend_` checkpoint still has the same four known failures in
  variadic and dynamic-member-array semantic lowering outside this packet's
  owned files.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed and preserved `test_after.log` at the
repo root.
