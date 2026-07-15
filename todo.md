# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_operand_provenance_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Propagate checked SSA operand provenance

## Just Finished

- Step 1 trace complete.  The real HIR producer route is `CallExpr` through
  `emit_rval_operand` -> `emit_rval_call_operand` ->
  `emit_post_builtin_call_operand` -> `emit_call_with_result`, followed by a
  complex `UnaryExpr` (`BitNot`, `RealPart`, or `ImagPart`) through
  `emit_rval_operand` -> `emit_rval_expr` -> `emit_rval_payload(UnaryExpr)`.
  The call helper already returns `LirOperand`, but aggregate/composite call
  results take its non-authoritative `LirOperand(fresh_tmp(ctx))` branch;
  `fresh_value(ctx)` is currently selected only for direct integer and a
  narrow scalar-floating result contract.  Thus this producer needs an
  opt-in existing `LirValueId` at its definition, not an ID inferred later.
- The concrete conversion-to-string loss is
  `emit_rval_id`, which returns `emit_rval_operand(...).str()`.  In the unary
  producer it assigns that string to `val`, and the exact construction seam is
  `LirExtractValueOp{real_v/op_ty/val/index}` (or `tmp/llvm_ty(op_ts)/val/index`)
  in `expr/misc.cpp`; aggregate use construction then has only a display
  spelling.  The coordinator's generic `emit_rval_expr` fallback also wraps
  every string-returning payload in `LirOperand::raw(...)`, so the opt-in path
  must bypass both string conversions.  No text, `%t` spelling, printer, or
  instruction-order recovery is admissible.
- Selected minimum carrier: reuse `lir::LirOperand` (its existing SSA
  alternative is `LirValueId` plus compatibility display) end-to-end, with a
  narrowly introduced operand-returning unary/extractvalue producer entry
  selected by `emit_rval_operand`; keep existing string payload APIs as
  compatibility paths.  On the direct aggregate-call producer, allocate the
  same `fresh_value(ctx)` SSA operand only for this opt-in aggregate handoff;
  do not generalize every expression or add `LirExtractValueOp` row fields.
  Construct the extract operand directly from that carrier.
- Required Step 2 checks: the carrier must be `SsaValue` with a valid
  `LirValueId`, resolve to exactly one definition in the current `LirFunction`
  (not missing, stale, duplicate, or foreign), and have the defining call's
  structured `return_type` equal to the extract aggregate `agg_type`; display
  must mirror the selected definition but never choose it.  Existing
  `verify_module` definition/use collection and `LirCallOp.return_type` /
  `LirTypeRef` checks are the narrow verifier seams; row-specific result/index
  publication remains 754 work.
- Candidate proof surfaces for Steps 2-3: a frontend HIR-to-LIR case that
  makes a direct complex/anonymous aggregate return feed `__real__` or
  `__imag__`, asserting the produced call and aggregate operand share native
  IDs; and a focused LIR verifier fixture adjacent to
  `make_rv64_anonymous_aggregate_return_extractvalue_module` that rejects a
  missing, unknown/foreign, or return-type-incoherent aggregate carrier while
  leaving misleading `%t` text unable to repair it.  The latter must become a
  structured fixture before it can test this contract; its current operands
  are raw compatibility text.

## Suggested Next

- Step 2 only: propagate the selected opt-in `LirOperand`/`LirValueId` from
  the direct aggregate call through the unary extractvalue producer and add
  the bounded current-function/type-coherence validation; do not publish the
  `LirExtractValueOp` row.

## Watchouts

- The current generic call policy deliberately leaves aggregate returns raw,
  and the generic expression fallback intentionally materializes raw payload
  text.  Change only the selected producer route; do not add
  `LirExtractValueOp` result/use fields or recover an ID from a `std::string`.
  754 resumes at its unchanged Step 2 only after this handoff.

## Proof

- Trace-only packet; no build/test and no root proof log were run.  Evidence:
  `c4c-clang-tool-ccdb function-signatures .../expr/misc.cpp
  build/compile_commands.json`; `c4c-clang-tool-ccdb find-definition ...
  emit_rval_operand|emit_rval_id|emit_lir_op build/compile_commands.json`
  (the latter reports cross-TU symbols absent, then source declarations and
  definitions were located); targeted `rg`/`sed` reads of
  `expr/coordinator.cpp`, `expr/misc.cpp`, `call/builtin.cpp`,
  `call/target.cpp`, `operands.hpp`, `verify.cpp`, and the nearby backend
  fixture.  Preserve 754 references `d8e5ed3a8` and root baseline/prototype
  logs as historical evidence only; the prototype was rejected and reverted.
