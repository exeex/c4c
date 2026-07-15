# Current Packet

Status: Active
Source Idea Path: ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the focused successor and hand off the parent gate

## Just Finished

- 807 Step 2 completed: floating `UnaryOp::Minus` now allocates its `fneg`
  result with `fresh_value(ctx)`. Focused coverage proves both floating ternary
  PHI incoming operands retain their producing `fneg` native current-function
  `LirValueId`, and removing the selected `fneg` result authority is rejected
  by the existing verifier.

## Suggested Next

- Execute 807 Step 3: obtain fresh build and focused same-feature proof, then
  have the supervisor accept it; retain 806 parked at Step 3 pending 808 and
  the required follow-on full baseline.

## Watchouts

- The repair is isolated to scalar floating `UnaryOp::Minus` producer handoff.
  Do not reopen PHI/ternary or verifier contracts, or absorb complex/vector or
  other unary producer families.

## Proof

- Passed fresh `cmake --build --preset default`, then passed
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`;
  focused positive and malformed `fneg` authority coverage are included in
  `frontend_lir_call_type_ref`. Proof log: `test_after.log`.
