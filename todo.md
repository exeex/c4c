# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 packet by moving the
materialized compare-join branch-plan labels onto the prepared branch contract:
`src/backend/prealloc/prealloc.hpp` now makes
`find_prepared_materialized_compare_join_branch_plan()` publish entry labels
from the prepared branch condition instead of mutable predecessor-block labels,
and `tests/backend/backend_x86_handoff_boundary_test.cpp` now proves the same
authoritative labels survive later predecessor relabeling for both the normal
and `PreparedJoinTransferKind::EdgeStoreSlot` compare-join carriers.

## Suggested Next

Stay in Step 3 and keep migrating the materialized compare-join consumer path by
publishing one prepared helper for the entry compare/test shape itself, so the
x86 render path can stop hardcoding the param-zero `test`/`jcc` setup around
the now-authoritative branch and EdgeStoreSlot join labels.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- This packet intentionally moved label ownership, not feature coverage: do not
  justify new compare-join passthrough shapes or emitter-local matcher growth
  from it.
- The materialized compare-join branch plan now ignores post-hoc predecessor
  relabeling, but x86 still hardcodes the surrounding param-zero compare/test
  spelling; keep the next packet focused on one more prepared consumer seam
  instead of broad cleanup.
- The broader `^backend_` checkpoint still has the same four known failures in
  variadic and dynamic-member-array semantic lowering outside this packet's
  owned files.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed and preserved `test_after.log` at the
repo root.
