# Current Packet

Status: Active
Source Idea Path: ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map target public-field consumers

## Just Finished

Step 1 mapped the x86/riscv `PreparedFunctionLookups::memory_accesses`
consumer surface. Current direct public-field reads remain absent in x86 and
riscv backend code: x86 reads Route 3/Route 5 source-memory through
`PreparedEdgePublication::source_memory_access` plus
`PreparedAddressingFunction`, and riscv reads the same source-memory facts
through `PreparedEdgePublication` while carrying `PreparedFunctionLookups` for
edge publications and value homes.

Best candidate route exists on riscv:
`RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent()` ->
`render_edge_publication_source_operand(...)` in
`src/backend/mir/riscv/codegen/emit.cpp`. The dynamic stack-source `LoadLocal`
branch already renders `lw a1, 12(s2)` from a prepared source-memory row and
has the normal fixture path
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
`make_load_local_dynamic_stack_source(...)` /
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` plus
`check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`.
That path can legitimately be changed in Step 2 to fetch or verify the same
source-memory row from `lookups->memory_accesses` instead of relying only on
the pointer already embedded in the edge publication.

The adjacent x86 Route 3/Route 5 fixture remains adjacent evidence, not the
selected public-field consumer route. Preserve
`make_x86_param_eq_zero_branch_joined_loadlocal_or_sub_then_add_module()` /
`branch_join_loadlocal_then_add` output exactly; it currently proves selected
`LoadLocal` Route 3/Route 5 agreement and drift rejection, but does not
directly read `lookups->memory_accesses`.

## Suggested Next

Execute Step 2 on the riscv route only: make the dynamic stack-source
`LoadLocal` consumer in `render_edge_publication_source_operand(...)` read or
validate the row through `lookups->memory_accesses` using the existing prepared
lookup helpers, while keeping the existing emitted instruction text and
fail-closed statuses stable.

## Watchouts

- Do not claim current x86 or riscv coverage as a direct
  `PreparedFunctionLookups::memory_accesses` consumer; Step 1 found only a
  legitimate riscv candidate route, not an existing direct read.
- The riscv row to keep focused is the dynamic stack-source `LoadLocal` row:
  predecessor `left`, successor `join`, destination value id `2`, source
  `%src`, pointer base `%base`, byte offset `12`, size `4`, align `4`, emitted
  as `lw a1, 12(s2)`.
- The matching Route 3/Route 5 authority facts already exist in
  `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`:
  `route3_find_memory_access_record(..., 0, LoadLocal)` and
  `route5_cfg_edge_publication_record(...)` with `MemorySource`,
  `source_memory_identity_available`, and agreement/mismatch probes.
- Do not claim helper-only, oracle-only, edge-publication, or addressing-record
  evidence as backend `memory_accesses` public-field consumer coverage.
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

Read-only/todo-only Step 1 packet. No build or test proof required, and no
`test_after.log` was produced.

Suggested narrow proof for the next code-changing packet:
`cmake --build build --target backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' | tee test_after.log`
