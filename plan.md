# LIR Computed-Goto Address Value Identity Publication Runbook

Status: Active
Source Idea: ideas/open/757_lir_computed_goto_address_value_identity_publication.md
Resumed from: paused Step 1 after separately scoped prerequisite 758 completed
at `c8a205218`. The preserved current-function local/parameter rvalue
`LirValueId` is now available to the existing rvalue/operand route; no 757
implementation step has been accepted.

## Purpose

Remove the remaining display-operand address boundary that prevents idea 734
from receiving one typed computed-goto row.

## Goal

Publish and verify a current-function typed address identity for every active
`LirIndirectBrOp`, while keeping its operand display carrier non-authoritative.

## Core Rule

The typed address carrier is semantic authority. `addr`, labels, printer
output, and rendered text must neither select nor repair it.

## Read First

- `ideas/open/757_lir_computed_goto_address_value_identity_publication.md`
- `ideas/closed/758_lir_rvalue_value_identity_preservation_for_computed_goto.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- active `LirIndirectBrOp` producers and LIR verifier ownership checks

## Non-Goals

- no Raw-BIR container/importer/receiver work; one later 734 packet owns it
- no successor-carrier rewrite, legacy indirect branch, conditional/switch
  authority, PHI, local/object, memory/va, aggregate/vector, or parameter work
- no presentation-text recovery, target lowering, MIR, or emission

## Execution Rules

1. Add only the smallest typed `LirIndirectBrOp` address carrier consistent
   with existing current-function pointer authority.
2. Populate it from the preserved rvalue identity without parsing `addr` and
   retain that operand only as a checked display mirror.
3. Fail closed for missing, invalid, foreign, or non-pointer address authority
   before printing or downstream use.
4. Add nearby positive and malformed-authority coverage. Require a fresh build
   and focused proof; the supervisor selects broader acceptance separately.

## Ordered Steps

### Step 1 - Publish computed-goto address authority

Goal: give active `LirIndirectBrOp` a verifier-checked current-function typed
address identity suitable for the later bounded Raw-BIR receiver.

Actions:

- trace active producers and introduce the minimal typed address field
- populate it from preserved pointer value identity, never from `addr` text
- verify presence, validity, owner, and pointer suitability; preserve existing
  ordered successor checks
- add focused valid, misleading-display, missing, invalid, foreign, and
  non-pointer coverage without Raw-BIR receiver changes
- record the exact typed-field handoff and fail-closed boundary for 734

Completion check:

- a fresh build and focused proof show address display text cannot establish
  computed-goto semantics and malformed typed address authority rejects before
  downstream consumption.
