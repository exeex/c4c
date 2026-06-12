Status: Active
Source Idea Path: ideas/open/211_route5_current_block_join_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Name the Reader and Baseline Prepared Behavior

# Current Packet

## Just Finished

Step 1 selected one current-block join-source semantic reader for idea 211:
`current_block_join_prepared_query_source(...)` in
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`. This is the dispatch
reader used by `src/backend/mir/aarch64/codegen/dispatch.cpp` to skip lowering a
current-block source instruction when the join parallel-copy source is already
authoritative for the current block.

Baseline behavior:

- Prepared baseline: when Route 5 identity is unavailable or rejected,
  `build_current_block_join_prepared_query_routing(...)` falls back to
  `prepare::prepare_current_block_join_parallel_copy_source_facts(...)`; the
  selected reader then returns the prepared source bit for the instruction
  result value id. Prepared edge publications, value homes, move bundles,
  branch policy, parallel-copy scheduling, and output remain authoritative.
- Missing-source behavior: if the selected instruction result has no prepared
  value id, the prepared facts are unavailable, or the routing table does not
  contain the instruction index, `current_block_join_prepared_query_source(...)`
  returns false and normal lowering/fallback remains in charge.
- Route 5 evidence source: `build_current_block_join_prepared_query_routing(...)`
  builds `bir::route5_build_edge_join_source_index(...)` and reads
  `mir::find_bir_current_block_join_source_identity(...)` for the successor
  block. Only `BirCurrentBlockJoinSourceStatus::Available` populates Route 5
  source bits; every other status falls through to prepared facts.
- Selected source identity: for the current test fixture, the selected source
  bit is true for `%current.join.routing.source` and false for the operand-only
  incoming expression `%current.join.routing.operand`.

Coverage inventory:

- Positive Route 5 source behavior: `backend_aarch64_current_block_join_routing`
  covers available Route 5 identity with and without attached prepared policy;
  `backend_aarch64_instruction_dispatch` covers the selected reader in the
  dispatch fixture.
- Absent Route 5 fallback: `backend_aarch64_current_block_join_routing` covers
  the no-PHI/absent-route case falling back to prepared source bits.
- Invalid/missing source evidence: `backend_prepared_lookup_helper` covers
  missing block, missing successor label, missing publication/no PHI, and
  missing named source producer statuses; the routing test covers no-source and
  memory-source shapes returning prepared-derived bits.
- Duplicate/conflict Route 5 evidence: `backend_prepared_lookup_helper` covers
  duplicate indexed current-block join records and wrong-predecessor indexed
  records failing closed as missing publication.
- Mismatch/fallback coverage gap: existing tests cover Route 5 helper-level
  destination/source type and missing-source mismatches, but the selected
  AArch64 reader does not yet have an explicit route/prepared disagreement
  assertion proving the prepared source bit is retained.
- Joined-branch and wrapper string surfaces: `backend_prepared_printer` and the
  configured wrapper tests should remain byte-stable. This build exposes
  `backend_riscv_prepared_edge_publication`; the x86 joined-branch source file
  is present behind `C4C_ENABLE_X86_BACKEND_TESTS`, but no
  `backend_x86_handoff_boundary` CTest is configured in the current build.

## Suggested Next

Execute Step 2 for the selected reader only. Add a narrow Route 5/prepared
agreement check for `current_block_join_prepared_query_source(...)` so Route 5
source bits are accepted only when they identify the same prepared current
block, join edge, source value, and destination/source relationship; absent,
invalid, duplicate/conflict, and mismatch cases must retain the prepared source
bit.

Recommended narrow proof command for the next code-changing packet:

```bash
cmake --build --preset default --target backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test backend_prepared_printer_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' > test_after.log
```

## Watchouts

- Keep the packet to one named current-block join-source reader.
- Do not move branch, parallel-copy, execution-site, value-home, move-bundle,
  final output, wrapper, or expected-string policy into Route 5.
- Do not migrate `current_block_join_prepared_query_incoming_expression(...)` in
  the same packet; it is adjacent but not the selected reader.
- Do not rewrite expected strings or wrapper behavior as proof. The reader is a
  semantic source identity gate only.
- Treat no-source, invalid/missing reference, duplicate/conflict,
  route/prepared mismatch, and prepared fallback as required proof dimensions.
- If x86 backend tests are enabled in a later build, add
  `backend_x86_handoff_boundary_test` and `backend_x86_handoff_boundary` to the
  final wrapper proof; they are not available in the current configured CTest
  set.

## Proof

Analysis-only packet; no build required and no `test_after.log` produced.
Inspected the selected reader and coverage using `rg`, the repo clang-tool
function-signature query, CMake/CTest target discovery, and focused reads of:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
