# 281 Phase F5 edge_publication_source_producers x86 consumer boundary gate

## Goal

Name and prove, or explicitly block, the concrete x86 consumer boundary for
`PreparedFunctionLookups::edge_publication_source_producers`.

## Why This Exists

Idea 267 still blocks `edge_publication_source_producers` movement because no
complete concrete x86 consumer boundary has been named. Route 5
edge-publication move support is only partial copied-publication support for
`LoadLocal`, and non-`LoadLocal` compatibility fallback remains the key
blocker.

## In Scope

- Inspect x86 consumption of `edge_publication_source_producers` and adjacent
  Route 5 edge-publication support.
- Name one concrete x86 consumer boundary if it exists.
- If supportable, prove one bounded x86 row that preserves compatible output
  and fallback behavior.
- If not supportable, close with a precise blocker and the smallest follow-up
  fixture-support idea needed.

## Out of Scope

- Do not open implementation work until a real x86 consumer boundary is named.
- Do not fold in memory_accesses proof rows, edge_publications proof families,
  metadata, liveness, aggregate retirement, or draft 155 work.
- Do not claim non-`LoadLocal` fallback completion without direct evidence.

## Acceptance Criteria

- The gate names the x86 consumer surface examined and whether it is a real
  boundary.
- Any proof exercises a real x86 consumer and preserves exact compatible
  output/fallback behavior.
- If blocked, the closure note names the exact missing fixture or consumer
  support rather than claiming progress.
- Status/debug, route-debug, prepared-printer, helper/oracle, wrapper,
  exact-output, fallback, and baseline behavior are not weakened.

## Reviewer Reject Signals

- Reject helper-only or classification-only proofs claimed as x86 consumer
  boundary proof.
- Reject testcase-shaped shortcuts, named-case-only branches, or expectation
  rewrites that weaken compatibility fallback.
- Reject broad x86 lowering rewrites, Route 5 rewrites, prepared API
  privatization, aggregate retirement, metadata/liveness, or draft 155 work
  under this gate.
- Reject claims of non-`LoadLocal` fallback completion without a real consumer
  proof.
- Reject any route that hides the absence of a concrete x86 consumer boundary
  behind a renamed helper or wrapper.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
