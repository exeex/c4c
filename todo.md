Status: Active
Source Idea Path: ideas/open/309_rv64_aggregate_global_storage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Prepared Global-Symbol Offset Loads and Stores

# Current Packet

## Just Finished

Step 4 taught prepared RV64 global-symbol i32 load/store emission to accept
zero-initialized aggregate storage as a valid backing object for constant
byte-offset field accesses while preserving the existing prepared-access checks
for default address space, nonvolatile access, 4-byte size, alignment, and
12-bit offsets. Added
`backend_codegen_route_riscv64_aggregate_global_field_access`, which stores two
integer fields of a simple aggregate global and loads both fields back through
prepared `global_symbol` offsets.

## Suggested Next

Step 5 representative recheck is next: rerun the selected external/sample route
cases now that `tests/c/external/c-testsuite/src/00024.c` emits a real function
body instead of stopping after the entry label.

## Watchouts

- `tests/c/external/c-testsuite/src/00024.c` now emits `.bss` storage plus
  `lla`/`sw`/`lw` body code for offsets 0 and 4 in a spot check.
- Direct pointer-style global address materialization remains on the existing
  scalar-defined-global predicate; this packet did not add pointer global,
  floating global, string, external-call, or nonzero aggregate initializer
  behavior.
- The selected RV64/RISC-V subset still includes the known pre-existing
  `backend_riscv_prepared_edge_publication` failure.

## Proof

Ran the delegated proof command exactly:
`cmake --build build --target c4c_backend_tests c4cll -j 2 > /tmp/c4c_step4_agg_build.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*' > test_after.log 2>&1`.

Build succeeded. CTest selected 37 tests; 36 passed, including
`backend_codegen_route_riscv64_aggregate_global_field_access`,
`backend_codegen_route_riscv64_zero_aggregate_global_storage`, and the existing
RV64 global load/store runtime cases. The command exited nonzero only because
known pre-existing `backend_riscv_prepared_edge_publication` failed. Canonical
proof log: `test_after.log`.
