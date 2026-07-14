# LIR Conditional Branch Condition Identity Publication

Status: Closed (capability complete; Raw-BIR conditional receipt remains idea 734)
Type: bounded LIR conditional-branch producer authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish verifier-checked current-function `LirValueId` authority for the
active `LirCondBr` condition so one Raw-BIR conditional-branch receiver packet
can consume it with the already typed successor IDs.

## Why This Exists

Closed idea 750 made `LirCondBr.true_successor` and `false_successor` typed,
but `cond_name` remains display text. Idea 734 cannot reconstruct a condition
value from that spelling, so its next CFG receiver row is blocked at the LIR
producer boundary.

## In Scope

- add the smallest typed condition-value carrier to active `LirCondBr`
- populate it from the existing current-function value authority without
  parsing condition text
- verify presence, validity, current-function ownership, and boolean
  suitability before downstream use
- retain `cond_name` only as a checked display mirror
- add focused positive and malformed-authority coverage plus an exact handoff
  to idea 734

## Out Of Scope

- Raw-BIR containers/importer/receiver work; that remains idea 734
- conditional successor changes already completed by idea 750
- `LirSwitch` selector authority, `LirIndirectBrOp` address authority, PHI,
  local/object, memory/va, aggregate/vector, parameter, or other identity work
- label/name/printer-text recovery, canonicalization, target lowering, MIR, or
  emission

## Acceptance Criteria

- Every active `LirCondBr` condition controlling CFG flow has a valid,
  current-function typed value identity independent of `cond_name`.
- Missing, invalid, foreign, or non-boolean condition authority rejects before
  printing or downstream consumption, with no text fallback.
- Focused positive and negative tests prove misleading condition display text
  cannot select or repair the semantic condition.
- The handoff names the typed condition field, the existing typed successor
  fields, fail-closed boundaries, and proof required for one 734 receiver
  packet.

## Reviewer Reject Signals

- Reject parsing `cond_name`, rendered LLVM, printer output, or testcase names
  to derive the condition identity.
- Reject a named-case-only fix, expectation downgrade, or display-label
  agreement claimed as semantic progress.
- Reject changes to Raw-BIR receiver code, switch/computed-goto authority, or
  unrelated LIR families under this bounded producer source.
- Reject a carrier that leaves the same missing/foreign/non-boolean condition
  failure reachable behind a renamed field or permits downstream fallback.

## Closure Record

Disposition: capability complete.

Accepted implementation `ae006a0f1` publishes `LirCondBr.condition` as the
current-function `LirValueId` for the active boolean condition. Direct
producers thread the typed boolean operand, and verification rejects missing,
invalid, foreign, or non-boolean authority while checking `cond_name` only as
a display mirror. No Raw-BIR container, importer, or receiver was changed.

Supervisor acceptance proof: fresh `cmake --build --preset default`; direct
`build/tests/frontend/frontend_lir_call_type_ref_test`; matching backend guard
`ctest --test-dir build -j --output-on-failure -R '^backend_'` 5/5 before and
after (non-regressive); and broader
`ctest --test-dir build -j --output-on-failure -R '^(frontend_cxx_|positive_sema_)'`
35/35.

Handoff: open idea
`ideas/open/734_lir_to_new_bir_container_completeness.md` owns the next,
separately bounded Raw-BIR conditional-branch receiver packet. It must consume
`LirCondBr.condition` together with `LirCondBr.true_successor` and
`LirCondBr.false_successor`, preserve the fail-closed verifier boundary, and
never recover semantics from `cond_name` or other presentation text.
