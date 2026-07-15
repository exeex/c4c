# Current Packet

Status: Active
Source Idea Path: ideas/open/818_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one body-parameter use authority row

## Just Finished

- Plan Step 1 selection complete: the exact 819 plain fixed scalar
  `LirBinOp.lhs` row is receiver-ready for a later 734 receipt. Its only
  receiver-consumable authority fields are the matching native `LirValueId`,
  parameter index, `LirTypeRef`, current-function `LinkNameId` owner,
  `LirNativeBodyParameterAbi::DirectScalar`, and
  `LirScalarBinaryParameterRole::Lhs`; `LirBinOp.lhs` is the matching SSA
  value/type binding. The allowed ABI is DirectScalar only. The verifier
  transactionally rejects absent, invalid, duplicate, foreign-owner,
  out-of-range, non-scalar/type-or-ABI-incoherent, wrong-role, and lhs
  value/type-mismatch forms; `param_slots`, rendered text/signatures,
  `LirOperand::raw`, diagnostics, and testcase identity remain forbidden.
  No Raw-BIR receipt and no 818 Steps 2–3 work occurred. The exact consumer
  return point is 734 Step 7.35, "Receive the one 818-authorized
  body-parameter authority row."

## Suggested Next

- Supervisor may select the next bounded 818 packet; retain this one selected
  DirectScalar `LirBinOp.lhs` row and do not start Raw-BIR receipt work.

## Watchouts

- `param_slots`, rendered names/signatures, `LirOperand::raw`, diagnostics,
  and testcase identity are forbidden authority.
- Do not touch Raw-BIR/importer/dispatcher/receiver work or reuse accepted
  817 / separate 795 authority as this row.
- 819 permits only the exact plain fixed scalar `LirBinOp.lhs` tuple with
  `DirectScalar` ABI and `Lhs` role; do not generalize to rhs, another
  operator, or another parameter form.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log` passed 6/6;
  the required log is `test_after.log`. This selection/trace proof does not
  imply a Raw-BIR receiver proof.
