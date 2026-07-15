# LIR Instruction, Terminator, and Inline-Assembly Residual Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/closed/793_lir_to_new_bir_remaining_coverage_umbrella.md`
Related: `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Establish one bounded residual instruction/terminator/inline-assembly
value-or-type authority handoff without reopening accepted CFG/PHI work or
parsing opaque assembly text.

## In Scope

- Select exactly one residual family only when its native value, edge, and
  type facts can be published and verified; retain inline-asm templates and
  constraints as opaque text.
- Record malformed-authority rejection, focused proof, and a one-row future
  receiver handoff where the selected family permits it.

## Out Of Scope

- Raw-BIR receipt, a combined residual sweep, prior CFG/PHI authority,
  type-mirror work owned by 761, or template/constraint parsing.

## Acceptance Criteria

- The selected family is explicit, bounded, natively structured, and covered
  by focused positive/negative producer proof; every nonselected residual stays
  fail closed.

## Reviewer Reject Signals

- Reject a catch-all instruction/terminator conversion, text parsing, receiver
  changes, testcase-shaped shortcuts, or weaker verifier/test contracts.
