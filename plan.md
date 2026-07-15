# LIR-to-New-BIR Switch-Selector Parameter Receipt Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: post-Step 7.37 source-gate reassessment after closed Idea 825

## Purpose

Receive exactly closed 825's native DirectScalar body-parameter authority for
a direct `LirSwitch.selector` into verified Raw BIR.

## Core Rule

Consume only `LirSwitch.selector_parameter_authority` and its checked selector
and selector-type relations. Do not recover identity from display text, reuse
binary-LHS authority, or materialize an `add`.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md` (the closed-825
  resumption record)
- `ideas/closed/825_lir_next_body_parameter_authority_handoff.md`
- Existing Raw-BIR switch container, LIR importer, and reachable verifier
  surfaces for the already accepted `LirSwitch` receiver route

## Non-Goals

- Any second parameter-use row, generic parameter admission, ABI conversion,
  or producer/schema/verifier change.
- Repeating accepted Steps 1 through 7.37, including the DirectPointer and
  DirectScalar binary-LHS/RHS/ReturnValue receipts.
- Memory/VA, aggregate/vector, module/type/global, residual
  instruction/terminator, inline-assembly, or presentation-derived work.

## Ordered Steps

### Step 7.38 - Receive the 825-authorized DirectScalar switch-selector parameter authority row

Goal: add one typed Raw-BIR switch-selector parameter destination and import
only the authority tuple that closed 825 proved for a direct switch selector.

Actions:

- map only `LirSwitch.selector_parameter_authority` into the existing typed
  Raw-BIR switch receiver path, retaining the parameter value, owner, index,
  type, DirectScalar ABI, and `SwitchSelector` role;
- require the exact native relations `selector == authority.value` and
  `selector_type_ref == authority.type`, current-function ownership, and all
  existing typed switch target/order constraints before publication;
- extend the reachable importer/verifier path and nearby positive plus
  malformed-authority coverage so missing, invalid, duplicate, foreign,
  owner/index/type/ABI/role-mismatched, or consumer-incoherent input rolls back
  transactionally;
- keep every nonselected parameter and semantic family fail closed.

Completion check: a fresh build, focused same-feature receiver proof, and
matching regression guard show only this one 825-authorized row is received;
then return to the source completion gate without claiming whole-source
completion.
