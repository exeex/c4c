# Current Packet

Status: Complete
Source Idea Path: ideas/open/525_bir_route3_memory_access_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract route3 memory-access bodies

## Just Finished

- Step 2 extracted the mapped route3 memory-access bodies and private helpers
  from `src/backend/bir/bir.cpp` into the new focused owner
  `src/backend/bir/bir_route3_memory.cpp` without changing public declarations
  in `src/backend/bir/bir.hpp`.
- `bir.cpp` retains the route5 and route6 consumers:
  `route5_cfg_edge_publication_record`,
  `route6_call_argument_publication_source_record`, and
  `find_call_argument_publication_source_routing`.
- Added `src/backend/bir/bir_route3_memory.cpp` to the direct-source
  `backend_lir_to_bir_notes_test` source list; production `c4c_backend` already
  picks up the new BIR source through the existing glob.

## Suggested Next

- Supervisor should review the body-move diff for route drift and commit this
  coherent Step 2 slice if accepted.

## Watchouts

- No route3 helper promotion or `bir.hpp` edits were needed.
- The extraction depends on the existing BIR source glob for production builds
  and explicit direct-source wiring only for `backend_lir_to_bir_notes_test`.

## Proof

- Passed:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_lir_to_bir_notes|backend_riscv_prepared_edge_publication)$') > test_after.log 2>&1`
- `test_after.log` contains a successful build and 4/4 passing focused tests:
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_store_source_publication_plan`, `backend_lir_to_bir_notes`, and
  `backend_riscv_prepared_edge_publication`.
