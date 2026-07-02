# Current Packet

Status: Complete
Source Idea Path: ideas/open/525_bir_route3_memory_access_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map route3 memory-access symbols and dependencies

## Just Finished

- Step 1 mapping completed with `c4c-clang-tools`; no implementation files were
  edited.
- AST-backed inventory found the route3 memory-access implementation bodies in
  `src/backend/bir/bir.cpp`:
  `route3_memory_access_node_kind`,
  `route3_memory_access_base_kind`, `route3_memory_access_record`,
  `route3_memory_access_result_value_record`,
  `route3_memory_access_stored_value_record`,
  `route3_build_memory_access_index`,
  `route3_find_memory_access_record`,
  `route3_same_block_global_load_access_record`,
  `route3_same_block_load_local_source_record`,
  `route3_find_same_block_global_load_access`,
  `route3_find_same_block_load_local_source`, and
  `route3_find_same_block_load_local_stored_value_source`.
- Private helpers in the same move cluster are the anonymous-namespace helpers
  `route3_result_value`, `route3_stored_value`, `route3_memory_address`,
  `route3_same_local_slot`, `route3_same_local_range`, and
  `route3_local_ranges_overlap`; move them with the bodies as private helpers,
  not as public declarations.
- Public route3 enums, records, query structs, name helpers, and declarations
  stay in `src/backend/bir/bir.hpp`; no public route3 declaration or memory
  authority record should move.
- Proposed focused owner for Step 2: add
  `src/backend/bir/bir_route3_memory.cpp` and move only the route3
  memory-access bodies/helpers above into it, with build wiring for that new
  translation unit.
- Direct AST callers to protect:
  `route5_cfg_edge_publication_record` calls
  `route3_build_memory_access_index` and `route3_find_memory_access_record`;
  `route6_call_argument_publication_source_record` calls
  `route3_memory_access_record` after
  `find_call_argument_publication_source_routing`.
- Extra source consumers found by search/AST: `src/backend/mir/query.cpp`
  mirrors route3 records into MIR query identities; RISC-V
  `prepared_edge_publication_emit.cpp` checks route3 source-memory agreement;
  x86 module code also queries route3 load-local memory facts. These consumers
  should be protected by focused tests, not moved in this slice.

## Suggested Next

- Delegate Step 2 from `plan.md`: create `src/backend/bir/bir_route3_memory.cpp`,
  move the mapped route3 memory-access bodies/helpers intact, add the new file
  to build wiring, and leave public declarations in `bir.hpp`.

## Watchouts

- Keep public route3 declarations in `bir.hpp`.
- Preserve route6 access to the same route3 facts.
- Do not move memory authority records into private lowering files.
- Keep `route5_cfg_edge_publication_record`,
  `route6_call_argument_publication_source_record`, and
  `find_call_argument_publication_source_routing` in their current owner; they
  are consumers of public route3 facts for this plan, not part of the route3
  body move.
- Do not promote the route3 anonymous helpers unless the move exposes a real
  linkage need; they can remain private in `bir_route3_memory.cpp`.
- Tests identified for the next proof: `backend_aarch64_prepared_memory_operand_records`
  covers direct route3 record/index/value/global-load behavior;
  `backend_store_source_publication_plan` covers route3 load-local and
  stored-value source queries; `backend_lir_to_bir_notes` covers public
  route6 call-argument publication routing; and
  `backend_riscv_prepared_edge_publication` covers route5/route3
  source-memory agreement consumed by RISC-V publication code.

## Proof

- Not run; this was a mapping-only packet and the delegated proof explicitly
  required no build or CTest run. No `test_after.log` was produced.
- Selected future initial proof command for the body move:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_lir_to_bir_notes|backend_riscv_prepared_edge_publication)$'`
