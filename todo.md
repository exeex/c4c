Status: Active
Source Idea Path: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Remaining Byte-Offset Surfaces

# Current Packet

## Just Finished

Completed Step 1 of `plan.md` by inventorying the remaining
`PreparedFunctionLookups::memory_accesses` byte-offset drift surfaces after the
existing RISC-V dynamic stack-source coverage.

Already-proven same-consumer RISC-V rows in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`:

- Compatible dynamic stack-source `LoadLocal` row: selected public
  `PreparedMemoryAccess` and publication carry pointer base `%base`, offset
  `12`, size `4`, align `4`; the real RISC-V consumer emits exact
  `lw a1, 12(s2)`.
- Stale public byte-offset row: the selected predecessor row stays at offset
  `12`, a separate public row for the same source value is visible at offset
  `16`, current Route 3 / Route 5 authority still agrees at offset `12`, and
  the real RISC-V consumer fails closed with no instruction and no recorded
  source-memory offset.
- Synthetic Route 3 / Route 5 byte-offset drift row: prepared publication and
  public lookup stay at offset `12`, the supplied Route 5 memory-source record
  is mutated to offset `16`, and the real RISC-V consumer fails closed with
  Route 5 / Route 3 agreement flags false.

Remaining byte-offset drift surfaces:

- Public lookup consumers: RISC-V edge publication uses
  `find_unique_indexed_prepared_memory_access_by_result_value_id`, while the
  shared source-producer planner also has result-value-name and positional
  `find_indexed_prepared_memory_access` paths; duplicate public rows with
  byte-offset drift are not exhausted by the stale cross-block row.
- Target operand surfaces: AArch64 target operand / ALU / memory operand code
  still has prepared-memory lookup consumers by result value id and positional
  access, but those are out of this RISC-V gate unless the supervisor opens an
  explicit target-operand packet.
- Wrapper / exact-output contracts: the compatible RISC-V wrapper output is
  proven for `lw a1, 12(s2)`, but wrapper-level negative exact output for a
  same-block duplicate public row remains unexamined.
- Synthetic prepared offset drift rows: current proof covers Route 5 / Route 3
  offset drift to `16`; it does not exhaust drift between
  `publication.source_memory_byte_offset`, `publication.source_memory_access`,
  and alternate public lookup entries with the same source value.

## Suggested Next

Step 2 should add one bounded RISC-V same-consumer proof row: keep the selected
publication, selected position row, and Route 3 / Route 5 authority at offset
`12`, then add a second same-block public `PreparedMemoryAccess` for the same
source value at offset `16`. Drive
`consume_edge_publication_move_intent(..., &route5_memory_edge)` through the
real RISC-V consumer and assert fail-closed behavior with no instruction, no
source-memory offset, and still-true Route 5 / Route 3 agreement for the
selected offset `12` row.

## Watchouts

This is a supportable fixture row, not a fixture-support blocker. Keep it
distinct from the already-proven stale cross-block offset `16` row by using the
same predecessor block as the selected row. Do not claim exhaustive byte-offset
coverage from this one row, and do not weaken fallback, status/debug,
route-debug, helper/oracle, prepared-printer, wrapper, exact-output, or
baseline contracts.

## Proof

Inspection-only packet per supervisor delegation; no build or tests were run,
and `test_after.log` was not touched.
