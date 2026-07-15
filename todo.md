# Current Packet

Status: Active
Source Idea Path: ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish checked authority for selected aggregate producers

## Just Finished

- Step 1 trace selected two producer-specific extensions to the existing
  `LirOperand::ssa(display, LirValueId)` handoff. Aggregate local loads already
  create `fresh_value(ctx)` in `emit_decl_ref_rval_operand` and retain the
  local-pointer receipt; mark only the selected aggregate load as a native
  result producer, then carry that operand through unary real/imag to
  `LirExtractValueOp.agg`. Constructed complex aggregates in
  `emit_complex_binary_arith` / conversion helpers instead use `fresh_tmp` and
  return strings; the selected terminal `LirInsertValueOp` must publish a
  `fresh_value` result plus a narrow opt-in native-result flag and retain that
  operand to its extract use.
- The ownership seam is `definition_insts` in
  `verify_function_value_ownership`: accept only opted-in `LirCallOp`, selected
  `LirLoadOp`, or selected `LirInsertValueOp` producers; require producer
  result-ID equality, current-function definition, producer aggregate type
  (`return_type`, `type_str`, or `agg_type`) equality with `extract.agg_type`,
  and an exact display mirror. Keep all other producer kinds fail-closed.
- First Step 2 implementation attempt rejected and unaccepted: its fresh build
  passed, but the eight-test focused command failed 3/8. Generalized producer
  verification incorrectly imposed an opt-in flag on legacy SSA extracts
  (`backend_lir_to_bir_interface`), and modeling aggregate load type as the
  anonymous pointee violated `LirAllocaOp.local_object_authority` raw-pointee
  equality. The GCC complex path still lacks a valid aggregate ID.

## Suggested Next

- Step 2 repair only: restore the rejected 803-specific hunks without touching
  preserved 801/802 work. Gate authority checks on explicit selected-extract
  `requires_native_result_authority`; publish selected producer IDs plus a
  dedicated aggregate producer-type carrier, leaving raw local-object pointee
  facts unchanged. Add malformed proof for missing/invalid, foreign or
  unknown, stale display, wrong producer kind, and type-incoherent authority.

## Watchouts

- Do not use display text or raw fallback, broaden to generic operand/
  expression provenance, add extractvalue index/layout/result rules, or touch
  Raw-BIR. Existing `modeled_scalar_result_type` is insufficient for
  `LirInsertValueOp`; Step 2 needs a narrowly named aggregate producer-type
  selection rather than treating all instruction results as aggregates.
  Do not change `LirAllocaOp.local_object_authority` or its raw-pointee
  equality rule. Preserve 754 Step 2 for the exact post-handoff return.

## Proof

- Reproduce with fresh build then `ctest --test-dir build -R
  '^(positive_sema_ok_call_builtin_runtime_c|llvm_gcc_c_torture_src_complex_2_c|frontend_hir_tests$|backend_)' --output-on-failure`.
- Current named failures are the runtime and direct-complex aggregate paths;
  raw classification is explicitly rejected because it evades authority.
- No new proof was run for this trace-only packet; retain the existing focused
  reproduction evidence and do not overwrite canonical logs.
- Before accepting Step 2, run a fresh build, focused positive/malformed
  aggregate proof, and the supervisor-selected broader acceptance required
  for the 754 handoff.
- Required focused command: `ctest --test-dir build -R
  '^(positive_sema_ok_call_builtin_runtime_c|llvm_gcc_c_torture_src_complex_2_c|frontend_hir_tests$|backend_)' --output-on-failure`.
