Status: Active
Source Idea Path: ideas/open/586_uniform_target_register_identity_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Identity Publication Tests

# Current Packet

## Just Finished

Step 4 from `plan.md` added focused identity publication tests for stable ABI
argument/result placements across RV64, AArch64, and x86.

Test notes:
- Changed file: `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`.
- Added direct `PreparedTargetRegisterIdentity` assertions through
  `call_arg_destination_register_placement(...)`,
  `call_result_destination_register_placement(...)`, and
  `target_register_identity_for_abi_register_placement(...)`.
- Covered RV64 `a1`/`a0`, AArch64 `x2`/`x0`, and x86-64
  `rdi`/`rsi`/`rax` physical identities.
- Added prepared call-boundary effect assertions that argument/result endpoints
  carry the published optional target identities forward.
- Covered fail-closed or identity-less shapes for x86 vector ABI placement,
  AArch64 contiguous-width ABI placement, and unsupported I686 ABI placement.

## Suggested Next

Execute Step 5 from `plan.md`: validate the slice and inventory remaining
unsupported or intentionally identity-less ABI placement shapes for lifecycle
review.

## Watchouts

- Keep target-lowering/rendering paths placement-based unless a consumer
  genuinely needs physical identity; final instruction operands still require
  target register representations.
- This packet intentionally did not edit implementation files or expectation
  contracts.
- RV64 contiguous-width ABI placement still follows the existing RV64 identity
  helper behavior; Step 5 should inventory that separately from the tested
  AArch64/x86 fail-closed shapes.

## Proof

Command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Proof log: `test_after.log`.
