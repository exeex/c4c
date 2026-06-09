Status: Active
Source Idea Path: ideas/open/146_call_argument_materialization_call_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Definitions and Reconnect Shared Dependencies

# Current Packet

## Just Finished

Completed Step 3 definition ownership move.

- `prepared_call_argument_binary_producer_opcode_is_materializable` and
  `find_prepared_call_argument_source_producer_materialization` definitions now
  live in `src/backend/prealloc/call_plans.cpp`.
- `src/backend/prealloc/prepared_lookups.cpp` no longer defines either public
  call-argument materialization symbol.
- The moved materialization lookup still delegates to
  `find_prepared_same_block_scalar_producer` for same-block source-producer
  discovery instead of duplicating that scan.

## Suggested Next

Next packet: supervisor-selected follow-up for the remaining call-argument
materialization ownership route, with no ABI move-bundle or AArch64 emission
policy changes folded into the same slice.

## Watchouts

- AST checks confirmed the public definitions now resolve in
  `src/backend/prealloc/call_plans.cpp` under `c4c::backend::prepare`.
- Keep `find_prepared_same_block_scalar_producer` shared in the select-chain
  lookup owner; this packet intentionally left those definitions in
  `src/backend/prealloc/prepared_lookups.cpp`.
- Do not mix ABI move-bundle work, AArch64 emission/register policy, or
  expectation rewrites into the next packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Regression guard passed with 179/179 backend tests before and after,
no new failures.
Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
