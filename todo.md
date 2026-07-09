Status: Active
Source Idea Path: ideas/open/619_bir_aggregate_global_store_handoff.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Residual Owners And Close Readiness

# Current Packet

## Just Finished

Step 4 - Classify Residual Owners And Close Readiness completed as a no-code
classification packet after the Step 3 aggregate-global semantic repair.

Fresh targeted probes were written under
`build/agent_state/619_step4_residual_classification/`:

| Row | `--dump-bir` | `--dump-prepared-bir` | Object/runtime probe | Residual owner |
| --- | --- | --- | --- | --- |
| `src/pr22141-1.c` | rc `0` | rc `0` | rc `1`, `RV64_BACKEND_RUNTIME_MISMATCH`, QEMU SIGSEGV at address `0x1` | RV64 consumer / ABI-runtime runtime mismatch after prepared aggregate/global facts are present. |
| `src/compndlit-1.c` | rc `0` | rc `0` | rc `0`, `[PASS][rv64-gcc-torture-backend-obj]` | none; row is no longer residual for idea 619. |
| `src/pr57344-1.c` | rc `0` | rc `0` | rc `0`, `[PASS][rv64-gcc-torture-backend-obj]` | none; row is no longer residual for idea 619. |
| `src/pr39120.c` | rc `0` | rc `0` | rc `1`, `RV64_BACKEND_RUNTIME_MISMATCH`, QEMU SIGSEGV at `NULL` | RV64 consumer runtime mismatch after prepared `store_global @x` handoff; prepared facts show `store_source ... intent=store_global_publication` and `access ... base=global_symbol stored=x.aggregate.copy.0`. |
| `src/ieee/20001122-1.c` | rc `0` | rc `0` | rc `1`, `unsupported_global_data` | prepared global-data/RV64 global memory access-width support; not aggregate-global semantic handoff. |
| `src/991030-1.c` | rc `0` | rc `0` | rc `0`, `[PASS][rv64-gcc-torture-backend-obj]` | none; current targeted object/runtime probe passes. |

No row remains blocked by the same aggregate/global handoff stop. The four
original producer/handoff rows (`pr22141-1.c`, `compndlit-1.c`,
`pr57344-1.c`, and `pr39120.c`) all reach semantic and prepared BIR now, and
the two split-in rows also dump semantic/prepared BIR cleanly. No residual row
is classified as terminator, global initializer bootstrap, or unresolved for
this packet.

No expectation files, unsupported markers, allowlists, timeout/accounting
policy, implementation files, or tests were changed for Step 4.

## Suggested Next

Ask the plan owner to decide lifecycle closure for idea 619. Based on this
packet, the idea appears close-ready: the aggregate/global handoff producer
stop is repaired, and remaining failures are already assignable to downstream
RV64 consumer/ABI-runtime or prepared global-data ownership.

## Watchouts

- `src/pr22141-1.c` and `src/pr39120.c` still fail at runtime, but only after
  object compilation/linking succeeds; do not count those as 619 producer or
  prepared-handoff misses.
- `src/ieee/20001122-1.c` is a prepared global-data/global memory access-width
  rejection: `unsupported_global_data: RV64 object route supports only 1-, 2-,
  4-, and 8-byte prepared global memory accesses`.
- Existing full-scan summaries may still contain stale pre-Step-3 semantic
  failures for the original rows; use the Step 4 targeted artifacts when
  judging close readiness.

## Proof

Focused diagnostic probes, no root proof log:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu <case>`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu <case>`
- `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=clang -DQEMU_RISCV64=qemu-riscv64 -DSRC=<case> -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=<artifact> -DOUT_OBJECT=<artifact> -DOUT_C4C_BIN=<artifact> -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

Result: semantic/prepared BIR rc `0` for all six named rows; object/runtime
passes for `compndlit-1.c`, `pr57344-1.c`, and `991030-1.c`; downstream
failures classified above for `pr22141-1.c`, `pr39120.c`, and
`ieee/20001122-1.c`.

No build/regression command was required for this no-code packet, and no
`test_after.log` was produced.
