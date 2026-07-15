# Current Packet

Status: Active
Source Idea Path: ideas/open/826_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected truthiness-comparison LHS authority contract
你該做code review了

## Just Finished

- Step 2 complete: published the native `DirectScalar` integer parameter
  authority on `LirCmpOp` only for `StmtEmitter::to_bool_operand`'s exact
  truthiness relation: matching LHS `LirValueId` and type, integer `ne`, and
  authoritative integer-zero RHS. The verifier fail-closes missing, invalid,
  duplicate-definition, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority; focused producer coverage exercises the
  positive row and malformed variants.

## Suggested Next

- Step 3: record the exact 734 receiver handoff with the selected tuple,
  consumer relation, rejection boundary, proof, and bounded receiver return
  action.

## Watchouts

- Do not edit Raw-BIR/importer/receiver code or reopen accepted 734 parameter
  rows.
- Do not derive identity from names, signatures, rendered operands,
  diagnostics, `monostate`, or testcase shape.
- `LirCondBr.condition` is a comparison-result authority, not the direct
  parameter-use seam; the published row remains bounded to `LirCmpOp.lhs`.

## Proof

- Passed: `cmake --build --preset default`; `ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`. The delegated packet
  prohibits changing canonical `test_before.log` and `test_after.log`, so no
  regression log was written.
