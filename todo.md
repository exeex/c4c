# Current Packet

Status: Active
Source Idea Path: ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Record row and authority facts for follow-up proof

## Just Finished

Step 3 recorded the row and authority facts needed for a later prepared-only
same-consumer fail-closed proof.

Exact backend consumer: the RISC-V dynamic stack-source `LoadLocal` path in
`RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent()` via
`render_edge_publication_source_operand(...)`. The consumer directly reads
`lookups->memory_accesses` by calling
`prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
&lookups->memory_accesses, *publication.source_value_id)` and rejects the
publication if the indexed row is missing or not the same row carried by
`publication.source_memory_access`.

Exact prepared memory row: function `join_regs`, block `left`, instruction
index `0`, producer kind `LoadLocal`, source value id `1`, destination value id
`2`, source `%src`, destination `%dst`, pointer base `%base`, byte offset `12`,
size `4`, align `4`, default address space, non-volatile, base-plus-offset
available, no materialization required. Normal prepared lookup construction
indexes this row in `PreparedFunctionLookups::memory_accesses` by result value
id `1`.

Route 3/Route 5 authority facts for the later proof: the matching diagnostic
authority rows are the dynamic memory-source rows in
`check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`.
`route3_find_memory_access_record(route3_index, 0, LoadLocal)` identifies the
`load_local` row with pointer-value base `%base`, byte offset `12`, and size
`4`. `route5_cfg_edge_publication_record(...)` reports `MemorySource`, has
`source_memory_identity_available`, points at the same Route 3 instruction, and
agrees with the prepared publication on source memory byte offset and size.
The same fixture also proves mismatched or incomplete Route 3/Route 5 facts are
diagnostic only: the prepared-backed output remains available and renders
`lw a1, 12(s2)`.

Supported fixture path: `make_load_local_dynamic_stack_source(prepared, ids,
load_local_source_access(ids))` followed by normal
`make_prepared_function_lookups(...)`, exercised by
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`. The later
fail-closed proof should keep using this normal construction path instead of
hand-built stale prepared state.

Preserved adjacent compatibility output: the RISC-V dynamic memory-source
output remains `lw a1, 12(s2)`. The adjacent x86 Route 3/Route 5 fixture
`branch_join_loadlocal_then_add` remains compatibility evidence to preserve,
but it is not the direct public `memory_accesses` consumer for this route.

Changed files: `todo.md` only.

## Suggested Next

Execute the follow-up prepared-only same-consumer fail-closed proof against the
RISC-V dynamic stack-source `LoadLocal` consumer. Recommended proof command:

`bash -lc "cmake --build --preset default --target backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'" > test_after.log 2>&1`

## Watchouts

- The backend consumer evidence is the direct `lookups->memory_accesses` read
  in `render_edge_publication_source_operand`; helper, oracle,
  edge-publication, and addressing-record evidence remains supporting context
  only.
- The later fail-closed proof should remove or corrupt the exact public
  `memory_accesses` row reached by normal construction and prove the same
  consumer fails closed with `UnsupportedSourceHome` and no rendered
  instruction.
- Route 3/Route 5 disagreement is not enough by itself for the prepared-only
  proof because existing coverage deliberately preserves prepared fallback on
  non-agreeing or incomplete diagnostic facts.
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

No build or test was run for this Step 3 todo-only packet, per delegated proof.
No new `test_after.log` was produced by this packet.

Existing Step 2 delegated proof passed and was written to `test_after.log`:

`bash -lc "cmake --build --preset default --target backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'" > test_after.log 2>&1`

Result: target `backend_riscv_prepared_edge_publication_test` built, and
CTest `backend_riscv_prepared_edge_publication` passed (`1/1`, `0` failed).

Supervisor extra validation also passed:

`ctest --test-dir build -j --output-on-failure -R 'riscv'`

Result: `3/3` tests passed.
