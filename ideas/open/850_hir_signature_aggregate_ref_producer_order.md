# HIR Function-Signature Aggregate-Ref Producer Order

Status: Open
Type: bounded upstream HIR semantic producer/order blocker
Blocked Route: `ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md`, Step 2

## Goal

Create the smallest production HIR semantic/order path that makes an already
module-issued, definition-backed `HirAggregateRef` available at
function-signature construction before `Lowerer::lower_function` is called.

## Why This Exists

849 Step 1 was accepted at `109ea13f4`, but its Step 2 delivery API cannot
produce capability by itself: all production `lower_function` callers supply
the default null carrier. A carrier that only validates then holds/discards an
input does not deliver a fact to return or parameter lowering. The missing
first fact is therefore upstream semantic production and ordering, outside
849's API-delivery scope.

848 remains parked. Its Step 2a implementation/proof is accepted at
`359a9b94b` with `frontend_hir_tests`; no 849 implementation or proof is
accepted.

## In Scope

- Trace the production HIR semantic/construction order for function
  signatures before `lower_function`.
- Introduce the narrowest direct producer/order contract that can supply an
  existing module-issued `HirAggregateRef` (or its authoritative HIR
  definition) for aggregate return and explicit parameter signature facts.
- Require the produced fact to be complete, valid, and owned by the receiving
  module; absent, invalid, incomplete, or foreign facts fail closed.
- Add focused same-feature coverage proving the production path supplies the
  direct fact before `lower_function`, including malformed-boundary behavior.
- Return the accepted direct production fact to 849 Step 2 only.

## Out Of Scope

- The 849 carrier/API delivery implementation itself, including changing
  `lower_function` to consume a new carrier.
- Parser, `TypeSpec`, owner, tag, text, `record_def`, parser pointer, `Node*`
  map, reconstructed lookup, or any normalized-type metadata as canonical
  identity authority.
- `qtype_from` attachment or any 848 Step 2b occurrence-population edit.
- LIR, aggregate-store consumers, verifier/printer work, or unrelated nominal
  families.

## Acceptance Criteria

- A production semantic/order route supplies an already definition-backed,
  module-owned direct aggregate fact at function-signature construction before
  `lower_function`.
- The route handles aggregate returns and explicit parameters without identity
  recovery from forbidden metadata and fails closed for malformed facts.
- Focused positive and malformed-boundary coverage, plus a fresh build,
  demonstrate production availability rather than a test-only or discarded
  carrier.
- The completion record returns precisely to 849 Step 2: wire the accepted
  production fact into the identified carrier/API boundary. 848 stays parked
  until 849 finishes, then resumes at its unchanged Step 2b.

## Reviewer Reject Signals

- Reject a default-null production call, test-only injection, or a carrier
  that validates then discards its input as producer/order progress.
- Reject parser/`TypeSpec`/`record_def`, owner, tag, text, parser-pointer,
  `Node*` map, or reconstructed lookup presented as a direct semantic fact.
- Reject `qtype_from` attachment, occurrence population, LIR work, or 849
  carrier delivery claimed under this upstream producer/order blocker.
- Reject named-case-only tests, weaker malformed contracts, broad rewrites, or
  an abstraction rename that preserves the absence of a production direct fact.
