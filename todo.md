Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 removed the selected retained metadata decision for small AArch64 byval
register-lane size.

- Deleted `aarch64_register_byval_argument_size_bytes` from `calls.hpp` and
  `calls_byval_aggregates.cpp`.
- Replaced its remaining `lower_before_call_move` uses in `calls_moves.cpp`
  with the prepared-lane extent path:
  `is_aarch64_byval_register_lane_move(...)` through
  `prepared_byval_lane_extent_bytes(...)`, consuming
  `PreparedMoveResolution`, `PreparedCallArgumentPlan`, and
  `PreparedValueHome::size_bytes`.
- Kept the packet limited to the selected small register-lane byval path; the
  indirect and stack byval helpers were not widened into this slice.
- Updated the focused instruction-dispatch fixture so the small byval payload
  lane is represented by the prepared move reason
  `call_arg_byval_aggregate_register_lanes` instead of relying on retained
  `CallInst::arg_abi` to reinterpret a generic move.

## Suggested Next

Execute Step 3 for this selected target: consolidate the affected helper
boundary now that the obsolete small register-lane byval size helper is gone,
or document the surviving byval helper boundaries as emission-only where they
still serve indirect or stack byval paths.

## Watchouts

Do not treat the surviving retained ABI reads in
`aarch64_indirect_byval_argument_size_bytes`,
`aarch64_stack_byval_argument_size_bytes`, or
`aarch64_indirect_register_byval_argument` as part of this completed Step 2
slice; those were explicitly out of scope for this packet.

The instruction-dispatch byval fixture now requires the prepared move reason
for small byval payload-lane lowering. Reintroducing support for generic moves
would require a new prepared fact, not a restored AArch64-local `arg_abi` read.

## Proof

Ran the supervisor-selected proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, `162/162` backend tests in `test_after.log`.
