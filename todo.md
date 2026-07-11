# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Attach complete routing facts at the prepared owner boundary

## Just Finished

- Step 6.1 attached complete edge-derived current-block routing facts to
  `PreparedFunctionLookups` during owner construction.
- The stable-key consumption query now accepts the prepared owner directly;
  its owner-backed contract covers available, missing, ambiguous, and
  mismatched states, and lookup-owner copies retain the attached collection.

## Suggested Next

- Execute Step 6.2 by switching AArch64 current-block consumption to the
  owner-backed stable-key query and removing local routing-fact reconstruction.

## Watchouts

- The vector-based query remains as a low-level compatibility helper; the
  stable owner-based overload is the boundary Step 6.2 should consume.
- AArch64 still reconstructs per-block facts locally and must be switched in
  Step 6.2 without treating Route 5 as authority.
- Ideas 713 and 705 remain open and blocked pending handback.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`.
- Result: 317/317 backend tests passed; the supervisor-selected proof was
  sufficient; proof log: `test_after.log`.
