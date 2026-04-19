# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow In X86 packet by tightening
the next plain compare-driven local-guard seam around authoritative prepared
branch ownership: `src/backend/prealloc/prealloc.hpp` now publishes a shared
helper for prepared i32-immediate branch conditions keyed by block/value name,
and `src/backend/mir/x86/codegen/prepared_module_emit.cpp` uses that helper so
the plain local-guard render path takes compare setup from the prepared branch
contract instead of the live carrier compare when that prepared contract is
available; `tests/backend/backend_x86_handoff_boundary_test.cpp` now proves the
route and helper still publish the canonical semantics after the carrier
compare opcode/operands and branch topology are corrupted.

## Suggested Next

Stay in Step 3 and tighten the next compare-driven entry seam in the
short-circuit or compare-join entry helpers, where x86 still depends on the
carrier compare being locally recognizable even though prepared control-flow
already owns the branch/join semantics.

## Watchouts

- Keep this route in Step 3 consumer work; do not widen into Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- This packet only hardens the plain direct-entry local-guard consumer against
  mutated carrier compare semantics and label state; short-circuit,
  compare-join, and other guard-chain consumers still have their own
  carrier-compare seams and should be tightened separately.
- The route is acceptable because it removes dependence on carrier branch state
  and compare semantics for the covered case; do not regress into new
  emitter-local CFG recovery or
  testcase-shaped guard lanes.
- The broader `^backend_` checkpoint still has the same four known failures in
  variadic and dynamic-member-array semantic lowering outside this packet's
  owned files.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3 handoff proof passed and preserved `test_after.log` at the
repo root.
