# Current Packet

Status: Active
Source Idea Path: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Isolate the vaarg helper result path

## Just Finished

- Plan Step 3 complete: added `test_logical_short_circuit_result_authority_loss_boundary`,
  an independent frontend-LIR probe for `(lhs && rhs) + 3`. It verifies the two
  boolean conversions, branch condition/successor IDs, non-`i1` RHS `zext`, typed
  `i32` PHI and later typed `i32` add facts. The RHS conversion result, PHI result,
  and the later add's left operand have neither `LirValueId` nor other `LirOperand`
  authority.
- First loss seam: `emit_logical` receives structured boolean conversion results for
  the LHS branch and RHS conversion operand, but creates non-`i1` `rhs_val` with
  `fresh_tmp`, then creates the final PHI result with another `fresh_tmp` and returns
  raw text. `LirPhiOp::incoming` stores only `(std::string, std::string)` pairs, and
  the verifier only requires a PHI result/type and non-empty incoming list; therefore
  no verifier-backed malformed-ID proof is faithful. The required repair direction is
  a typed logical result carrier through RHS conversion, PHI incoming values/result,
  and the later consumer. No production repair, ternary coverage, or vaarg coverage is
  claimed.

## Suggested Next

- Begin Plan Step 4 with an independent focused frontend-LIR probe of the vaarg
  helper / `emit_lir_op` result path; ternary and logical remain outside that
  packet.

## Watchouts

- A logical repair needs a typed carrier through non-`i1` RHS conversion, PHI incoming
  values/result, and the later consumer; do not infer that capability from this
  diagnostic probe.
- Do not treat this logical observation as coverage for ternary or vaarg paths.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed. The delegated packet forbade modifying the
  supervisor-owned `test_after.log`, so no root regression log was written by this packet.
