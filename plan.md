# HIR Function-Signature Direct Aggregate-Ref Carrier Runbook

Status: Active
Source Idea: ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md
Supersedes: 848 Step 2b pending upstream semantic-construction carrier

## Purpose

Provide the direct, definition-backed HIR aggregate carrier that 848 needs to
populate function-signature occurrence refs without identity recovery.

## Goal

Deliver a narrow return/parameter semantic-construction carrier from an
already registered HIR aggregate definition to the HIR lowering call sites.

## Core Rule

Canonical aggregate authority must arrive directly from HIR definition-backed
semantic construction. Never recover it from parser, `TypeSpec`, owner, tag,
text, or LIR state.

## Read First

- `ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md`
- `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`
- `src/frontend/hir/hir_functions.cpp` and its function-signature semantic
  construction inputs
- HIR aggregate definition registration and `HirStructDef::aggregate_ref`

## Scope

- Direct HIR semantic/construction carrier/API for aggregate function returns
  and parameters only.
- Carrier validity and focused positive/malformed proof.

## Non-Goals

- Do not use parser/`TypeSpec`/owner/tag/text identity recovery or a `Node*`
  map.
- Do not change `qtype_from`, occurrence population, or LIR.
- Do not expand to other nominal families or signature positions.

## Execution Rules

- Start from an existing module-owned HIR aggregate definition/ref; validate
  direct facts only and fail closed when they are absent or invalid.
- Keep the carrier separate from 848's `qtype_from` attachment work.
- Build and run focused HIR proof for each code-bearing packet.

## Ordered Steps

### Step 1 - Locate the signature semantic carrier seam

Goal: identify the smallest semantic/construction object or API that can hold
an already definition-backed aggregate ref/definition until return and parameter
HIR lowering.

Actions:

- Trace only the return and parameter construction routes from registered HIR
  aggregate definitions to their `hir_functions.cpp` lowering call sites.
- Confirm that current normalized `TypeSpec` inputs cannot legally serve as
  canonical authority and name the new direct carrier boundary.
- Specify missing, incomplete, invalid, and foreign/module-mismatch behavior
  without a fallback lookup.

Completion check: the proposed carrier has a direct HIR definition-backed
source, reaches both signature positions, and requires no forbidden recovery.

### Step 2 - Add the bounded direct carrier/API

Goal: retain and deliver the already definition-backed aggregate fact through
function-signature semantic/construction.

Actions:

- Implement the smallest direct carrier/API needed by return and parameter
  construction.
- Preserve module ownership and reject absent, incomplete, invalid, and
  foreign facts at the carrier boundary.
- Do not edit `qtype_from`, attach occurrence refs, or modify LIR.

Completion check: both lowering call sites can receive a direct valid carrier,
while invalid direct carrier states fail closed without metadata recovery.

### Step 3 - Prove carrier delivery and return to 848

Goal: establish focused evidence and the precise handoff.

Actions:

- Add nearby coverage for direct carrier delivery through return and parameter
  routes plus malformed-boundary behavior.
- Run a fresh build and focused HIR proof; run a proportional checkpoint if
  the changed semantic API has broader callers.
- Record accepted proof and return 848 at unchanged Step 2b only.

Completion check: accepted evidence proves the bounded carrier; no occurrence
attachment or LIR work is claimed.
