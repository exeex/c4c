# 274 Phase F5 RISC-V memory_accesses stale-publication fail-closed proof

Status: Closed - blocked by fixture support

## Closure Note

Step 1 found that the supported RISC-V dynamic stack-source `LoadLocal`
fixture can express the compatible `PreparedFunctionLookups::memory_accesses`
publication row, but cannot express a stale-publication row through normal
supported construction. The public row is tied to the same Route 3 / Route 5
memory-source identity; constructing the stale case would require synthetic
mutation or hand-built stale state, which this idea explicitly rejects.

The runbook is closed as a blocker per its acceptance criteria. Follow-up
fixture-support work is tracked in
`ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md`.

## Goal

Prove one bounded RISC-V same-consumer stale-publication
`PreparedFunctionLookups::memory_accesses` boundary row fails closed instead of
being accepted as Route 3 / Route 5 semantic agreement.

## Why This Exists

Ideas 272 and 273 proved one RISC-V same-consumer prepared-only row and one
byte-offset drift row for `memory_accesses`. Idea 265 still records
stale-publication blockers. This idea opens only the next bounded proof packet
from the draft 274 backlog so the remaining prepared/BIR boundary work can
continue without turning the whole backlog into one broad implementation plan.

## In Scope

- Start from the supported RISC-V dynamic stack-source `LoadLocal` fixture if
  it can express a stale-publication row through normal construction.
- Name the prepared memory row, the stale Route 3 / Route 5 authority relation,
  the expected fail-closed status, and the stable positive output.
- Prove that an old or stale prepared memory fact does not become Route 3 /
  Route 5 semantic agreement for the real backend consumer.
- Preserve the compatible positive path and exact RISC-V output for the
  same-consumer fixture.
- Keep the proof to one bounded stale-publication row unless investigation
  shows that fixture support must be opened as a separate follow-up idea.

## Out of Scope

- Do not open the full draft 274 backlog as one implementation plan.
- Do not claim x86 public-field parity from this RISC-V proof.
- Do not claim exhaustive byte-offset, cross-publication, edge-publication,
  metadata, liveness, aggregate retirement, or draft 155 readiness.
- Do not demote, delete, privatize, wrap, or retire prepared aggregate APIs
  from this one proof row.
- Do not move target policy into BIR; stack, storage, addressing, source-home,
  register, layout, wrapper, formatting, emission, instruction spelling, and
  exact output remain target-owned.

## Acceptance Criteria

- The implementation identifies whether the supported RISC-V dynamic
  stack-source `LoadLocal` construction can express the stale-publication row.
- If supported, the proof exercises a real backend same-consumer path that
  directly depends on the relevant `memory_accesses` boundary behavior.
- The stale prepared memory fact fails closed instead of being accepted as
  Route 3 / Route 5 agreement.
- The failure evidence records the relevant status/debug facts needed to
  distinguish stale-publication rejection from helper-only behavior.
- The positive same-consumer path remains byte-stable, including the expected
  RISC-V load output for the compatible fixture.
- Helper/oracle status, fallback behavior, route-debug output,
  prepared-printer output, wrapper output, exact target output, and baseline
  behavior are not weakened.
- If normal construction cannot express the stale-publication row, this idea
  closes as a blocker with a narrower fixture-support follow-up under
  `ideas/open/`.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts, named-case-only branches, or synthetic
  stale state that normal construction would reject.
- Reject helper-only proofs that are claimed as backend same-consumer boundary
  proof.
- Reject expectation rewrites, weaker unsupported/status contracts, output
  relaxations, fallback relaxations, route-debug weakening, prepared-printer
  weakening, wrapper weakening, or baseline weakening without explicit user
  approval.
- Reject changes that prove only classification or helper formatting while
  leaving the real RISC-V consumer able to accept stale `memory_accesses`
  agreement.
- Reject broad rewrites of Route 3 / Route 5, prepared aggregate retirement,
  API privatization, edge-publication families, metadata, liveness, or x86
  parity work under this idea.
- Reject any route that retains the exact old stale-publication acceptance
  failure mode behind a renamed helper, wrapper, or abstraction.

## Source

Extracted from `ideas/draft/274_phase_f5_remaining_prepared_boundary_proof_backlog.md`.
