Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Backend-Wide Closure Validation

# Current Packet

## Just Finished

Completed plan.md Step 8 by refining the string-pointer publication regression
fix found during backend-wide closure validation.

Regression fix refined:

- String-pool global pointer stores into local pointer slots keep the
  route-visible SSA store (`bir.store_local %lv.p, ptr %t0`) and then publish
  the canonical string literal address value (`@.str0`) for semantic tracking.
- Non-string globals still require valid LinkNameId metadata before publishing
  symbol pointer values.
- `backend_lir_to_bir_notes` passes.
- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
  passes.
- The accepted RV64 runtime pointer cases
  `backend_rv64_runtime_defined_global_array_pointer` and
  `backend_rv64_runtime_defined_global_array_pointer_store` still pass.
- The only failure in the delegated proof regex is the documented
  `backend_riscv_prepared_edge_publication` baseline:
  `RISC-V prepared module should emit a register edge move`.
- Supervisor backend-wide closure validation also preserved only that
  documented baseline failure: 209 of 210 selected `^backend_` tests passed.

## Suggested Next

Supervisor should review the Step 8 closure result and decide whether to accept
the slice with the documented `backend_riscv_prepared_edge_publication`
baseline still present.

## Watchouts

- The refined string-pointer representation intentionally emits a redundant
  same-value local pointer store for string constants: first the SSA value for
  route stability, then the canonical string symbol for semantic publication.
- The known `backend_riscv_prepared_edge_publication` failure remains outside
  this source idea.

## Proof

Delegated proof command wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_lir_to_bir_notes|backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir|backend_riscv|backend_codegen_route_riscv64|^backend_rv64_runtime_defined_global_array_pointer(_store)?$" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; `backend_lir_to_bir_notes`,
`backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`,
both `backend_codegen_route_riscv64_*` tests, and both accepted RV64 runtime
pointer cases passed. The only selected failure was the documented
`backend_riscv_prepared_edge_publication` baseline failure.

Supervisor backend-wide closure command wrote the final `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; 209 of 210 selected backend tests passed; the only
failure was the documented `backend_riscv_prepared_edge_publication` baseline
failure.
