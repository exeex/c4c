Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 of `plan.md` repaired the byval register-lane extent replacement after
the selected `byval_register_lane_size_bytes` authority path was removed. The
helper is no longer declared or defined, and the matched AArch64 byval
register-lane emission branches now derive their aggregate byte extent from
prepared move, argument, source-home, and contiguous prepared store facts
instead of re-reading `CallInst::arg_abi`.

## Suggested Next

Supervisor should route the next coherent packet for Step 3 of `plan.md`.

## Watchouts

- `source_home->size_bytes` can already be the prepared aggregate extent. The
  replacement helper now preserves that total and only uses the prepared lane
  count when the home is lane-sized and no contiguous store coverage proves a
  larger extent.
- Contiguous prepared store coverage remains the stronger prepared fact for
  reconstructed fragmented byval publications.
- Other byval helper reads of `CallInst::arg_abi` remain outside this selected
  Step 2 removal path.

## Proof

Reproduced and fixed the focused guard:
`ctest --test-dir build --output-on-failure -R '^backend_aarch64_call_boundary_owner$'`.
Result after rebuild: passed.

Ran the exact delegated final proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
Result: passed, 162/162 tests. Proof log: `test_after.log`.
