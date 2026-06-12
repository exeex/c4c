Status: Active
Source Idea Path: ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Agreement And Fallback Proof

# Current Packet

## Just Finished

Completed Step 3 - Migrate Safe Production Consumers And Retain Required
Public Surface by moving the safe AArch64 production selected-identity consumer
in `find_prepared_conditional_branch_facts(...)` off the public three-argument
`find_prepared_control_flow_branch_target_labels(...)` wrapper.

Migrated consumer:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
  `find_prepared_conditional_branch_facts(...)`: the non-compare branch path now
  reads the prepared two-argument control-flow target labels for fallback, then
  calls `prepare::detail::read_agreeing_bir_branch_target_labels(...)` with
  `prepare::detail::BranchTargetIdentityPassContext` to use the private BIR
  structured successor identity only on the agreement path.

Retained prepared/public consumers and surfaces:

- `resolve_prepared_compare_branch_target_labels(...)` remains public/prepared
  owned because it carries compare resolver policy/oracle behavior: missing
  control-flow agreement preserves branch-condition targets, while target drift
  returns `std::nullopt`.
- The two-argument
  `find_prepared_control_flow_branch_target_labels(...)` remains the public
  prepared fallback for absent, invalid-label, non-conditional, mismatch, and
  invalid-id paths.
- The three-argument helper remains available for helper-oracle coverage and
  retained public API behavior; production selected-identity use was migrated.
- x86 wrapper consumers continue to use the compare resolver wrapper and
  handoff validators.
- Prepared printer/debug rows, helper-oracle names/statuses, wrapper output,
  expected strings, and aggregate `PreparedControlFlow`,
  `PreparedFunctionLookups`, and `PreparedBirModule` surfaces were not changed.

## Suggested Next

Proceed to Step 4: add focused agreement and fallback proof for the private
branch-target identity boundary without weakening helper-oracle contracts or
changing expected strings.

## Watchouts

- Step 4 should focus on helper proof, not aggregate API retirement or facade
  reshaping.
- Keep the compare resolver policy byte-stable; it is intentionally retained
  on the public/prepared surface.
- Keep prepared printer/debug rows, x86 joined-branch wrapper outputs,
  helper-oracle strings/statuses, and expected strings byte-stable.
- The migrated AArch64 path still preserves prepared fallback when the private
  BIR read is absent, non-conditional, invalid-id, or not in agreement.

## Proof

Supervisor-selected proof command ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering' > test_after.log 2>&1
```

Result: passed. `test_after.log` records 2/2 passing:

- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`
