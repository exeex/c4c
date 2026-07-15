# LIR Ternary/Coerce Arm Result Authority Publication

Status: Closed (capability complete)
Type: one-family typed expression-result producer successor
Blocks: `ideas/open/775_lir_phi_producer_helper_result_identity.md`
Downstream Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
Evidence Source: `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`

## Goal

Publish a native owning current-function `LirValueId` for the selected scalar
ternary arm coercion result before display spelling, without migrating generic
expression APIs or changing the raw PHI carrier.

## Why This Exists

The accepted 776 ternary probe proves the selected conditional's arm is
obtained through `emit_rval_id` and coerced through the string `coerce` API;
the resulting `LirCastOp`, ternary PHI, and later consumer all lose native
authority. The direct arm-coercion emission is a smaller producer boundary
than 775's all-family helper contract. Closed 779 supplies the standalone cast
result verifier contract and closed 780 restores module-wide value-ID
ownership, so this route can test one native cast result without reopening
either contract.

## In Scope

- introduce only the typed operand/coercion entry needed by the selected
  scalar ternary arm in `emit_rval_payload(TernaryExpr)`;
- allocate or forward the selected emitted `LirCastOp.result` as an owning
  current-function `LirValueId` before rendering its display spelling;
- keep compatibility strings as projections of that native operand;
- add focused frontend-LIR positive and missing, invalid, duplicate, and
  foreign-result rejection coverage for that selected arm/coercion result; and
- publish a narrow handoff to 775 naming the typed field, proof, and the raw
  PHI boundary still left unresolved.

## Out Of Scope

- generic `emit_rval_payload`, `emit_rval_id`, or `coerce` API migration;
- the other ternary arm, ternary PHI result/incoming pairs, final ternary
  consumer, logical, vaarg, or any unrelated expression family;
- `LirPhiOp` representation/verification, CFG predecessor work, 751 work,
  text-derived recovery, maps, side tables, or synthetic values;
- Raw-BIR/importer, backend, target lowering, MIR, emission, baseline-log
  changes, or testcase-specific branches.

## Acceptance Criteria

- The selected scalar ternary arm's emitted coercion result is a native,
  valid, current-function-owned `LirValueId` before any display spelling is
  used.
- Focused frontend-LIR coverage proves the result structurally and proves
  missing, invalid, duplicate, and foreign authority fail closed through the
  existing native verifier contract.
- The handoff explicitly leaves ternary PHI result/incoming entries and the
  later ternary consumer raw and unresolved; it makes no 751 or Raw-BIR claim.

## Reviewer Reject Signals

- Reject parsing `%t` names, labels, printed LLVM, instruction order, or
  testcase text to obtain a ternary arm result ID.
- Reject a broad `emit_rval_*` or `coerce` migration, a second expression
  family, or a PHI-carrier/verifier change claimed as this selected-arm repair.
- Reject a raw-string result plus a map, side table, synthetic value, or
  test-only branch presented as native authority.
- Reject positive-only coverage, malformed-authority weakening, expectation
  downgrades, or baseline edits in place of native fail-closed proof.

## Completion Record

Capability complete. Commit `22a4d555d` publishes native, valid,
current-function-owned `LirCastOp.result` authority for the selected ternary
`else` arm's i64-to-i32 coercion before compatibility spelling, through the
bounded `emit_rval_operand` to `coerce_operand` path. Focused structural
coverage proves that result and rejection of missing, invalid, duplicate, and
foreign authority through the existing verifier. The accepted command
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1, and the
matching baseline/after regression guard passed with
`--allow-non-decreasing-passed`.

The accepted handoff is recorded in open 775. This source deliberately leaves
the other ternary arm, raw ternary PHI result and incoming carriers, and raw
later final-consumer input unresolved; those exclusions are not criteria of
this selected-arm source and make no 751 or Raw-BIR claim.
