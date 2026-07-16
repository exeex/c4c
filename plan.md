# HIR Function-Signature Aggregate-Ref Producer Order Runbook

Status: Active
Source Idea: ideas/open/850_hir_signature_aggregate_ref_producer_order.md
Supersedes: 849 Step 2 pending an upstream production direct fact

## Purpose

Produce and order a direct module-issued aggregate fact before function
signature lowering, so 849 can later deliver it through its carrier boundary.

## Core Rule

Production semantic construction must supply an existing definition-backed HIR
fact directly. Never reconstruct canonical identity from parser, normalized
type, owner, tag, text, `Node*`, or LIR state.

## Read First

- `ideas/open/850_hir_signature_aggregate_ref_producer_order.md`
- `ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md`
- `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`
- Function-signature construction callers that precede `Lowerer::lower_function`
- HIR aggregate definition registration and `HirStructDef::aggregate_ref`

## Non-Goals

- Do not implement 849's `lower_function` carrier/API delivery.
- Do not edit `qtype_from`, attach occurrence refs, or alter LIR.
- Do not recover canonical identity from any parser or normalized-type metadata.

## Ordered Steps

### Step 1 - Establish the production signature-fact seam and order

Goal: identify and implement the earliest bounded HIR semantic/construction
seam that can issue or expose a valid module-owned direct aggregate fact before
each production function-signature construction call.

Actions:

- Trace every production caller that currently reaches `lower_function` with
  the default null carrier and locate its prior aggregate-definition semantic
  event.
- Add the minimal producer/order contract for aggregate return and explicit
  parameter facts; keep absent, incomplete, invalid, and foreign facts empty
  rather than recovering them.
- Do not change the 849 carrier boundary, `qtype_from`, or LIR.

Completion check: production construction—not tests—can expose a direct,
module-owned, definition-backed fact before `lower_function` for both required
signature positions, or rejects malformed input fail closed.

### Step 2 - Prove production availability and malformed boundaries

Goal: demonstrate that the new semantic/order route provides the direct fact
without forbidden recovery.

Actions:

- Add nearby coverage for aggregate return and parameter production paths and
  missing/invalid/foreign boundary behavior.
- Run a fresh build and focused HIR test proof; widen proportionally if the
  semantic construction API has shared callers.

Completion check: accepted evidence proves a production direct fact reaches
the pre-`lower_function` seam; no test-only injection or discarded carrier is
claimed.

### Step 3 - Return the bounded fact to 849

Goal: make the exact handoff durable without absorbing downstream delivery.

Actions:

- Record the accepted production fact, boundary checks, proof, and caller
  ordering in the source completion record.
- Return to 849 Step 2 to wire this fact into its already identified carrier/API
  boundary; keep 848 parked.

Completion check: 849 can resume at Step 2 without repeating discovery, while
848 remains unchanged until 849 is accepted.
