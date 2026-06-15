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

## Completion Notes

Closed after proving the RISC-V same-consumer public lookup path with a
same-block duplicate `PreparedMemoryAccess` row for the same `%src` value at
byte offset `16`, while the selected publication, selected position row, and
Route 3 / Route 5 authority remain at byte offset `12`. The real
`consume_edge_publication_move_intent(..., &route5_memory_edge)` consumer fails
closed as `UnsupportedSourceHome`, emits no instruction, records no
source-memory offset, and keeps Route 5 / Route 3 agreement true for the
selected offset `12` row. The compatible dynamic stack-source `LoadLocal` path
still emits exact `lw a1, 12(s2)`.

Remaining unsupported or unexamined byte-offset surfaces are preserved for
future ideas: shared source-producer planner consumers using
result-value-name or positional `find_indexed_prepared_memory_access` paths,
AArch64 target operand / ALU / memory operand consumers of prepared-memory
lookup data, wrapper-level negative exact output for a same-block duplicate
public row, and synthetic prepared offset drift between
`publication.source_memory_byte_offset`, `publication.source_memory_access`,
and alternate public lookup entries with the same source value.

Close proof used matching `test_before.log` and `test_after.log` for
`^backend_riscv_prepared_edge_publication$`; the non-decreasing regression
guard passed with 1/1 tests passing before and after.

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
