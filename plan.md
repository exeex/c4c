# LIR Scalar Binary-LHS Parameter Authority Runbook

Status: Active
Source Idea: ideas/open/819_lir_scalar_binary_lhs_parameter_authority.md
Supersedes: 818 Step 1 trace-only route pending its scalar producer prerequisite

## Purpose

Establish the one native producer authority tuple needed before 818 can decide
whether a scalar binary-LHS parameter use is receiver-ready.

## Core Rule

Authority must originate in native current-function facts. `param_slots`,
rendered names/signatures, `LirOperand::raw`, diagnostics, and display-string
matching are never authority.

## Read First

- `ideas/open/819_lir_scalar_binary_lhs_parameter_authority.md`
- `ideas/open/818_lir_next_body_parameter_authority_handoff.md` (return record)
- `ideas/closed/817_lir_body_parameter_receiver_authority_handoff.md`
- `ideas/open/795_lir_body_parameter_authority_handoff.md`

## Scope

Only a scalar current-function parameter used as `LirBinOp.lhs`: native value,
position, scalar type, owner, ABI classification, verifier admission, focused
producer proof, and a handoff to 818.

## Non-Goals

- Raw-BIR/importer/dispatcher/receiver changes or tests;
- API-wide parameter work, presentation recovery, or ABI-wide conversion;
- `LirBinOp.rhs`, other operand forms, closed 795/817 routes, or byval-memcpy.

## Execution Rules

- Keep the selected form fail-closed until all tuple fields are native and
  structurally verifiable.
- Do not generalize across operand roles or parameter forms.
- Build before focused same-feature proof; do not claim receiver completion.

## Ordered Steps

### Step 1 - Discover the native scalar binary-LHS authority producer

Goal: locate the minimum producer/schema seam for the selected `LirBinOp.lhs`
parameter form and map each required tuple field to a native source.

Actions:

- trace current-function value, parameter position, scalar type, owner, and ABI
  classification through production and schema seams;
- document any missing native fact and reject presentation-only substitutes;
- name the exact verifier tuple and malformed cases before editing producers.

Completion check: one exact native-field map and fail-closed reject boundary
are recorded; no alternate form or receiver work is selected.

### Step 2 - Publish and verify the one-form authority tuple

Goal: add the minimum producer/schema/verifier support for the selected tuple.

Actions:

- publish native value/position/type/owner/ABI plus `lhs` role only for the
  selected scalar current-function parameter form;
- reject missing, invalid, duplicate, foreign, role/position/type/ABI-incoherent,
  non-scalar, and display-derived forms transactionally;
- add nearby focused positive and malformed-authority producer coverage.

Completion check: selected-form authority verifies structurally and every
nonselected or malformed form remains fail-closed.

### Step 3 - Prove and return the exact producer handoff to 818

Goal: accept proof and preserve the bounded return contract.

Actions:

- run a fresh build and focused same-feature producer proof;
- provide the supervisor-selected matching regression guard when required;
- record the exact tuple, allowed ABI class, rejects, proof, and 818 Step 1
  return point in the source idea.

Completion check: one accepted producer handoff exists for 818 re-evaluation;
no Raw-BIR receipt, receiver claim, or wider parameter capability is made.
