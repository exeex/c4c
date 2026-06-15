# Current Packet

Status: Active
Source Idea Path: ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Expose supported fixture reachability

## Just Finished

Step 2 exposed supported RISC-V fixture reachability through the public
`PreparedFunctionLookups::memory_accesses` field. The dynamic stack-source
`LoadLocal` consumer
`RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent()` ->
`render_edge_publication_source_operand(...)` now validates the publication's
source-memory row by calling
`prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
&lookups->memory_accesses, *publication.source_value_id)` before rendering the
load.

The normal fixture path remains
`make_load_local_dynamic_stack_source(...)` /
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`. It proves
the selected prepared row is reachable through normal lookup construction:
source value id `1`, destination value id `2`, predecessor `left`, successor
`join`, source `%src`, pointer base `%base`, byte offset `12`, size `4`, align
`4`, emitted text `lw a1, 12(s2)`. Clearing
`lookups.memory_accesses.accesses_by_result_value_id` now makes the backend
consumer fail closed with `UnsupportedSourceHome` and no rendered instruction.

Changed files: `src/backend/mir/riscv/codegen/emit.cpp`,
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, and
`todo.md`.

## Suggested Next

Execute Step 3 by recording the final row and authority facts for the follow-up
prepared-only same-consumer proof. Suggested focus: document that the backend
consumer is the RISC-V dynamic stack-source `LoadLocal` path above, the
authority row is the normal prepared memory row for `%src` at `left` index `0`,
and the matching Route 3/Route 5 facts are the existing
`route3_find_memory_access_record(..., 0, LoadLocal)` and
`route5_cfg_edge_publication_record(...)` memory-source rows.

## Watchouts

- The RISC-V row to keep focused is the dynamic stack-source `LoadLocal` row:
  predecessor `left`, successor `join`, source value id `1`, destination value
  id `2`, source `%src`, pointer base `%base`, byte offset `12`, size `4`,
  align `4`, emitted as `lw a1, 12(s2)`.
- The matching Route 3/Route 5 authority facts already exist in
  `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`:
  `route3_find_memory_access_record(..., 0, LoadLocal)` and
  `route5_cfg_edge_publication_record(...)` with `MemorySource`,
  `source_memory_identity_available`, and agreement/mismatch probes.
- The backend consumer evidence is now the direct
  `lookups->memory_accesses` read in `render_edge_publication_source_operand`;
  helper, oracle, edge-publication, and addressing-record evidence remains
  supporting context only.
- Do not use hand-built stale prepared state or test-only mutation that
  bypasses normal prepared lookup construction.
- Do not add named-fixture shortcuts, branch-specific matching, or
  testcase-shaped logic.
- Preserve the adjacent x86 `branch_join_loadlocal_then_add` compatibility
  output unless the supervisor accepts a reviewed contract change.
- Do not weaken unsupported status, helper/oracle status, fallback behavior,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baselines.

## Proof

Delegated proof passed and was written to `test_after.log`:

`bash -lc "cmake --build --preset default --target backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'" > test_after.log 2>&1`

Result: target `backend_riscv_prepared_edge_publication_test` built, and
CTest `backend_riscv_prepared_edge_publication` passed (`1/1`, `0` failed).
