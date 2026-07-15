# LIR Switch Selector Type-Reference Verifier Repair

Status: Open
Type: bounded verifier-correctness blocker
Blocks: `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` Step 2

## Goal

Repair the deterministic `LirSwitch.selector_type_ref` verifier defect so a
switch's structured selector type reference is checked against the
selector-selected integer value definition correctly.

## Why This Exists

While evaluating the unaccepted in-scope 801 Step 2 repair, the former
direct-complex call-signature mismatch passes and exposes:
`LirSwitch.selector_type_ref: must match the selector-selected integer value
definition`. The defect traces to `a6c013ed0` and is independent of anonymous
aggregate field layout or structured call signatures.

## In Scope

- Trace the `LirSwitch` selector value, selected integer definition, and
  `selector_type_ref` verifier ownership/type-comparison seam.
- Implement the smallest verifier/model correction that enforces the intended
  selector type-reference contract for valid and malformed switch forms.
- Add nearby positive and malformed verifier coverage, then prove the bounded
  route with a fresh build and focused relevant tests.

## Out Of Scope

- Anonymous aggregate layouts, `LirCallOp` signature repair, or acceptance of
  any uncommitted 801 Step 2 work.
- Raw-BIR receiver work, switch successor/case lowering, CFG redesign,
  generic type-system rewrites, MIR, emission, or unrelated verifier cleanup.

## Acceptance Criteria

- Valid structured switch selectors whose type reference matches the selected
  integer value definition verify.
- Missing, foreign, stale, or type-incoherent selector type references reject
  under the intended verifier contract.
- Fresh build and nearby positive/malformed switch verifier proof support
  returning to 801 Step 2; broader proof is selected by the supervisor if the
  shared verifier blast radius requires it.

## Reviewer Reject Signals

- Reject weakening or bypassing `LirSwitch.selector_type_ref` checks merely to
  make the exposed case pass.
- Reject testcase-name, rendered-text, instruction-order, or named-case-only
  shortcuts instead of checking structured selector/type ownership facts.
- Reject absorbing anonymous-layout/call work, switch lowering/successor work,
  or broad verifier rewrites into this blocker.
- Reject expectation downgrades, helper-only renames, or classification-only
  changes claimed as verifier-contract repair.
- Reject retaining the same selector/type mismatch behind a renamed helper or
  alternate representation.

## Resumption Record: Step 1 801 argument-mirror prerequisite

No 802 implementation is accepted. Its isolated Step 1 switch verifier/test
working-tree hunk remains preserved and unaccepted, including the exact
focused proof command:
`ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`.

The command currently fails before the switch selector check in the preserved
801 Step 2 repair at `LirCallOp.arg_type_refs`: `argument 0 mirror does not
match call text; shadow 'i32', call argument type 'rendered arguments are
not'`. That native argument-mirror verifier defect is in scope for 801 Step 2,
not this selector blocker. Resume 802 unchanged at Step 1 only after 801
repairs that defect without weakening contracts and proves the focused test
reaches the switch check. Do not claim this parked hunk/tests accepted or use
them as evidence for returning control to 801.
