# HIR Function-Signature Definition-Provenance Architecture Blocker

Status: Open
Type: architecture and semantic-construction blocker
Blocked Parents:
- `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`, Step 2b
- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`, Step 2 return
Predecessor conclusion: `ideas/closed/849_hir_function_signature_direct_aggregate_ref_carrier.md`

## Goal

Decide, from production HIR construction evidence, whether a legal broader
semantic-construction architecture can make module-owned aggregate definition
provenance available before function-signature normalization, and specify the
smallest separately scoped implementation successor only if it can.

## Why This Exists

849 conclusively found that all production `lower_function` callers have only
`Node*`/name/template/NTTP inputs, while module-issued aggregate refs appear
only after HIR struct registration. The previously proposed direct carrier has
no production fact to carry. Retrying 848 Step 2b would therefore require
forbidden identity recovery or a test-only path. The remaining question is an
architecture decision outside both 849's forwarding scope and 848's occurrence
population scope.

## In Scope

- Trace the earliest legal HIR definition-construction and function-signature
  semantic phases that could own a direct aggregate definition fact before
  normalized `TypeSpec` lowering.
- Record the lifecycle decision with exact evidence: either a feasible direct
  provenance architecture and a narrow implementation successor, or a
  conclusive no-feasible-route result for the current supported signature
  contract.
- Preserve the exact return conditions for 848 Step 2b and 838 Step 2.

## Out Of Scope

- Implementing a carrier, changing `qtype_from`, attaching occurrence refs,
  changing LIR, altering parser/type metadata, or weakening tests/contracts.
- Recovering canonical identity via parser, `TypeSpec`, `record_def`, owner,
  tag, text, parser pointer, `Node*` map, or reconstructed lookup.
- Claiming aggregate function-signature support before a direct-fact producer
  is established and independently accepted.

## Acceptance Criteria

- The active decision records all production `lower_function` callers and the
  registration timing that invalidated 849's carrier premise.
- It identifies a concrete earlier legal direct-fact owner and a bounded
  implementation successor, or explicitly concludes no feasible architecture
  exists under the stated boundaries.
- 848's exact return is either an approved successor with a direct fact, or a
  deliberate no-change conclusion of its function-signature route; 838 is not
  falsely unblocked.
- No C++ code, tests, test expectations, or canonical regression logs change
  in this architecture decision initiative.

## Reviewer Reject Signals

- Reject a renamed carrier or test-only injection asserted as production
  provenance.
- Reject parser/normalized-type/owner/tag/text recovery or `Node*` maps as
  canonical identity authority.
- Reject `qtype_from`, occurrence population, or LIR changes claimed as
  architecture-decision progress.
- Reject leaving 848 parked with a generic blocker reference and no exact
  return-or-conclude decision.
