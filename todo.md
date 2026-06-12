Status: Active
Source Idea Path: ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Safe Production Consumers And Retain Required Public Surface

# Current Packet

## Just Finished

Completed Step 2 - Introduce Private Pass-Context Branch-Target Identity Read
by routing only the agreement-proven BIR structured successor id read through
`detail::BranchTargetIdentityPassContext` and
`detail::read_agreeing_bir_branch_target_labels(...)` in
`src/backend/prealloc/control_flow.hpp`.

The public three-argument
`find_prepared_control_flow_branch_target_labels(...)` still calls the
two-argument prepared helper first. If prepared labels are absent, or if the
private BIR pass-context read sees a missing block, non-conditional
terminator, invalid successor id, or prepared/BIR id mismatch, the helper
returns the original prepared result. Only the positive agreement path returns
labels read from the BIR terminator through the private context boundary.

No public prepared facade was removed or reshaped, no aggregate
`PreparedControlFlow`, `PreparedFunctionLookups`, or `PreparedBirModule`
surface was retired, and no printer/debug, wrapper, oracle, expected-string,
or output behavior was changed.

## Suggested Next

Proceed to Step 3: migrate only safe production consumers that need the
selected branch-target identity read to the private boundary, while explicitly
retaining public prepared access for compare-branch policy, fallback,
printer/debug, helper-oracle, wrapper, and expected-string responsibilities.

## Watchouts

- The two-argument helper remains the retained prepared fallback/public
  authority for absent, invalid-label, non-conditional, mismatch, and invalid-id
  paths.
- `resolve_prepared_compare_branch_target_labels(...)` still has
  policy/oracle semantics: missing control-flow agreement can preserve prepared
  branch-condition targets, but target drift returns `std::nullopt`.
- AArch64 `comparison.cpp` remains the only direct production
  selected-identity call found by `rg`; x86 production consumers use the
  compare resolver wrapper and handoff validators.
- Keep prepared printer/debug rows, x86 joined-branch wrapper outputs,
  helper-oracle strings/statuses, and expected strings byte-stable.
- No aggregate `PreparedControlFlow`, `PreparedFunctionLookups`, or
  `PreparedBirModule` retirement belongs in Step 2.
- The new `detail` helper is the private read boundary; do not broaden the
  next packet into aggregate API retirement or facade reshaping.

## Proof

Supervisor-selected proof command ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering' > test_after.log 2>&1
```

Result: passed. `test_after.log` records 2/2 passing:

- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`
