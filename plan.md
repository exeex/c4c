# LIR Vaarg Result Authority Publication Runbook

Status: Exhausted — close accepted pending supervisor lifecycle review
Source Idea: ideas/open/777_lir_vaarg_result_authority_publication.md
Activated from: closed 776's independently evidenced vaarg first-loss handoff.

## Purpose

Repair exactly the semantic vaarg helper result boundary so one native result
can reach its immediate expression consumer with authority.

## Goal

Publish an owning current-function `LirValueId` for the selected `LirVaArgOp`
result without widening into generic expression migration or PHI work.

## Core Rule

Authority is allocated before rendering as native `LirOperand::ssa`; raw
spelling is compatibility/display only and must never be recovered into an ID.

## Read First

- `ideas/open/777_lir_vaarg_result_authority_publication.md`
- `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- no ternary/coerce, logical, or generic expression-result migration
- no PHI carrier/verifier, text recovery, Raw-BIR/importer, backend, map, or
  side-table work
- no target-lowering or non-semantic-vaarg route claim

## Ordered Steps

### Step 1 - Publish the semantic vaarg helper result as a typed operand

Goal: replace the selected VaArgExpr helper's raw `fresh_tmp` result path with
a vaarg-only `LirOperand::ssa` result allocated by `fresh_value`, and route the
VaArgExpr expression entry through that operand without changing generic
payload APIs.

Primary targets:

- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/call.hpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`

Actions:

- introduce only the VaArgExpr-specific typed result entry/return needed for
  the semantic `LirVaArgOp` route;
- preserve raw compatibility spelling only as a projection of the authoritative
  operand, never as an ID source;
- do not edit PHI carriers, other expression payload families, or target
  lowering.

Completion check:

- the selected `LirVaArgOp.result` owns a valid current-function ID and the
  immediate typed vaarg consumer receives that same ID structurally.

### Step 2 - Prove the vaarg result/use authority contract

Goal: add focused native frontend-LIR positive and fail-closed malformed proof
for the selected vaarg route.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- extend the vaarg-only probe to require typed result/use identity without
  display-spelling assertions;
- prove missing, invalid, duplicate, and foreign vaarg result authority is
  rejected by the native verifier contract;
- do not add PHI or other-family cases.

Completion check:

- focused positive and malformed vaarg authority cases pass/fail structurally
  as required, with no text recovery or testcase-shaped shortcut.

### Step 3 - Publish the bounded 775 handoff

Goal: record the exact vaarg field, proof, remaining exclusions, and 775
return point without reactivating 775 or 751.

Completion check:

- 775 can consume a vaarg-only native result-authority handoff while ternary,
  logical, PHI, and generic expression work remain separate.

Completed handoff:

- Selected field and route: scalar AMD64 semantic `LirVaArgOp.result` is
  allocated with `fresh_value` as an owning current-function `LirValueId`,
  enters `emit_lir_op` as `LirOperand::ssa`, then travels through the
  VaArgExpr-specific `LirOperand` route to the immediate typed Add with the
  exact same native value ID.
- Focused proof: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passed. The focused
  native verifier cases reject missing, invalid, duplicate, and foreign vaarg
  result authority. The supervisor accepted the regression guard and full
  baseline review for commit `55c499775`.
- Exact 775 return point: 775 may consume this vaarg-only typed-result
  handoff. Ternary/coerce and logical remain unresolved; do not reactivate 751
  or claim PHI incoming-carrier work from this result.
- Disposition: this vaarg-only producer runbook is capability-complete within
  its source scope and may close. It does not authorize generic expression
  migration, nonsemantic vaarg target-lowering work, text recovery, result-name
  maps, side tables, PHI work, Raw-BIR/importer/backend work, or a broader
  helper-family capability claim.

## Proof

- For Steps 1–2: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`
- Before any final close, the supervisor selects and accepts the required
  regression proof; this runbook does not own root regression logs.
