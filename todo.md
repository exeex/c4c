# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_operand_provenance_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Propagate checked SSA operand provenance

## Just Finished

- Completed Plan Step 2. A `require_direct_aggregate_ssa` opt-in reaches only
  a direct structured aggregate/composite call consumed by unary `BitNot`,
  `RealPart`, or `ImagPart`. That selected call now uses `fresh_value(ctx)`;
  its existing `LirOperand` carrier reaches `emit_unary_rval_operand` and is
  passed directly as `LirExtractValueOp.agg`. The old string payload API is a
  wrapper, and unselected expression paths retain their raw compatibility path.
- `verify_function_value_ownership` checks an authoritative extract aggregate
  against the current function: SSA alternative, known definition, matching
  `LirCallOp` result ID and structured return/aggregate type, plus an exact
  display mirror. Existing definition collection rejects missing, unknown,
  duplicate, and foreign IDs; no display text selects or repairs authority.
  The gate now enters on `SsaValue` kind itself and explicitly rejects an SSA
  spelling with missing or invalid `LirValueId`; raw legacy operands remain
  compatibility paths outside this opt-in check.
  No `LirExtractValueOp` schema/result/index work, backend receiver work, or
  tests changed.

## Suggested Next

- Step 3 only: add focused positive and malformed same-feature proof for the
  direct aggregate-call/unary-extract carrier, then publish the exact 754
  handoff contract.

## Watchouts

- Do not widen the opt-in call/unary route into generic expression authority,
  add `LirExtractValueOp` result/use fields, or recover an ID from a
  `std::string`; 754 remains blocked until Step 3 proof accepts this handoff.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` passed: build succeeded
  and the selected backend subset passed 5/5 (rerun after missing-authority
  repair). `test_after.log` is the proof log. AST trace used
  `c4c-clang-tool-ccdb` signature/caller/callee queries;
  cross-TU caller queries reported targets outside the queried TU, then the
  named source seams were read directly.
