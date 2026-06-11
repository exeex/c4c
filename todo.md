Status: Active
Source Idea Path: ideas/open/193_route3_prepared_policy_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define The Boundary In Code Or Tests

# Current Packet

## Just Finished

Completed Step 2 by selecting the AArch64 indirect-callee stored-value-source
consumer boundary in `src/backend/mir/aarch64/codegen/calls.cpp`.

The boundary is now explicit in code:

- `find_route3_indirect_callee_stored_value_source_identity` reads Route 3 first
  and returns only exact target-neutral stored-value identity: stored value plus
  store instruction index.
- `find_prepared_indirect_callee_stored_value_source_fallback` keeps prepared
  source-producer, stack-layout, and addressing authority for absent,
  mismatched, ambiguous, or policy-sensitive Route 3 facts.
- `find_indirect_callee_stored_value_source` documents and enforces the
  route-first/fallback order at the consumer helper surface.
- The downstream indirect-callee materialization path still asks prepared
  direct-global/select-chain facts before retargeting the callee source, so
  AArch64 addressing, relocation, frame, volatility, materialization, and final
  operand policy remain prepared/target-owned.

## Suggested Next

Execute Step 3 by adding focused positive/negative proof around the selected
AArch64 indirect-callee stored-value-source boundary: one exact same-range Route
3 identity case and one prepared fallback case where Route 3 lacks range
authority or must defer to prepared policy.

## Watchouts

- The selected helper returns a local boundary record, not
  `PreparedSameBlockLoadLocalStoredValueSource`, so the Route 3 path cannot
  accidentally imply prepared address/range policy beyond source identity.
- Existing MIR/prepared oracle tests already cover exact Route 3 match and
  prepared fallback when Route 3 lacks range authority, but Step 3 should add or
  tighten AArch64 consumer-level proof rather than relying only on helper-oracle
  coverage.
- No residual blockers for this Step 2 slice.

## Proof

`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log"` passed.

Result: build succeeded; 180/180 `backend_` tests passed. Proof log:
`test_after.log`.
