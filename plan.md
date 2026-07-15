# LIR PHI Producer Helper Result Identity Then-Arm Runbook

Status: Active
Source Idea: ideas/open/775_lir_phi_producer_helper_result_identity.md
Repaired from: Step 1 reassessment, which found the first remaining loss in the
selected scalar ternary `then` arm.

## Purpose

Publish native result authority for the selected scalar ternary `then`-arm
coercion before compatibility spelling, completing only this next bounded
producer repair for 775's later three-family reassessment.

## Goal

Route the selected `then` arm from `emit_rval_id` through a bounded typed
operand/coercion entry so its emitted `LirCastOp.result` has an owning,
current-function `LirValueId` before rendering.

## Evidence

In `src/codegen/lir/hir_to_lir/expr/misc.cpp`, accepted 781 already routes the
selected `else` arm through `emit_rval_operand` to `coerce_operand`. The
selected `then` arm in the same ternary function remains the first loss:
`emit_rval_id` followed by string `coerce`.

## Core Rule

Native result authority must be carried before display spelling. Do not derive
it from `%t` names, labels, rendered output, instruction order, or testcase
identity.

## Read First

- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/closed/781_lir_ternary_coerce_arm_result_authority_publication.md`
- `src/codegen/lir/hir_to_lir/expr/misc.cpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- no work beyond the selected ternary `then` arm
- retain, without revisiting, accepted selected `else`-arm, logical-RHS, and
  vaarg producer facts
- no raw ternary PHI result, either incoming carrier, later final-consumer
  input, 751, generic `emit_rval_*`/`coerce` API migration, Raw-BIR, backend,
  or baseline-log work
- no side tables, synthetic values, or text-derived authority

## Execution Rules

1. Change only the narrow typed operand/coercion route required by the selected
   scalar `then` arm; compatibility strings are projections, not authority.
2. Keep the raw PHI boundary and final consumer untouched.
3. Use the existing native verifier contract to fail closed for malformed
   authority.
4. Do not claim a full ternary, PHI, 751, or Raw-BIR solution from this packet.

## Ordered Steps

### Step 1 - Publish selected then-arm coercion result authority

Goal: route only the selected scalar ternary `then` arm through the bounded
typed operand/coercion entry and publish a valid current-function
`LirCastOp.result` before compatibility spelling.

Primary targets:

- `src/codegen/lir/hir_to_lir/expr/misc.cpp`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` only if the existing narrow
  typed entry requires a matching bounded call site

Actions:

- replace the selected `emit_rval_id` to string-`coerce` authority loss with
  the existing narrow typed operand/coercion route;
- preserve the selected arm's type and conditional control flow; and
- retain raw PHI result/incomings and the later consumer exactly as raw
  compatibility boundaries.

Completion check:

- the selected emitted then-arm coercion has a native, valid,
  current-function-owned result ID before display spelling, without a generic
  API or PHI-carrier change.

### Step 2 - Prove selected then-arm authority and failure closure

Goal: add focused structural positive and malformed-authority coverage for the
selected then-arm coercion result.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- prove the native result structurally, without rendered-text or order probes;
- prove missing, invalid, duplicate, and foreign result authority rejects via
  the existing verifier; and
- retain explicit proof that the raw PHI and later-consumer boundaries remain
  outside this route.

Completion check:

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passes with focused
  structural positive and fail-closed malformed coverage.

## Proof

- Required focused proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- The supervisor owns regression-log and broader acceptance selection.
