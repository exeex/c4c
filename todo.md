Status: Active
Source Idea Path: ideas/open/586_uniform_target_register_identity_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adapt Consumers Without Broad Lowering Rewrites

# Current Packet

## Just Finished

Step 3 from `plan.md` adapted narrow prepared/prealloc ABI consumers to carry
shared target register identity without broad lowering rewrites.

Implementation notes:
- Changed files: `src/backend/prealloc/regalloc.hpp`,
  `src/backend/prealloc/regalloc/move_records.hpp`,
  `src/backend/prealloc/regalloc/move_records.cpp`,
  `src/backend/prealloc/regalloc/call_moves.cpp`,
  `src/backend/prealloc/regalloc.cpp`, `src/backend/prealloc/calls.hpp`,
  and `src/backend/prealloc/call_plans.cpp`.
- `PreparedMoveResolution` and `PreparedAbiBinding` now optionally publish
  destination `PreparedTargetRegisterIdentity` alongside existing ABI register
  placement/name facts.
- ABI call argument, call result, and function return move-resolution paths
  populate identity through
  `target_register_identity_for_abi_register_placement(...)` when the ABI
  placement has a stable shared identity.
- Prepared call argument/result plans and call-boundary effect endpoints carry
  those optional identities forward from ABI bindings, value homes, or the
  shared helper fallback.
- Existing target lowering and final register rendering still consume prepared
  placements/spellings where they need target operands; this packet did not
  rewrite AArch64, x86, or RV64 call lowering.
- Identity-less/fail-closed shapes remain `std::nullopt`: stack ABI
  destinations, missing placement, non-ABI placement pools, multi-register or
  contiguous ABI placements, x86 vector ABI placements, AArch64 `x8` sret
  without shared placement policy, and any unsupported target/ABI shape rejected
  by the shared helper.

## Suggested Next

Execute Step 4 from `plan.md`: add focused identity publication tests for RV64,
AArch64, and x86 stable ABI argument/result placements, including at least one
fail-closed unsupported or identity-less shape if practical.

## Watchouts

- Step 4 should assert `PreparedTargetRegisterIdentity` facts directly rather
  than only register spelling or placement text.
- Keep target-lowering/rendering paths placement-based unless a consumer
  genuinely needs physical identity; final instruction operands still require
  target register representations.
- AArch64 `x8` sret, x86 vector ABI placements, stack destinations, and
  multi-register/contiguous ABI shapes should remain identity-less/fail-closed
  unless a later plan explicitly changes shared placement policy.

## Proof

Command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Proof log: `test_after.log`.
