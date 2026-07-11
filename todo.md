# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove all-applicable-edge invariance

## Just Finished

- Step 4 completed the all-applicable-edge invariance contract and registered
  `backend_prealloc_current_block_all_edge_invariance`.
- Distinct predecessors remain valid only when every destination, source, and
  semantic-origin identity agrees; parallel-destination conflicts and exact
  duplicate families fail closed as `Ambiguous`.
- Destination and source IDs/names are covered independently, and mixed
  non-Available families now report deterministic status precedence regardless
  of input order. No AArch64 behavior or expectation changed.

## Suggested Next

- Execute Step 5 composition at the owner-attached query boundary, preserving
  the three focused authority contracts and unchanged AArch64 integration
  expectations.

## Watchouts

- Negative-status precedence is explicit: `Ambiguous`, `Mismatched`,
  `Unsupported`, `Incomplete`, then `Missing`; every negative remains
  non-authoritative.
- Keep Step 5 composition owner-attached and do not reintroduce successor-only
  or target-local reconstruction.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 320/320 backend tests passed, including the focused all-edge matrix,
  corrected parallel-destination probes, incoming-expression authority, and
  unchanged AArch64 current-block routing integration.
- Canonical proof log: `test_after.log`.
