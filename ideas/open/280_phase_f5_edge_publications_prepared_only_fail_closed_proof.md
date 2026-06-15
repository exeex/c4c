# 280 Phase F5 edge_publications prepared-only fail-closed proof

## Goal

Prove one bounded `PreparedFunctionLookups::edge_publications` prepared-only
publication row fails closed instead of being accepted as Route 5 semantic
edge-publication authority.

## Why This Exists

Idea 266 still blocks edge-publication movement on duplicate Route 5 records,
prepared-only rows, Route 5-only rows, and wrong-edge rows. A first bounded
prepared-only proof row is needed before any demotion, deletion, accessor
wrapping, adapter migration, or broad prepared retirement can be considered.

## In Scope

- Choose one natural edge-publication consumer with a real target path.
- Construct one prepared-only `edge_publications` row that lacks matching Route
  5 authority.
- Prove the row fails closed on the real consumer.
- Preserve compatible positive output and exact target behavior.
- Record status/debug evidence that distinguishes prepared-only publication
  rejection from missing fixture support.

## Out of Scope

- Do not attempt all edge-publication families in one packet.
- Do not open `edge_publication_source_producers`, metadata, liveness,
  aggregate retirement, draft 155, or broad prepared API hiding.
- Do not demote, delete, privatize, wrap, or retire edge-publication prepared
  APIs from this one row.

## Acceptance Criteria

- The proof names the prepared-only `edge_publications` row and the missing or
  mismatched Route 5 authority.
- A real target consumer rejects the prepared-only row as semantic authority.
- Positive output and compatibility behavior remain stable.
- Status/debug, fallback, route-debug, prepared-printer, helper/oracle,
  wrapper, exact-output, and baseline behavior are not weakened.

## Reviewer Reject Signals

- Reject helper-only proofs claimed as backend edge-publication proof.
- Reject testcase-shaped matching, named-case-only branches, or synthetic state
  mutation that normal construction would reject.
- Reject expectation rewrites, unsupported downgrades, output relaxation,
  fallback weakening, route-debug weakening, prepared-printer weakening,
  wrapper weakening, or baseline weakening without explicit user approval.
- Reject broad Route 5 rewrites, prepared API privatization, adapter migration,
  aggregate retirement, metadata/liveness, or draft 155 work under this idea.
- Reject a route that leaves prepared-only publication acceptance intact behind
  a renamed helper or wrapper.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
