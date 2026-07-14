# LIR Conditional Branch Condition Identity Publication Runbook

Status: Active
Source Idea: ideas/open/755_lir_conditional_branch_condition_identity_publication.md
Activated from: paused idea 734 after accepted Step 7.21 legacy
`LirIndirectBr` receipt (`b528dc1`); matching backend guard 5/5 and prior full
checkpoint 3034/3034 passing

## Purpose

Remove the remaining display-text condition boundary that prevents idea 734
from receiving one typed conditional-branch row.

## Goal

Publish and verify a current-function typed condition identity for every
active `LirCondBr`, while keeping `cond_name` display-only.

## Core Rule

The condition carrier is semantic authority. `cond_name`, labels, printer
output, and rendered text must neither select nor repair it.

## Read First

- `ideas/open/755_lir_conditional_branch_condition_identity_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- active `LirCondBr` producers and LIR verifier ownership checks

## Non-Goals

- no Raw-BIR importer/container work; one later 734 packet owns that receipt
- no successor-carrier rewrite, switch selector, computed-goto address, PHI,
  local/object, memory/va, aggregate/vector, or parameter authority work
- no presentation-text recovery, target lowering, MIR, or emission

## Execution Rules

1. Add only the smallest typed `LirCondBr` condition carrier consistent with
   existing current-function value authority.
2. Populate it without parsing `cond_name` and retain the name as a checked
   display mirror only.
3. Fail closed for missing, invalid, foreign, or non-boolean condition
   authority before printing or downstream use.
4. Add nearby positive and malformed-authority coverage. Require a fresh build
   and focused proof; the supervisor selects broader acceptance separately.

## Ordered Steps

### Step 1 - Publish conditional-branch condition authority

Goal: give active `LirCondBr` a verifier-checked current-function typed
condition identity suitable for the later bounded Raw-BIR receiver.

Actions:

- trace the active producers and introduce the minimal typed condition field
- populate the field from existing value identity, never from `cond_name`
- verify presence, validity, owner, and boolean suitability; preserve the
  existing typed true/false successor checks
- add focused valid, misleading-display, missing, invalid, foreign, and
  non-boolean coverage without receiver changes
- record the exact typed-field handoff and fail-closed boundary for 734

Completion check:

- a fresh build and focused proof show that condition display text cannot
  establish CFG semantics and that malformed typed condition authority rejects
  before downstream consumption.
