# HIR Function-Signature Direct Aggregate-Ref Carrier Runbook

Status: Active
Source Idea: ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md
Resumes: Step 2 after 850's intentionally concluded no-change route

## Purpose

Provide the direct, definition-backed HIR aggregate carrier that 848 needs to
populate function-signature occurrence refs without identity recovery.

## Core Rule

Canonical aggregate authority must arrive directly from HIR definition-backed
semantic construction. Never recover it from parser, `TypeSpec`, owner, tag,
text, `Node*`, or LIR state.

## Read First

- `ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md`
- `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`
- `ideas/closed/850_hir_signature_aggregate_ref_producer_order.md`
- `src/frontend/hir/hir_build.cpp` production callers and
  `src/frontend/hir/hir_functions.cpp` lowering boundary
- HIR aggregate definition registration and `HirStructDef::aggregate_ref`

## Non-Goals

- Do not edit `qtype_from`, attach occurrence refs, or alter LIR.
- Do not recover canonical identity from any parser or normalized-type metadata.

## Ordered Steps

### Step 1 - Locate the signature semantic carrier seam (accepted)

Accepted at `109ea13f4`. The carrier boundary is `Lowerer::lower_function`;
direct HIR definition/ref facts must travel in parallel with normalized types.

Actions:

- Do not repeat this discovery packet.

Completion check: preserved accepted seam and validation contract.

### Step 2 - Add the bounded direct carrier/API

Goal: retain and deliver an already definition-backed aggregate fact through
production function-signature construction.

Actions:

- Implement the smallest carrier/API at `lower_function` and its forwarding
  boundaries for return and explicit parameter lowering.
- Update its production `hir_build.cpp` callers to pass an existing
  module-issued fact only when their construction context has one; absent,
  incomplete, invalid, or foreign facts remain empty and fail closed.
- Do not derive a fact from parser/`TypeSpec`/owner/tag/text/record data or use
  a `Node*` map; do not edit `qtype_from` or LIR.

Completion check: both lowering sites receive a direct valid carrier through
production callers, while invalid states fail closed with no metadata recovery.

### Step 3 - Prove carrier delivery and return to 848

Goal: establish focused evidence and the precise parent handoff.

Actions:

- Add nearby coverage for return and parameter carrier delivery through the
  production path plus malformed-boundary behavior.
- Run a fresh build and focused HIR proof; widen proportionally if the semantic
  API has shared callers.
- Record accepted proof and return 848 at unchanged Step 2b only.

Completion check: accepted evidence proves the bounded carrier; no occurrence
attachment or LIR work is claimed.
