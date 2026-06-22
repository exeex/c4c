Status: Active
Source Idea Path: ideas/open/309_rv64_aggregate_global_storage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Aggregate Global Storage Baseline and Scope

# Current Packet

## Just Finished

Step 1 established the RV64 aggregate global storage baseline for
`tests/c/external/c-testsuite/src/00024.c` without implementation or test edits.
The prepared BIR reaches `main` with a zero-frame function, two aggregate-field
global stores, two aggregate-field global loads, and prepared addressing entries
for `base=global_symbol`, constant byte offsets, `size=4`, `align=4`, and
`range_verdict=proven_in_bounds`. RV64 asm emission currently exits `1` with:
`error: riscv prepared module emitter does not support this prepared global
storage layout`. No assembly file is emitted.

## Suggested Next

Delegate the first implementation packet for the generic RV64 backend contract:
emit defined zero-initialized aggregate globals plus direct integer
`load_global`/`store_global` accesses through `global_symbol` prepared
addressing with constant in-bounds subobject offsets. The focused backend test
shape should be a small internal C case with a file-scope aggregate of integer
fields, stores to distinct fields, loads them back, and returns an arithmetic
check; assertions should prove the asm route succeeds and emits a real global
storage definition plus symbol-relative RV64 load/store code, without depending
on the external testcase name, symbol spelling, or specific literal offsets.

## Watchouts

- Do not repair the shared `.text`-only output contract in this plan.
- Do not special-case `src/00024.c`, symbol `v`, or fixed offsets `0` and `4`.
- Keep pointer globals, floating globals, strings, and external calls outside
  the acceptance claim unless the supervisor opens a separate scope.
- The current baseline shows the semantic/prepared BIR path is already carrying
  the aggregate field accesses; the first backend target is the RV64 prepared
  module/global-storage emission contract, not frontend lowering.
- The delegated CTest subset is not green before implementation work:
  `backend_riscv_prepared_edge_publication` failed while the other 34 selected
  RV64/RISC-V backend tests passed.

## Proof

Ran the delegated proof command exactly. Build was up to date; baseline
artifacts were written under `build/rv64_aggregate_global_baseline/`, including
`summary.md`, `00024.prepared_bir.txt`, `00024.emit.err`, and
`00024.emit.status`. `summary.md` captures the prepared BIR global/access facts,
`emit_exit=1`, the unsupported global-storage-layout diagnostic, and that no
assembly file exists.

Canonical proof log: `test_after.log`. The full delegated command exited
nonzero because `ctest --test-dir build -j --output-on-failure -R
'backend_.*(rv64|riscv).*'` failed 1 of 35 selected tests:
`backend_riscv_prepared_edge_publication`.
