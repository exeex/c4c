Status: Active
Source Idea Path: ideas/open/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Fixture Expressiveness

# Current Packet

## Just Finished

Step 1 - Inspect Fixture Expressiveness: inspected
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`,
`src/backend/mir/riscv/codegen/emit.cpp`, and
`src/backend/prealloc/prepared_lookups.cpp`.

The supported RISC-V dynamic stack-source fixture is
`make_load_local_dynamic_stack_source` plus
`check_load_local_dynamic_stack_source_exposes_shared_memory_access` in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`. Its
normal prepared row is `load_local_source_access(ids)`, published through
`PreparedFunctionLookups::memory_accesses` by
`find_unique_indexed_prepared_memory_access_by_result_value_id(&lookups.memory_accesses, 1)`.
The row describes `%src` from `left` instruction `0`, `LoadLocal`, pointer
base `%base`, byte offset `12`, size `4`, align `4`, and is consumed by the
RISC-V helper as `lw a1, 12(s2)`.

The named stale authority relation would be the Route 5 memory-source edge
record from `route5_cfg_edge_publication_record(...)` and its embedded Route 3
`source_memory_access`, compared by
`route5_edge_source_agrees_with_prepared_publication` /
`route3_source_memory_agrees_with_prepared_publication`. Existing mismatch
coverage mutates that Route 3/Route 5 oracle row after construction, but the
normal fixture construction ties the prepared publication's
`source_memory_access` pointer to the same public
`PreparedFunctionLookups::memory_accesses` row by source value id.

Conclusion: normal construction can express the compatible publication row, but
cannot express a stale-publication `memory_accesses` row without synthetic
mutation or hand-built stale state.

## Suggested Next

Stop this runbook as blocked and open the narrower fixture-support follow-up:
add supported RISC-V prepared-fixture construction for a stale
`PreparedFunctionLookups::memory_accesses` publication row, where the public
memory row can intentionally differ from the current Route 3 / Route 5 memory
identity while still being produced through supported preparation APIs.

## Watchouts

- Do not synthesize stale state that normal construction would reject.
- Do not broaden this into the full draft 274 backlog.
- Do not weaken positive output, fallback, status, route-debug,
  prepared-printer, wrapper, or baseline contracts.
- The existing supported fixture already proves the positive row and the public
  lookup requirement; clearing `lookups.memory_accesses.accesses_by_result_value_id`
  or mutating `route5_memory_edge.source_memory_access.byte_offset` is synthetic
  state, not normal stale-publication construction.

## Proof

Read-only investigation packet. No build or test proof required by the
delegated proof contract; `test_after.log` was not touched.
