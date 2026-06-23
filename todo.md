Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Iterate Or Split Remaining Candidate Buckets

# Current Packet

## Just Finished

Added focused expected-repair backend coverage for the remaining idea 316
in-scope residual: pointer-typed select materialization/publication into a
local pointer slot. The new case is
`tests/backend/case/riscv64_pointer_typed_select_publication.c`; it is smaller
than `src/00144.c` and distinct from the existing
`riscv64_pointer_integer_select_chain.c` fixture because the selected value is
itself `ptr` typed rather than an I32 select followed by `inttoptr`.

New expected-repair tests:

- `backend_dump_riscv64_pointer_typed_select_publication`
- `backend_codegen_route_riscv64_pointer_typed_select_publication_expected_repair`

The dump test asserts the prepared `ptr` select, local pointer publication, and
available select/store-source records. The codegen route test asserts the
current RV64 truncation after `.Lmain_tern_end_6` with no `ret`. No runtime
expected-repair test was added because the RV64 runtime harness only supports
expected run-code checks and does not provide a green expected-crash/truncated
assembly mode.

## Suggested Next

Repair pointer-typed select materialization/publication for RV64 prepared
emission, then convert the new expected-repair coverage to positive semantic
dump/codegen/runtime contracts if qemu execution is supported after repair.

## Watchouts

- Do not weaken the existing positive `riscv64_pointer_integer_select_chain`
  tests; this new fixture covers a different `ptr` select boundary.
- The expected-repair codegen test intentionally proves current truncation
  without naming `src/00144.c` or depending on one stack offset.
- `src/00077.c` and `src/00143.c` remain classified as separate stack-homed
  compare/control-flow residuals from the prior Step 4 reprobe.

## Proof

Ran focused proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route)_riscv64_pointer_typed_select_publication'`,
which passed 2/2 tests.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The new expected-repair
tests passed in the full backend run. The backend subset still did not pass
because `backend_riscv_prepared_edge_publication` failed with "RISC-V prepared
module should emit a register edge move"; `test_after.log` is the proof log.
