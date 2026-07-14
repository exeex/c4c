# LIR Switch Selector Value Identity Publication Runbook

Status: Active
Source Idea: ideas/open/756_lir_switch_selector_value_identity_publication.md
Activated from: paused idea 734 after accepted Step 7.22 typed `LirCondBr`
receipt (`220a3b5ad`); fresh matching backend guard 5/5 is accepted, while the
later 2961/3034 full postcheck is not a clean-baseline regression result.

## Purpose

Remove the remaining display-text selector boundary that prevents idea 734
from receiving one typed switch CFG row.

## Goal

Publish and verify a current-function typed selector identity for every active
`LirSwitch`, while keeping selector display fields non-authoritative.

## Core Rule

The typed selector carrier is semantic authority. `selector_name`,
`selector_type`, labels, printer output, and rendered text must neither select
nor repair it.

## Read First

- `ideas/open/756_lir_switch_selector_value_identity_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- active `LirSwitch` producers and LIR verifier ownership checks

## Non-Goals

- no Raw-BIR container/importer/receiver work; one later 734 packet owns it
- no successor-carrier rewrite, conditional or computed-goto authority, PHI,
  local/object, memory/va, aggregate/vector, or parameter authority work
- no presentation-text recovery, target lowering, MIR, or emission

## Execution Rules

1. Add only the smallest typed `LirSwitch` selector carrier consistent with
   existing current-function value authority.
2. Populate it without parsing selector display fields and retain those fields
   only as checked display mirrors.
3. Fail closed for missing, invalid, foreign, or non-integer selector
   authority before printing or downstream use.
4. Add nearby positive and malformed-authority coverage. Require a fresh build
   and focused proof; the supervisor selects broader acceptance separately.

## Ordered Steps

### Step 1 - Publish switch-selector authority

Goal: give active `LirSwitch` a verifier-checked current-function typed
selector identity suitable for the later bounded Raw-BIR receiver.

Actions:

- trace the active producers and introduce the minimal typed selector field
- populate it from existing value identity, never from selector display text
- verify presence, validity, owner, and integer suitability; preserve existing
  typed default/case successor checks
- add focused valid, misleading-display, missing, invalid, foreign, and
  non-integer coverage without Raw-BIR receiver changes
- record the exact typed-field handoff and fail-closed boundary for 734

Completion check:

- a fresh build and focused proof show selector display text cannot establish
  CFG semantics and malformed typed selector authority rejects before
  downstream consumption.
