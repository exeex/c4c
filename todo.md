Status: Active
Source Idea Path: ideas/open/01_shared_call_plan_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume the Shared Decision in AArch64 Emission

# Current Packet

## Just Finished

Step 3 migrated AArch64 callee-saved preservation population and
republication to consume shared
`PreparedCallBoundaryEffectPlan` preservation effects.

`lower_before_call_moves()` now selects
`PreservationHomePopulation` entries from
`plan_prepared_call_boundary_effects()` instead of iterating
`call_plan.preserved_values` as its preservation decision source.
`lower_after_call_moves()` likewise selects
`PreservationRepublication` entries from the prepared boundary-effect plan.

The preservation emission helpers now receive the selected prepared effect and
derive register, placement, width, occupied-register, value, and reason facts
from the effect endpoints. Block-entry republication was kept aligned with the
same endpoint-driven helper path. `backend_aarch64_call_boundary_owner_test`
now covers callee-saved population and republication through the shared
prepared effects without weakening existing contracts.

## Suggested Next

Run Step 4 proof/guard work for this migrated family: compare the current diff
against the source idea for overfit risk, then run the supervisor-selected
broader or regression-guard subset for call-boundary preservation.

## Watchouts

- Do not start AArch64 calls file consolidation before shared authority is
  proven.
- Do not hard-code AArch64-only facts into the shared planner.
- Do not weaken tests or add named-case matching as proof of progress.
- AArch64 still owns live-use gates for whether a prepared preservation effect
  should emit at block entry or before/after a call. This packet moved the
  preservation storage decision source, not those liveness decisions.
- `find_prior_preserved_value_for_value()` still checks previous preservation
  state to avoid redundant population; it is not the current-call preservation
  decision source.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$"; } > test_after.log 2>&1'`

Proof passed: build completed and all three selected tests passed:
`backend_call_boundary_effect_plan`,
`backend_aarch64_call_boundary_owner`, and
`backend_codegen_route_aarch64_prepared_call_boundary_scalability`.
Proof log: `test_after.log`.
