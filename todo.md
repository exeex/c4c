Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire Obsolete Calls Translation Units

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: retired exactly one obsolete AArch64 calls
translation unit after checking its remaining boundary.

Concrete retirement:

- Deleted `calls_preservation.cpp` and removed it from `src/backend/CMakeLists.txt`.
- Merged the remaining callee-saved preservation home population and
  republication machine-node emitters into `calls_moves.cpp`.
- Kept `find_move_bundle` exported from `calls_moves.cpp` because
  `calls_dispatch_bridge.cpp` still consumes that lookup helper.
- Made the argument-source preservation lookup and preservation-effect emitters
  local to `calls_moves.cpp`, removing their stale public declarations from
  `calls.hpp`.

## Suggested Next

Proceed to Step 5 acceptance assessment: compare the remaining AArch64 calls
surface against the source idea and decide whether closure or a narrower
follow-up runbook is appropriate.

## Watchouts

- `calls_preservation.cpp` no longer had a clear standalone emission-only
  boundary: its remaining exported helpers were consumed by `calls_moves.cpp`
  after prepared call-boundary effect planning became the authority.
- `find_move_bundle` cannot be hidden yet because `calls_dispatch_bridge.cpp`
  still uses it outside `calls_moves.cpp`.
- `calls_argument_sources.cpp` and `calls_printing.cpp` still have distinct
  boundaries and were intentionally left in place for this packet.
- No tests or expectations were weakened.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build passed; delegated CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
