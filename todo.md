Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 consolidated the affected helper boundary after
`aarch64_register_byval_argument_size_bytes` was removed.

- Verified the removed helper has no surviving declaration or caller outside
  this packet note.
- Removed the dead `prepared_byval_lane_extent_bytes(...)` probe from the
  non-byval frame-slot-to-register branch in `calls_moves.cpp`; that branch
  already excludes `is_aarch64_byval_register_lane_move(move)`.
- Kept `prepared_byval_lane_extent_bytes(...)` local to prepared small byval
  register-lane emission, where it consumes prepared move, argument, and value
  home authority.
- Left the surviving retained ABI helpers as future blockers for separate
  indirect and stack byval slices:
  `aarch64_indirect_byval_argument_size_bytes`,
  `aarch64_stack_byval_argument_size_bytes`, and
  `aarch64_indirect_register_byval_argument`.

## Suggested Next

Supervisor should decide whether to commit this Step 3 consolidation slice or
delegate the next slice for one surviving retained ABI byval helper boundary.

## Watchouts

The surviving retained ABI helpers are not retired by this packet. They remain
valid only for the current indirect-register and stack byval emission paths
until those paths gain prepared size/class authority.

The small register-lane byval path should continue to require prepared move
reason `call_arg_byval_aggregate_register_lanes`; generic frame-slot register
moves should not recover that classification locally.

## Proof

Ran the supervisor-selected proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, `162/162` backend tests in `test_after.log`.
