# Current Packet

Status: Active
Source Idea Path: ideas/open/776_lir_typed_expression_result_carrier_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate the ternary/coerce result path

## Just Finished

- Plan Step 1 complete: the pre-render owning allocation boundary is
  `fresh_value(FnCtx&)` in `src/codegen/lir/hir_to_lir/core.cpp`, which allocates
  a current-function `LirValueId` and returns `LirOperand::ssa`; `fresh_tmp` allocates
  only a `%t` display spelling. `emit_rval_operand` is the typed expression-result
  boundary. `emit_rval_id` immediately calls `.str()`, every `emit_rval_payload`
  overload returns `std::string`, and string `coerce` accepts/returns text, so none
  can carry result authority.
- The existing `coerce_operand(LirOperand, ...)` is the one focused typed conversion
  seam: its supported scalar casts allocate a new `fresh_value` result and retain the
  input operand; its fallback deliberately delegates to string `coerce` and returns
  raw text. Focused contract: `test_scalar_cast_result_use_identity_boundary` in
  `frontend_lir_call_type_ref` requires the load ID to be the cast operand ID and the
  cast result ID to be the later binary-use ID, while display text may be made
  misleading without changing verification. No test-only carrier was added.
- Independent follow-ons: Step 2 owns ternary's `emit_rval_id` plus string `coerce`
  arms and `fresh_tmp` PHI result; Step 3 owns logical's `fresh_tmp` cast/PHI chain;
  Step 4 owns vaarg helper and `emit_lir_op` raw-result chains. This packet makes no
  PHI, backend, or production-migration claim.

## Suggested Next

- Begin Plan Step 2: isolate the ternary/coerce result path with its own frontend-LIR probe.

## Watchouts

- `LirOperand` authority, not `%t` spelling or rendered LLVM, is the result contract.
- Do not treat the scalar `coerce_operand` probe as coverage for ternary, logical, or vaarg.

## Proof

- `cmake --build --preset default` passed, followed by
  `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`:
  1/1 passed. Log: `test_after.log`.
