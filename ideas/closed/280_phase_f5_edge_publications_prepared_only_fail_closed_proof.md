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

## Completion Notes

Closed on 2026-06-15 after proving one bounded prepared-only
`PreparedFunctionLookups::edge_publications` row fails closed on a real target
consumer. The proven row is the RISC-V dynamic `LoadLocal` publication for
`left -> join`, destination value id `2` / `%dst`, source value id `1` /
`%src`, source memory `%base + 12`, size `4`, and destination register `a1`.

The proof keeps that public prepared row visible, supplies same-edge Route 5
`NoSource` authority through
`riscv::consume_edge_publication_move_intent(..., &route5_edge)`, and verifies
that no instruction is emitted from prepared-only authority. The compatible
positive path remains the matching Route 5 `MemorySource` dynamic stack-source
row with `Available` intent status and exact `lw a1, 12(s2)` output.

Close proof used matching `test_before.log` and `test_after.log` for
`^backend_riscv_prepared_edge_publication$`; the regression guard passed with
1/1 tests passing before and after, no new failures.

Remaining nearby edge-publication families are later scope: duplicate Route 5
records, Route 5-only rows, wrong-edge rows, scalar register-source rows that
can still permit prepared fallback, metadata, liveness,
`edge_publication_source_producers`, aggregate retirement, and draft 155 work.
No broad prepared API retirement, migration, privatization, deletion, demotion,
wrapping, or adapter conversion was performed under this idea.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
