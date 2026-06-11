Status: Active
Source Idea Path: ideas/open/193_route3_prepared_policy_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Broaden And Document Readiness

# Current Packet

## Just Finished

Completed Step 4 by documenting the accepted Route 3 boundary and readiness
state for the indirect-callee stored-value-source work completed in Steps 2-3.

Accepted boundary:

- Route 3 may identify target-neutral same-block memory/source identity through
  `route3_find_same_block_load_local_source` and
  `route3_find_same_block_load_local_stored_value_source` when BIR load/store
  memory facts agree.
- Route 3 must not authorize prepared or target-owned policy. Addressing,
  frame/slot layout, relocation, volatility, materialization, direct-global
  select-chain policy, and final AArch64 operand decisions remain outside Route
  3 route facts.
- Missing, mismatched, ambiguous, or policy-sensitive Route 3 facts must stay
  on prepared fallback instead of being treated as route-first identity proof.

Retained prepared oracle/fallback surfaces:

- `prepare::find_prepared_same_block_load_local_stored_value_source` remains
  the prepared stored-source fallback/oracle for the selected indirect-callee
  case when Route 3 memory identity is unavailable or unsafe.
- Prepared source-producer lookups, including
  `prepare::find_indexed_prepared_edge_publication_source_producer`, remain the
  public oracle for prepared publication source producer identity.
- `prepare::find_prepared_direct_global_select_chain_dependency` remains the
  direct-global/select-chain policy oracle required before AArch64 lowers the
  selected indirect callee.
- AArch64 prepared addressing and materialization helpers remain authoritative
  for printable memory operands and emitted target instructions.

Residual blockers:

- This packet does not claim draft 155 readiness.
- This packet does not claim `PreparedBirModule` readiness or a prepared-surface
  retirement point.
- Future Phase E retirement still needs a separate analysis proving equivalent
  route-native coverage before any public prepared oracle/fallback surface is
  deleted.

## Suggested Next

Closure recommendation: treat the current runbook packet as complete and route
to the plan owner for lifecycle close/deactivate/split judgment. Any draft 155,
`PreparedBirModule`, or prepared-surface retirement work should be a separate
idea or later plan, not an expansion of this completed boundary-hardening
packet.

## Watchouts

- The accepted boundary is readiness for Route 3 source-identity use at the
  selected indirect-callee consumer, not general prepared-contract retirement.
- Keep the Step 3 positive/negative tests as the consumer proof: matching BIR
  memory facts permit Route 3 identity, while absent BIR memory range authority
  preserves prepared fallback and target policy.
- Do not weaken supported-path expectations or replace retained prepared
  oracles with draft-field assumptions as part of closure.

## Proof

Docs/lifecycle-only packet; no new build was required by the supervisor.

Accepted proof already rolled into `test_before.log`:

`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log"`

Result: build succeeded; 180/180 `backend_` tests passed, with matching
non-decreasing regression guard. Current proof log path for the accepted
broader run: `test_before.log`.
