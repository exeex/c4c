# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify one binary-LHS authority tuple

## Just Finished

- Step 1 trace complete: `StmtEmitter::emit_decl_ref_rval_operand` in
  `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` produces the native
  parameter `LirOperand::ssa("%p.<name>", value)` from
  `ctx.param_value_authorities`; `StmtEmitter::emit_binary_rval_operand` in
  `src/codegen/lir/hir_to_lir/expr/binary.cpp` is the sole native scalar
  arithmetic producer of the resulting `LirBinOp.lhs`.
- In its authoritative scalar integer/floating branch, that producer preserves
  the source LHS authority only when its normalized spelling is identical, then
  publishes `scalar_lhs_parameter_authority` only after finding one current
  function definition matching `(lhs.value_id, operation LirTypeRef,
  DirectScalar ABI)`. The verifier's
  `verify_scalar_binary_lhs_authority` missing-authority relation instead
  finds `(op.lhs.value_id, DirectScalar ABI)` without the type predicate and
  aborts if that relation exists while the tuple is absent.
- Therefore the focused abort proves the producer left a retained native
  DirectScalar LHS unrepresented. The exact predicate seam to resolve is a
  definition whose value/ABI matches `lhs` but whose `definition.type` differs
  from the binary operation `type`; that fails producer selection yet triggers
  verifier rejection. A null function context or absent LHS value ID cannot by
  itself trigger this verifier relation.

## Suggested Next

- Step 2: add focused positive and malformed binary-LHS coverage in
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`, then
  repair only `emit_binary_rval_operand`'s DirectScalar LHS publication path
  so its selected definition and `LirBinOp` operation type satisfy the
  existing exact tuple contract; do not modify or rely on selector authority.

## Watchouts

- Do not touch or credit the preserved unaccepted Idea 825 `core.cpp`,
  `ir.hpp`, `verify.cpp`, or focused-test changes.
- Do not expand into direct selector, Raw-BIR/importer, RHS, return, pointer,
  generic parameter, or presentation-derived authority routes.
- The Step 2 owner must preserve the verifier's required tuple: valid value,
  unique current-function owner, in-range parameter index, DirectScalar ABI,
  LHS role, SSA LHS/value equality, operation-type equality, and exactly one
  matching native parameter definition.

## Proof

- `git diff --check` passed. This trace-only packet made no code/test changes;
  no CTest or `test_after.log` is required. Before Step 2 acceptance, require
  a fresh build and exact independently attributable
  `^frontend_lir_function_signature_type_ref$` CTest after nearby
  positive/malformed binary-LHS coverage. Do not claim or require the full
  `^frontend_lir_call_type_ref$` CTest here: it remains the parent/composite
  checkpoint after Idea 825 resumes.
