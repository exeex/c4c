Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Focused RISC-V Validation

# Current Packet

## Just Finished

Completed plan.md Step 6 focused RISC-V validation. The delegated build plus
focused CTest command completed the build and ran three matching tests. The two
`backend_codegen_route_riscv64` tests passed, and the only failing test was the
documented accepted baseline failure:
`backend_riscv_prepared_edge_publication`.

## Suggested Next

Proceed to Step 7 backend-wide closure validation:
`ctest --test-dir build -R '^backend_' --output-on-failure`.

## Watchouts

The focused Step 6 proof did not expose any new RISC-V/backend-route failures.
The nonzero CTest exit status is attributable only to the plan-documented
`backend_riscv_prepared_edge_publication` baseline failure.

## Proof

Proof command run exactly as delegated, with combined output preserved in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_riscv|backend_codegen_route_riscv64'`

Result: blocked only by the documented accepted baseline failure. `ctest`
reported 2/3 tests passed, with the sole failure
`backend_riscv_prepared_edge_publication`.
