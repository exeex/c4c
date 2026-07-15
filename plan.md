# LIR Next Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/823_lir_next_body_parameter_authority_handoff.md
Activated from: 734 post-Step 7.35 separate-blocker decision

## Purpose

Produce one exact, checked future body-parameter authority handoff. This is a
producer/schema/verifier route; it does not receive Raw-BIR data and does not
complete the parent parameter family.

## Core Rule

Select no row until its current-function identity, owner, parameter index,
type, ABI, role, and consuming operation are native structured facts. Never
recover any of them from text, names, diagnostics, or rendered operands.

## Read First

- `ideas/open/823_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.35
  resumption record)
- `ideas/closed/818_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/820_lir_directscalar_parameter_producer_verifier_publication.md`

## Non-Goals

- Raw-BIR/importer/builder/receiver work, generic parameter admission, or a
  combined parameter sweep.
- Repeating the accepted direct-pointer or DirectScalar binary-LHS rows.
- Pending Ideas 821/822 selector work and every non-parameter family.

## Ordered Steps

### Step 1 - Trace and select one distinct native parameter-use row

Goal: identify one candidate only when the complete structured authority tuple
and its consumer operation can be demonstrated.

Actions:

- inspect the native LIR producer and verifier seams for one unreceived
  body-parameter use;
- record why the candidate is distinct from the accepted pointer and scalar
  rows, and fail closed if any required structured fact is missing;
- define the exact rejected malformed forms before editing code.

Completion check: one bounded row is selected with no presentation-derived
authority and no expansion into other parameter forms.

### Step 2 - Publish and verify the selected authority contract

Goal: implement the minimum native producer/schema/verifier publication for
the selected row.

Actions:

- preserve current-function ownership and the selected identity/type/ABI/role
  contract;
- add nearby positive and malformed/foreign/type- or ABI-incoherent coverage;
- keep all nonselected forms fail closed.

Completion check: the selected structured tuple verifies and malformed forms
reject without a generic parameter path.

### Step 3 - Prove and hand off to 734

Goal: produce accepted focused evidence and an exact receiver contract.

Actions:

- run a fresh build and the focused same-feature producer proof selected by
  the supervisor;
- record the exact fields, rejected forms, proof, and return action in the
  source idea;
- return control to 734 only after supervisor acceptance.

Completion check: 734 can be resumed for exactly one typed Raw-BIR receiver
row without rediscovering authority or widening scope.
