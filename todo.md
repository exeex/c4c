Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Focused RISC-V Validation

# Current Packet

## Just Finished

Completed plan.md Step 7 by running focused RISC-V/backend-route validation
after accepting both local-pointer global-array runtime cases.

Focused validation result:

- Build completed with no work to do.
- `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi`
  passed.
- `backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir`
  passed.
- `backend_riscv_prepared_edge_publication` failed with the existing baseline
  message: `RISC-V prepared module should emit a register edge move`.

This matches the plan's documented accepted baseline failure and did not expose
a new focused RISC-V/backend-route regression.

## Suggested Next

Continue with plan.md Step 8 only if delegated: run backend-wide closure
validation for the accepted local-pointer global-array capability and confirm
only documented baseline failures remain.

## Watchouts

- The known `backend_riscv_prepared_edge_publication` failure remains outside
  this source idea.
- Step 8 should preserve both accepted local-pointer global-array runtime cases
  and report only documented baseline failures.

## Proof

Focused validation command wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_riscv|backend_codegen_route_riscv64" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; 2 of 3 selected tests passed; the only failure was the
documented `backend_riscv_prepared_edge_publication` baseline failure.
