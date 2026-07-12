# Block-Entry Publication Identity Completion

Status: Open
Type: common semantic publication identity repair
Parent: `ideas/closed/703_bir_mir_contract_abstraction_umbrella.md`
Unblocks: `ideas/open/716_prepared_call_plan_cursor_complete_production.md`

## Goal

Restore exact agreement between available prepared block-entry publication
facts and the BIR semantic publication identity view, keyed by successor and
destination identity and remaining fail closed for incomplete evidence.

## Why This Exists

`backend_prepare_frame_stack_call_contract` constructs prepared publication
data whose readiness query is available, then fails when the BIR identity view
does not reproduce the expected semantic destination and instruction identity.
This later failure predates and is disjoint from prepared-call production.

## In Scope

- Trace prepared current-block entry publication readiness into
  `find_bir_block_entry_publication_identity`.
- Preserve successor, destination value ID/name/type, PHI instruction index,
  and proof attribution across the prepared-to-BIR semantic boundary.
- Reject missing, wrong-successor, wrong-destination, wrong-type, stale,
  duplicate, or unattributed evidence without synthesizing identity.
- Add producer/query proof across multiple publication shapes, not only the
  call-contract fixture.

## Out Of Scope

- Prepared-call plans, call argument materializability, or ABI policy.
- Join-source/edge-publication identity and target materialization.
- Storage hooks, register spelling, move order, or emitted publication policy.
- Route-number restoration or printer/debug vocabulary cleanup.

## Acceptance Criteria

- The earliest prepared-to-BIR identity divergence is documented before
  repair.
- Available attributed publication facts map to exact BIR successor,
  destination, type, and instruction identity.
- Missing or inconsistent evidence remains unavailable with precise typed
  status; no source-order or nearest-PHI recovery is introduced.
- Focused publication and frame/stack contract tests plus a
  supervisor-selected broader backend comparison are green without expectation
  changes.

## Reviewer Reject Signals

- A `%join.arg`, value-71, instruction-zero, or named-fixture special case.
- Selecting a PHI/publication by source order, nearest instruction, display
  name alone, or target-emission evidence.
- Weakening unavailable cases, changing expected results, or marking the
  fixture unsupported.
- Helper renames, proof-bit forcing, or status-only changes claimed as semantic
  identity repair.
- Broad changes to call preparation, join-source handling, target
  materialization, storage policy, or publication emission.
