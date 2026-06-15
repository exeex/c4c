# 277 Phase F5 memory_accesses cross-publication mismatch fail-closed proof

## Goal

Prove one real-target `memory_accesses` cross-publication mismatch row fails
closed when the prepared/public memory row belongs to a different owner or
publication than the Route 3 / Route 5 semantic authority.

## Why This Exists

The Phase F5 `memory_accesses` proof map still lacks a combined memory-source
target proof for internally consistent but wrong prepared owners. Prepared-only,
byte-offset drift, and stale-publication rows do not exhaust duplicate,
conflicting, wrong-edge, wrong-destination, or cross-publication mismatch
families.

## In Scope

- Pick one real target consumer with existing or supportable fixture coverage.
- Construct one prepared memory/source row whose owner or publication does not
  match the Route 3 / Route 5 semantic authority.
- Prove the mismatch fails closed instead of being accepted as agreement.
- Preserve exact target output and compatibility surfaces for the positive
  path.
- Record enough status/debug evidence to tell owner/publication mismatch apart
  from offset drift or stale-publication rejection.

## Out of Scope

- Do not attempt exhaustive cross-target matrices in this packet.
- Do not fold in byte-offset coverage, x86 parity, edge-publication families,
  metadata, liveness, aggregate retirement, or draft 155 work.
- Do not treat helper-only classification as backend boundary proof.

## Acceptance Criteria

- The proof names the prepared row, the mismatched owner/publication, and the
  Route 3 / Route 5 authority row.
- The real target consumer rejects the mismatched publication as semantic
  agreement.
- Positive output and compatibility rows remain stable.
- Status/debug evidence distinguishes cross-publication mismatch from
  prepared-only, byte-offset drift, and stale-publication cases.
- No fallback, wrapper, route-debug, prepared-printer, helper/oracle, exact
  output, or baseline contract is weakened.

## Reviewer Reject Signals

- Reject testcase-shaped matching, named-case-only shortcuts, or synthetic
  mutation that normal fixture construction would reject.
- Reject helper-only behavior claimed as real backend proof.
- Reject expectation rewrites, unsupported downgrades, weaker status or output
  contracts, fallback weakening, route-debug weakening, prepared-printer
  weakening, wrapper weakening, or baseline weakening without explicit user
  approval.
- Reject broad Route 3 / Route 5 rewrites or prepared API retirement under
  this idea.
- Reject any route that accepts a wrong owner/publication row as agreement
  behind a renamed helper or wrapper.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
