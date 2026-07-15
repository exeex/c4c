# LIR Function-Body Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/795_lir_body_parameter_authority_handoff.md
Activated from: 810 Step 3 full-baseline blocker; return to 810 only after accepted parameter-index proof.

## Purpose

Publish the native function-body parameter authority needed by the selected
variable pointer-compound GEP index without treating declaration facts or
rendered parameter text as body-use identity.

## Core Rule

A variable GEP index sourced from a function-body parameter must carry checked
native integer or SSA authority owned by the current function. Do not recover
identity from spelling, weaken `verify_authoritative_gep`, or broaden to ABI
families not selected by evidence.

## Read First

- `ideas/open/795_lir_body_parameter_authority_handoff.md`
- `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md` resumption record
- current `pr21173.c` failure trace and the direct pointer-compound GEP lowering path
- nearby parameter publication/body-use authority and authoritative GEP verification

## Non-Goals

- Reopening 810's accepted postfix/native-immediate GEP producer repair.
- PHI producer authority, including `20060910-1.c`, owned by 806.
- Broad ABI/byval/HFA/vector/variadic conversion, Raw-BIR receipt, pointer or
  memory redesign, verifier weakening, or text-derived authority.

## Ordered Steps

### Step 1 - Trace the selected parameter-index body-use handoff

Goal: establish the exact native parameter identity, ownership, and ABI-form
classification at the variable RHS index entering the direct pointer-compound
GEP.

Actions:

- trace `pr21173.c` from the GEP index verifier diagnostic to the function
  body parameter publication and lowering handoff;
- determine whether the selected parameter is native integer authority or a
  bounded ABI-expanded form requiring explicit classification;
- split any unshared parameter or ABI family into a separate open successor.

Completion check: one body-parameter/index authority route is evidenced and
bounded; no declaration-only, text-derived, PHI, or generic ABI work is
selected.

### Step 2 - Publish the selected parameter-index authority

Goal: make the smallest producer/schema/lowering repair that supplies checked
current-function integer or SSA authority for the selected body-parameter GEP
index.

Actions:

- repair only the evidenced parameter body-use/index handoff;
- retain rejection of malformed, foreign, type-incoherent, and display-derived
  authority;
- add nearby same-family positive and malformed-authority coverage.

Completion check: focused coverage proves the selected variable parameter
index is authoritative and malformed forms remain fail-closed.

### Step 3 - Prove the handoff and return control to 810

Goal: give the supervisor accepted focused proof sufficient to resume 810's
full-baseline gate.

Actions:

- run a fresh build and selected same-feature proof;
- have the supervisor assess the evidence and reactivate 810 unchanged at
  Step 3; do not call a focused result a 3037/3037 baseline clearance.

Completion check: supervisor acceptance permits 810 to resume exactly at Step
3, where it must obtain a fresh comparable full baseline before returning 801.
