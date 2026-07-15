# LIR Switch Selector Type-Reference Verifier Repair Runbook

Status: Active
Source Idea: ideas/open/802_lir_switch_selector_type_reference_verifier.md
Activated from: 801 Step 2 return-to-selector-gate switch; resume 801 only
after this blocker is accepted.

## Purpose

Restore the intended structured type-reference verification for a `LirSwitch`
selector, without absorbing the anonymous-layout/call repair that exposed it.

## Core Rule

Verify `selector_type_ref` against the selector-selected integer value
definition through native structured facts. Do not weaken the verifier or use
rendered compatibility text as type authority.

## Read First

- `ideas/open/802_lir_switch_selector_type_reference_verifier.md`
- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` resumption
  record
- historical introduction `a6c013ed0` and the nearby `LirSwitch` verifier,
  construction, and focused test seams

## Non-Goals

- 801 anonymous aggregate layout or direct-complex call-signature work.
- 754 aggregate-use authority changes/tests present in the working tree.
- Switch lowering, successors/cases, Raw-BIR, generic type-model changes, MIR,
  emission, and unrelated verifier cleanup.

## Ordered Steps

### Step 1 - Diagnose and repair switch selector type-reference verification

Goal: identify the exact selector value/type-reference ownership mismatch and
make the smallest correction that preserves the intended valid and malformed
switch contracts.

Actions:

- trace the selected integer value definition, its structured type fact, and
  `LirSwitch.selector_type_ref` through construction and verification;
- compare the `a6c013ed0` introduction with the current failure and repair the
  specific ownership/type comparison defect without weakening rejection;
- assess only the parked 802 verifier/test hunk; add nearby positive and
  malformed switch verifier coverage for matching, missing, foreign, stale,
  and incoherent selector type references as the actual contract supports;
- leave all 801 and 754 work untouched and do not reclassify either
  working-tree repair as accepted.

Completion check: a fresh `cmake --build --preset default` plus focused
positive/malformed switch verifier proof passes; invalid structured forms still
reject; supervisor selects and accepts any wider proof required by this shared
verifier seam.

### Step 2 - Publish the 801 return handoff

Goal: record the accepted bounded contract and return control to 801 without
claiming its interrupted Step 2 is complete.

Actions:

- record the accepted repair/proof and the exact selector type-reference facts
  801 may rely on only as an environmental prerequisite;
- reactivate 801 at unchanged Step 2, preserving its current unaccepted
  working-tree repair and its direct-complex call-signature/full-baseline gate.

Completion check: 801 can resume its exact Step 2 repair objective; no
anonymous-layout/call implementation has been accepted through this blocker.
