# Current Packet

Status: Active
Source Idea Path: ideas/open/808_lir_phi_scalar_bit_not_xor_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the focused successor and hand off the parent gate

## Just Finished

- Step 2 complete: the scalar `UnaryOp::BitNot` branch now creates its `xor`
  result with `fresh_value(ctx)`, publishing a checked native
  current-function ID while vector lowering remains unchanged. Added adjacent
  `condition ? ~left : ~right` coverage that verifies two scalar `xor`
  producers, one PHI, incoming producer-ID equality, and rejection after the
  selected PHI-referenced `xor` producer result is removed.

## Suggested Next

- Supervisor acceptance of the Step 2 focused proof, then resume 806 Step 3
  for its required 100% full baseline; do not claim 806 or 804 clearance here.

## Watchouts

- Keep this route limited to scalar bit-not `xor`; vector and complex bit-not,
  floating or scalar unary-minus, postfix old-value, PHI/ternary consumer,
  and verifier contracts remain out of scope. Existing verifier coverage
  preserves missing, unknown, foreign, and stale authority rejection.

## Proof

- Passed: `cmake --build --preset default`; then `ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
  The supervisor-selected focused proof is sufficient for this packet;
  `test_after.log` is preserved. After acceptance, 806 still requires the
  100% full baseline before 804 can resume.
