# LIR PHI Producer Helper Result Identity Producer-Handoff Runbook

Status: Active
Source Idea: ideas/open/775_lir_phi_producer_helper_result_identity.md
Repaired from: completed selected scalar ternary `then`-arm publication and
proof. The remaining route is a bounded source-level producer reassessment and
handoff decision; it must not absorb raw PHI-carrier work.

## Purpose

Reassess the accepted vaarg, logical-RHS, and both selected ternary-arm
producer facts against 775's remaining acceptance criteria, then publish only
the producer handoff that is actually supported by native structure and proof.

## Goal

Do not claim that raw PHI result/incoming carriers or final consumers are
authoritative. Decide whether the accepted native producer facts satisfy a
bounded handoff to 751, or identify the exact unmet criterion and the next
separate route without generic expression migration or PHI-carrier work.

## Evidence

In `src/codegen/lir/hir_to_lir/expr/misc.cpp`, accepted 781 already routes the
selected `else` arm through `emit_rval_operand` to `coerce_operand`; accepted
commits `a67fc07bd` and `b03baa3a6` establish the equivalent bounded route and
failure closure for the selected `then` arm. Closed 777 and 778 supply the
accepted vaarg and logical-RHS producer facts.

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

- no new implementation beyond the accepted selected ternary `then` arm
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

### Step 3 - Reassess the bounded producer handoff and source disposition

Goal: reconcile 775's accepted one-family producer facts with its remaining
handoff criterion without treating raw PHI carriers as producer authority.

Primary targets:

- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `plan.md`
- `todo.md`

Actions:

- inventory only the accepted vaarg, logical-RHS, selected ternary-else, and
  selected ternary-then native result fields and their proof/commit references;
- determine whether these facts alone prove the source's stated structural
  producer-to-eventual-PHI-seam criterion, keeping raw PHI results/incomings
  and later consumers excluded; and
- if the criterion remains unmet, record the exact criterion and a bounded
  return route. Do not repair it through generic `emit_rval_*`/`coerce`
  migration, `LirPhiOp` representation/verification changes, Raw-BIR, or 751
  implementation.

Completion check:

- lifecycle state names either an evidence-supported bounded producer handoff
  or the exact unresolved PHI-boundary criterion with an executable successor
  or return route; it makes no PHI/751/Raw-BIR capability claim.
