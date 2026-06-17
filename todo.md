Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Broader Backend Validation

# Current Packet

## Just Finished

Completed Step 8: Broader Backend Validation for the prepared RV64 emitter
decomposition. The narrow `backend_rv64_runtime` proof stayed green after each
extraction. Supervisor-side RISC-V focused validation preserved the known
`backend_riscv_prepared_edge_publication` baseline failure while the two
`backend_codegen_route_riscv64` route checks passed. Backend-wide validation
also preserved that same single known failure.

## Suggested Next

Suggested lifecycle follow-up: ask the plan owner to decide whether this
runbook can close with the documented `backend_riscv_prepared_edge_publication`
baseline failure, or whether a separate follow-up idea should own that
pre-existing edge-publication test repair.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- The decomposition stayed behavior-preserving; no tests or expectations were
  weakened.
- `backend_riscv_prepared_edge_publication` was already the accepted full-suite
  baseline failure before Step 7 and remains the only backend-wide failure.
- `emit.cpp` now keeps public compatibility wrappers, the legacy direct-LIR
  tiny-add fallback, `PhysReg` compatibility helpers, and assembler glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`, rolled forward to `test_before.log`.

Broader validation:
`ctest --test-dir build -j --output-on-failure -R 'backend_riscv|backend_codegen_route_riscv64'`

Result: 2/3 passed; the only failure was the known
`backend_riscv_prepared_edge_publication` baseline failure.

Backend-wide validation:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: 202/203 passed; the only failure was the known
`backend_riscv_prepared_edge_publication` baseline failure.
