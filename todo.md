Status: Active
Source Idea Path: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Real Consumer Fixture

# Current Packet

## Just Finished

Completed Step 1 of `plan.md` by selecting the existing RISC-V dynamic
`LoadLocal` stack-source edge fixture in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`:
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` plus
the Route 3 / Route 5 oracle coverage in
`check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`.

Compatible contract to preserve: `make_load_local_dynamic_stack_source()`
with `load_local_source_access(ids)` publishes source value id `1` / `%src`
from predecessor block `left`, instruction index `0`, pointer base `%base`
in register `s2`, byte offset `12`, size `4`, align `4`, and the RISC-V
consumer emits exactly `lw a1, 12(s2)` with
`EdgePublicationMoveIntentStatus::Available`.

Route 3 / Route 5 authority row to keep agreeing: the Route 3 `LoadLocal`
record from `route3_build_memory_access_index(route3_predecessor)` at
instruction index `0`, and the `route5_cfg_edge_publication_record(...)`
for predecessor `left`, successor `join`, destination `%dst`, prepared
destination id `2`, prepared source name `%src`, with
`Route5PublicationStatus::MemorySource`,
`source_memory_identity_available == true`, `byte_offset == 12`, and both
`route5_edge_source_agrees` and `route3_source_memory_agrees` expected true
for the compatible path.

Smallest supportable mismatch to prove next: a normal second public
`PreparedMemoryAccess` row for the same `%src` / value id `1` with the same
address facts as `load_local_source_access(ids)` but a different publication
block label, `ids.successor` (`join`) instead of `ids.predecessor` (`left`).
This selects a publication/block-label mismatch, not an owner mismatch, and
keeps the offset at `12` so the failure is not byte-offset drift or the prior
stale-offset `16` case.

## Suggested Next

Execute Step 2 by extending the selected RISC-V test with the wrong-publication
row above, then drive `consume_edge_publication_move_intent(...,
&route5_memory_edge)` through the real consumer path and assert fail-closed
status/debug evidence while the compatible row still emits `lw a1, 12(s2)`.

## Watchouts

- Do not claim cross-publication coverage through helper-only behavior.
- Do not use synthetic mutation that normal fixture construction would reject.
- Keep Route 3 / Route 5 authority current and agreeing at `left:0`, offset
  `12`; the wrong-publication row should be visible through public
  `memory_accesses` lookup ambiguity, not by mutating lookup maps or changing
  the selected publication's `source_memory_access`.
- Prefer publication/block-label mismatch over owner mismatch for the first
  proof because `PreparedMemoryAccessLookups` naturally preserves the extra
  public `%src` row by result value id, and the RISC-V consumer already requires
  that public lookup to be unique and identical to the selected publication row.
- Do not weaken fallback, status/debug, route-debug, prepared-printer, wrapper,
  helper/oracle, exact-output, or baseline contracts.
- Keep byte-offset, stale-publication, x86 parity, edge-publication, metadata,
  liveness, aggregate retirement, and draft 155 work out of this plan.

## Proof

Inspection-only packet per supervisor delegation. No build or tests run, and
`test_after.log` was not touched.
