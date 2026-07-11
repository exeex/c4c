# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Separate fixture policy from attachment

## Just Finished

- Step 5 replaced the fixture's boolean-only policy marker with real prepared
  lookup policy and exercised the AArch64 current-block source query.
- The supported axis now carries complete edge-derived authority and succeeds
  only when that policy is attached; policy-absent and detached axes fail
  closed independently.

## Suggested Next

- Execute Step 6, preserving the independently proven policy/attachment axes
  while continuing the active runbook.

## Watchouts

- Detachment must remove both the prepared lookup pointer and fallback value
  lookup paths; otherwise the query can reconstruct policy from module state.
- Ideas 713 and 705 remain open and blocked pending handback.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`.
- Result: 317/317 backend tests passed; the supervisor-selected proof was
  sufficient; proof log: `test_after.log`.
