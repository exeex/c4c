Status: Active
Source Idea Path: ideas/open/01_shared_call_plan_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume the Shared Decision in AArch64 Emission

# Current Packet

## Just Finished

Step 3 regression fix kept migrated AArch64 callee-saved preservation helpers
consuming `PreparedCallBoundaryEffectPlan` endpoint facts for source and
destination data while normalizing the emitted population move reason back to
`callee_saved_preservation_home_population`, the selected printable AArch64
callee-saved preservation subset.

`backend_aarch64_call_boundary_owner_test` now asserts that the shared-effect
population still uses the prepared destination register but emits the selected
AArch64 population reason instead of the shared planner's generic
`preservation_home_population` reason.

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
- Shared planner reasons are not automatically printable AArch64 move-selector
  reasons. Keep the selected AArch64 reason on emitted preservation population
  records unless the selector contract is deliberately widened.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00168_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00200_c|c_testsuite_aarch64_backend_src_00214_c|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$"; } > test_after.log 2>&1'`

Proof passed: build completed and all ten selected tests passed:
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00040_c`,
`c_testsuite_aarch64_backend_src_00168_c`,
`c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00181_c`,
`c_testsuite_aarch64_backend_src_00200_c`,
`c_testsuite_aarch64_backend_src_00214_c`,
`backend_call_boundary_effect_plan`,
`backend_aarch64_call_boundary_owner`, and
`backend_codegen_route_aarch64_prepared_call_boundary_scalability`.
Proof log: `test_after.log`.
