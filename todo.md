# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Consumers To The Authoritative Prepared Facts
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed a `plan.md` Step 3 slice for idea 62. The minimal compare-branch
route in `src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` now
requires authoritative prepared branch metadata whenever prepared control-flow
exists, and `tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp`
now proves the canonical prepared-module handoff rejects both drifted and
missing prepared branch records instead of reopening raw compare-driven branch
recovery.

## Suggested Next

Advance another bounded `plan.md` Step 3 compare-branch packet that extends the
same contract-strict prepared-branch rejection proof to the remaining minimal
nonzero and parameter-leaf compare-driven entry variants, or switch to the next
consumer family that still reopens raw branch recovery when authoritative
prepared metadata is removed.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Keep `PreparedBranchCondition` and `PreparedControlFlowBlock` targets
  contract-consistent; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- Keep Step 3 packets focused on consumer migration proof, not on reopening
  Step 2.3-style fallback cleanup that already landed for local-slot and
  countdown handoff surfaces.
- Keep the minimal compare-branch route aligned with the same prepared
  branch-condition and prepared-target contract that the broader guard-chain
  family already requires; do not reopen raw compare or terminator recovery
  once the prepared control-flow block is authoritative.
- Prefer authoritative prepared-metadata drift proofs over adding new
  compare-shape-specific fallback logic in x86 codegen.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after making the minimal
compare-branch prepared-module consumer reject missing authoritative prepared
branch records and adding handoff-boundary coverage for both drifted and
missing prepared compare-branch metadata. `test_after.log` is the proof
artifact for this packet.
