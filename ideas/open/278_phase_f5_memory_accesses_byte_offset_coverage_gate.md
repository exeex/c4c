# 278 Phase F5 memory_accesses byte-offset coverage gate

## Goal

Decide and prove the next bounded byte-offset coverage step for
`PreparedFunctionLookups::memory_accesses` after the selected RISC-V
same-consumer row from idea 273.

## Why This Exists

Idea 273 proved one RISC-V same-consumer byte-offset drift row, but it did not
clear every synthetic prepared offset drift row, every public lookup consumer,
every target operand surface, or every wrapper/exact-output contract. A gate is
needed before claiming byte-offset coverage is broad enough for later prepared
boundary decisions.

## In Scope

- Inventory the remaining byte-offset drift surfaces that matter to
  `memory_accesses` proof completeness.
- Choose one bounded next proof row or explicitly conclude that a separate
  fixture-support idea is needed first.
- If a row is supportable, prove the drift fails closed on a real consumer and
  preserve the compatible positive output.
- Record what coverage remains after this gate.

## Out of Scope

- Do not claim exhaustive byte-offset completion unless the evidence actually
  covers the named surfaces.
- Do not fold in stale-publication, cross-publication, x86 parity,
  edge-publication families, metadata, liveness, aggregate retirement, or draft
  155 work.
- Do not rewrite broad target memory lowering as part of the gate.

## Acceptance Criteria

- The gate names the remaining byte-offset drift surfaces and identifies the
  next bounded proof row or blocker.
- Any implemented proof exercises a real target consumer and preserves the
  compatible exact output.
- Closure notes state which byte-offset surfaces remain unsupported,
  unexamined, or proven.
- Fallback, status/debug, helper/oracle, route-debug, prepared-printer,
  wrapper, exact-output, and baseline contracts are not weakened.

## Reviewer Reject Signals

- Reject claims of exhaustive byte-offset coverage based on one narrow row.
- Reject helper-only proofs claimed as backend consumer proof.
- Reject testcase-shaped matching, named-case-only branches, or expectation
  rewrites that downgrade the proof contract.
- Reject output relaxation, fallback weakening, route-debug weakening,
  prepared-printer weakening, wrapper weakening, or baseline weakening without
  explicit user approval.
- Reject broad Route 3 / Route 5 rewrites, prepared API retirement, x86 parity,
  edge-publication, metadata, liveness, or draft 155 work under this idea.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
