Status: Active
Source Idea Path: ideas/open/309_rv64_aggregate_global_storage.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Add Focused Backend Coverage for Aggregate Data Symbols; Emit Simple Aggregate and Array Global Storage

# Current Packet

## Just Finished

Steps 2-3 added focused RV64 backend route coverage for simple zero-initialized
aggregate/array global storage and taught prepared RV64 global storage emission
to accept defined globals with immediate-zero initializer elements and a known
byte size. The new test
`backend_codegen_route_riscv64_zero_aggregate_global_storage` proves `.bss`
symbols for a zero struct and zero integer array using `.zero 8` and `.zero 12`
without exercising aggregate field offsets.

## Suggested Next

Delegate Step 4 for prepared aggregate/global-symbol offset load/store emission
and instruction-body proof. Use a focused RV64 backend route case with a small
integer aggregate, stores to distinct fields, loads them back, and checks the
result, without depending on `src/00024.c`, symbol `v`, or fixed source text.

## Watchouts

- The storage-only case passes and does not require offset access lowering.
- A spot check of `tests/c/external/c-testsuite/src/00024.c` now emits the
  `.bss` storage symbol for `v`, but the asm body still stops at `main:` /
  `.Lmain_entry:`; treat body/load/store emission as Step 4 work.
- Pointer globals, floating globals, strings, external calls, nonzero aggregate
  data emission, and expectation weakening remain outside this packet.
- The selected RV64/RISC-V subset still includes the known pre-existing failure
  `backend_riscv_prepared_edge_publication`.

## Proof

Ran the delegated proof command exactly:
`cmake --build build --target c4c_backend_tests c4cll -j 2 > /tmp/c4c_step23_agg_build.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*' > test_after.log 2>&1`.

Build succeeded. CTest selected 36 tests; 35 passed, including the new
`backend_codegen_route_riscv64_zero_aggregate_global_storage` case and the
existing scalar i32 global, string literal, and no-storage RISC-V route tests.
The command exited nonzero only because known pre-existing
`backend_riscv_prepared_edge_publication` failed. Canonical proof log:
`test_after.log`.
