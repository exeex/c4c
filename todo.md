# Current Packet

Status: Active
Source Idea Path: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Isolate the logical short-circuit result path

## Just Finished

- Plan Step 2 complete: added `test_ternary_coerce_result_authority_loss_boundary`,
  a standalone frontend-LIR probe for an `int`/`long long` ternary whose result is
  consumed by a later `long long` add. It verifies the semantic cast, PHI, and later
  binary type facts, then independently observes that the coerced arm result, PHI
  result, and consuming binary operand have neither `LirValueId` nor other
  `LirOperand` authority.
- First loss seam: ternary lowering calls `emit_rval_id` for each arm, applies the
  string `coerce`, constructs its PHI result with `fresh_tmp`, and returns raw text.
  No typed ternary result reaches the later consumer. `LirPhiOp::incoming` is also
  text-only, and the verifier only checks a PHI result plus non-empty incoming list;
  therefore corrupting a structured ID would not be a faithful malformed-proof
  contract. The focused positive structure is the truthful proof of the missing
  typed-result requirement. No production repair or logical/vaarg coverage is claimed.

## Suggested Next

- Plan Step 3: isolate the logical short-circuit result path with an independent focused
  frontend-LIR probe; vaarg remains outside this completed ternary/coerce probe.

## Watchouts

- A ternary repair needs a typed result carrier through both arm coercions, PHI incoming
  values, and the later consumer; do not infer that capability from this diagnostic probe.
- Do not treat this ternary observation as coverage for logical or vaarg paths.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed. The delegated packet forbade modifying the
  supervisor-owned `test_after.log`, so no root regression log was written by this packet.
