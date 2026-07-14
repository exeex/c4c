# LIR Switch Selector Value Identity Publication

Status: Open (active blocker for idea 734's next bounded CFG receiver)
Type: bounded LIR switch-selector producer authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish verifier-checked current-function `LirValueId` authority for each
active `LirSwitch` selector so one later Raw-BIR switch receiver packet can
consume it together with the already typed default and ordered case successors.

## Why This Exists

Closed idea 750 established `LirSwitch.default_successor` and ordered
`case_successors`, but `selector_name` and `selector_type` remain presentation
carriers. Idea 734 cannot reconstruct selector identity or type from those
spellings, so its next valid CFG receiver row is blocked at the LIR producer
boundary.

## In Scope

- add the smallest typed selector-value carrier to active `LirSwitch`
- populate it from existing current-function value authority without parsing
  selector text
- verify presence, validity, current-function ownership, and integer-selector
  suitability before downstream use
- retain `selector_name` and `selector_type` only as checked display mirrors
- add focused positive and malformed-authority coverage plus an exact handoff
  to idea 734

## Out Of Scope

- Raw-BIR `SwitchTerm`, importer, container, builder, verifier, or receiver
  work; that remains idea 734
- switch successor changes already completed by idea 750
- `LirCondBr`, `LirIndirectBr`/`LirIndirectBrOp`, PHI, local/object, memory/va,
  aggregate/vector, parameter, or other identity work
- label/name/printer-text recovery, canonicalization, target lowering, MIR, or
  emission

## Acceptance Criteria

- Every active `LirSwitch` selector controlling CFG flow has valid,
  current-function typed value identity independent of `selector_name` and
  `selector_type`.
- Missing, invalid, foreign, or non-integer selector authority rejects before
  printing or downstream consumption, with no text fallback.
- Focused positive and negative tests prove misleading selector display text
  cannot select or repair the semantic selector.
- The handoff names the typed selector field, existing typed default and case
  successor fields, fail-closed boundaries, and proof required for one 734
  receiver packet.

## Reviewer Reject Signals

- Reject parsing `selector_name`, `selector_type`, rendered LLVM, printer
  output, labels, or testcase names to derive selector identity or type.
- Reject a named-case-only fix, expectation downgrade, or display agreement
  claimed as semantic progress.
- Reject changes to Raw-BIR receiver code, switch-successor authority,
  conditional/computed-goto authority, or unrelated LIR families under this
  bounded producer source.
- Reject a carrier that leaves missing, foreign, or non-integer selector
  failure reachable behind a renamed field or permits downstream fallback.
