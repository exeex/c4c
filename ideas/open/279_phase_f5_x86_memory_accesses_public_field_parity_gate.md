# 279 Phase F5 x86 memory_accesses public-field parity gate

## Goal

Determine whether x86 has a real public
`PreparedFunctionLookups::memory_accesses` consumer boundary comparable to the
RISC-V fixture path, and either prove one bounded x86 row or record explicit
fail-closed non-applicability.

## Why This Exists

The current RISC-V proofs do not establish x86 public-field parity. Existing
x86 paths mostly consume prepared edge-publication source-memory and addressing
records rather than the public `memory_accesses` field. A dedicated gate is
needed before claiming parity or using RISC-V evidence to justify x86 prepared
boundary movement.

## In Scope

- Inspect x86 `memory_accesses` consumption and adjacent Route 3 / Route 5
  agreement support.
- Identify a real public-field consumer if one exists.
- If supportable, prove one bounded x86 public-field row with exact positive
  output preservation.
- If no real public-field consumer exists, record explicit non-applicability
  and the remaining x86 boundary blocker.

## Out of Scope

- Do not claim x86 parity from RISC-V-only proof rows.
- Do not migrate x86 consumers or retire prepared APIs unless a separate idea
  explicitly opens that implementation work.
- Do not fold in RISC-V stale/cross/byte-offset proof work, edge-publication
  families, metadata, liveness, aggregate retirement, or draft 155 work.

## Acceptance Criteria

- The gate names the x86 consumer surface examined.
- If a public-field consumer exists, the proof exercises that real x86 path and
  preserves exact output.
- If no public-field consumer exists, the closure note records explicit
  fail-closed non-applicability rather than silent parity.
- Status/debug, fallback, helper/oracle, route-debug, prepared-printer,
  wrapper, exact-output, and baseline behavior are not weakened.

## Reviewer Reject Signals

- Reject parity claims based only on RISC-V evidence or helper-only x86
  formatting.
- Reject testcase-shaped x86 shortcuts, named-case-only branches, or
  expectation rewrites that weaken the public-field contract.
- Reject consumer migration, prepared API privatization, aggregate retirement,
  or broad x86 lowering rewrites under this gate.
- Reject unsupported downgrades, output relaxation, fallback weakening,
  route-debug weakening, prepared-printer weakening, wrapper weakening, or
  baseline weakening without explicit user approval.
- Reject a route that hides the absence of an x86 public-field consumer behind
  a renamed helper or wrapper.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
