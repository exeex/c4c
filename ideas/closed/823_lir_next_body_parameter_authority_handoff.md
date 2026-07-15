# LIR Next Body-Parameter Authority Handoff

Status: Open (active successor to 734 post-Step 7.35)
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`, Step 7.35
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Trace, publish, verify, and hand off exactly one next valid function-body
parameter-use authority row that is distinct from the accepted direct-pointer
GEP-base and DirectScalar binary-LHS rows.

## Why This Exists

Commit `18443fc0f` completed only the 818-authorized DirectScalar
`LirBinOp.lhs` Raw-BIR receiver row. The parent source still requires a
separately evidenced typed disposition for remaining parameter-use forms. It
cannot select another row from text or extend the receiver route without a
checked producer/verifier handoff.

## In Scope

- Trace native current-function identity, owner, parameter index, type, ABI,
  role, and consumer-operation facts for one candidate body-parameter use.
- Select one receiver-ready row only if those facts are structured and can be
  verified fail-closed.
- Publish the minimum producer/schema/verifier contract and nearby positive,
  malformed, foreign-owner, type/ABI-incoherent, and role/value mismatch
  coverage needed for that one row.
- Record a precise one-row handoff, focused proof, rejected forms, and return
  action for 734.

## Out of Scope

- Raw-BIR destinations, importer, builders, receiver tests, generic scalar or
  parameter admission, or receipt of the selected row.
- Reopening the accepted direct-pointer `LirGepOp.ptr` or DirectScalar
  `LirBinOp.lhs` rows.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and switch-selector work.
- Text/name/diagnostic/operand reconstruction, testcase-specific exceptions,
  or weakening existing verifier contracts.

## Acceptance Criteria

1. Exactly one distinct body-parameter use has a structured current-function
   authority tuple and an explicit fail-closed verifier contract.
2. Nearby positive and malformed-authority coverage proves missing, invalid,
   duplicate, foreign-owner, type/ABI-incoherent, wrong-role, and consumer
   value/type-mismatch forms reject where applicable.
3. A fresh build and focused same-feature producer proof pass.
4. The completion record gives 734 an exact one-row receiver contract; every
   nonselected form remains fail-closed and unreceived.

## Reviewer Reject Signals

- Reject Raw-BIR/importer/builder or receiver changes presented as authority
  publication progress.
- Reject generic scalar/parameter admission, a combined parameter sweep, or
  reopening either accepted predecessor row.
- Reject deriving identity, type, ABI, owner, or role from rendered text,
  operand spelling, diagnostics, or named-test conditions.
- Reject expectation downgrades, verifier weakening, or malformed authority
  becoming accepted without explicit source authority.
- Reject claiming a handoff for a form whose native structured facts or
  same-feature positive/negative proof are absent.

## Initial Return Contract

The parent is paused after accepted Steps 1 through 7.35, most recently
`18443fc0f`. This successor must not select a receiver row merely because it
is adjacent. Once it has an accepted exact handoff, reactivate 734 and repair
its runbook for only that row's typed Raw-BIR receipt; otherwise record the
evidence-backed split or no-change outcome without claiming parameter-family
completion.

## Completion Record: DirectScalar binary-RHS authority handoff

Close accepted: this bounded producer/schema/verifier source is capability-complete. Commit `83d7959f2` publishes exactly one distinct native body-parameter authority for `LirBinOp.scalar_rhs_parameter_authority`. The sole receiver-consumable tuple is the RHS SSA `LirValueId`, parameter index, `LirTypeRef`, current-function `LinkNameId` owner, native `LirNativeBodyParameterAbi::DirectScalar`, explicit `Rhs` role, and matching `LirBinOp.rhs` operand/value and operation type. The producer binds it only from the native current-function parameter definition when value, type, and ABI agree; the verifier requires that same RHS role/value/type contract.

Nearby coverage rejects absent carrier, invalid value, duplicate definition, foreign owner, wrong LHS role, type mismatch, ABI mismatch, and RHS operand/value mismatch. All nonselected forms remain fail closed and unreceived. No Raw-BIR destination, importer, builder, receiver, or generic parameter admission changed in this source.

Accepted proof is the fresh `cmake --build --preset default --target backend_lir_selected_pointer_authority_test` build and focused `ctest --test-dir build --output-on-failure -R '^backend_lir_selected_pointer_authority$'` pass, with matching `test_before.log`/`test_after.log` 1/1 passing and the non-decreasing guard accepted using `--allow-non-decreasing-passed`.

Exact return action: reactivate 734 at one new bounded receiver step only. Receive this `LirBinOp.rhs` DirectScalar tuple into typed Raw-BIR, importer dispatch, reachable verification, and transactional positive/malformed coverage. Do not repeat Step 7.35, receive the previously accepted pointer or binary-LHS rows, start any other parameter form, or broaden into other source families.
