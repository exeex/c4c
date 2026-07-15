# LIR Ternary/Coerce Arm Result Authority Publication Runbook

Status: Active
Source Idea: ideas/open/781_lir_ternary_coerce_arm_result_authority_publication.md
Activated from: 775's reassessment after accepted 777 vaarg and 778 logical-RHS handoffs.

## Purpose

Resolve one independently evidenced ternary/coerce producer loss before
reassessing the larger 775 helper-result contract.

## Goal

Publish native result authority for one selected scalar ternary arm coercion,
without making PHI or generic-expression changes.

## Core Rule

Allocate or carry authority as a native `LirOperand::ssa` before rendering.
Display spelling may project that authority, but must never recreate it.

## Read First

- `ideas/open/781_lir_ternary_coerce_arm_result_authority_publication.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`
- `ideas/closed/779_lir_cast_result_authority_contract.md`
- `ideas/closed/780_lir_cross_function_value_id_ownership_restoration.md`
- `src/codegen/lir/hir_to_lir/expr/misc.cpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- no generic `emit_rval_*` / `coerce` API migration
- no other ternary arm, PHI result/incoming, final ternary consumer, logical,
  vaarg, or other expression family work
- no PHI/CFG/751, text recovery, maps, side tables, Raw-BIR/importer, backend,
  target lowering, MIR, emission, or baseline-log work

## Ordered Steps

### Step 1 - Publish the selected ternary arm coercion result

Goal: carry the selected scalar ternary arm through the smallest typed
operand/coercion path needed to emit an owning `LirCastOp.result` before its
display spelling.

Primary targets:

- `src/codegen/lir/hir_to_lir/expr/misc.cpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- the existing typed coercion helper only if a narrow overload is required

Actions:

- preserve the selected arm's type and existing conditional control flow;
- keep the string route as a compatibility projection, not an authority source;
- do not change the raw ternary PHI result/incoming pairs or final consumer;
- do not widen callers beyond the selected scalar ternary path.

Completion check:

- the selected emitted ternary arm coercion has a valid current-function native
  result ID, with no generic API or PHI-carrier change.

### Step 2 - Prove selected-arm authority and failure closure

Goal: add focused structural positive and malformed-authority coverage for
only the selected ternary arm coercion result.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- assert the coercion result's native ID without display-text or order matching;
- prove missing, invalid, duplicate, and foreign result authority rejects;
- retain explicit assertions that the PHI boundary is outside this packet.

Completion check:

- focused positive and malformed cases prove the native verifier contract with
no text recovery or testcase-shaped shortcut.

### Step 3 - Publish the bounded 775 handoff

Goal: record the selected field, accepted proof, and unresolved PHI/final
consumer boundary for 775's later reassessment.

Completion check:

- 775 receives only a selected ternary arm/coercion fact; it is not reactivated
and 751 remains blocked.

## Proof

- For Steps 1-2: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
- The supervisor owns regression logs, baseline selection, and final acceptance.
