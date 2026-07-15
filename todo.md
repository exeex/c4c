# Current Packet

Status: Active
Source Idea Path: ideas/open/824_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one receiver-candidate parameter-use row

## Just Finished

- Step 1 trace failed closed on one distinct candidate: an unchanged direct
  plain-scalar current-function parameter in `LirRet::value_str`.  The native
  definition published by `lower_function` (`hir_to_lir.cpp`, parameter loop)
  supplies its `LirValueId`, `LinkNameId` owner, index, typed signature mirror,
  and `DirectScalar` ABI; `emit_decl_ref_rval_operand`
  (`expr/coordinator.cpp`) preserves that ID, and
  `StmtEmitter::emit_control_flow_stmt(ReturnStmt)` (`stmt.cpp`) forwards it
  only for same-representation integer returns through `emit_term_ret`
  (`core.cpp`) into the exact return-value operand relation.
- The first missing fact is a role-specific native parameter-to-return binding:
  `LirRet` has only its generic `value_str` ID/type fields.  `verify_use` and
  `verify_terminator` (`verify.cpp`) prove a known current-function value and
  integer return shape, but do not require exactly one matching parameter
  definition or publish ReturnValue role/owner/index/type/ABI.  Therefore this
  is not a receiver-ready row and no presentation spelling was used.

## Suggested Next

- Step 2: add only an optional direct-scalar ReturnValue parameter-authority
  carrier on `LirRet`, publish it from the unchanged integer return path, and
  verify exact current-function definition/owner/index/type/ABI/value/role
  agreement plus missing, malformed, foreign, duplicate, and nonselected
  rejection.  This is in-scope authority publication, not a separate blocker.

## Watchouts

- Do not recover authority from text or select a generic parameter family.
- Do not edit Raw-BIR/importer code or reopen accepted pointer/DirectScalar
  LHS/RHS rows.
- Keep the candidate bounded to unchanged same-representation integer returns;
  coercing, pointer, floating, aggregate, and raw return paths remain
  nonselected and fail closed.

## Proof

- Read-only trace only; no build/test required and no `test_after.log` was
  created. Evidence: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` parameter
  publication, `expr/coordinator.cpp::emit_decl_ref_rval_operand`,
  `stmt.cpp::StmtEmitter::emit_control_flow_stmt(ReturnStmt)`,
  `core.cpp::StmtEmitter::emit_term_ret`, `ir.hpp::LirRet` and
  `LirCurrentFunctionBodyParameterDefinition`, and `verify.cpp` native
  parameter, value-use, and terminator verification.
