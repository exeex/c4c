Status: Active
Source Idea Path: ideas/open/642_rv64_global_residual_runtime_mismatch_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Residual Row

# Current Packet

## Just Finished

Completed plan Step 1, `Refresh The Residual Row`, for
`src/pr79737-2.c`.

Exact command:
`(cmake --build --preset default && ALLOWLIST=build/agent_state/642_step1_pr79737.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: build succeeded; one-row RV64 GCC torture backend refresh failed as the
current residual, with `total=1 passed=0 failed=1`.

Artifacts:
- case log:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/case.log`
- summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- failed list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- work directory:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c`
- object artifact:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/c4c.o`
- C4C linked binary:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/c4c.bin`
- clang control binary:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/clang.bin`

Current first observable failure boundary: C4C object compile, object ELF
sanity checks, clang control build/run, and C4C link all complete. The mismatch
is first observed when running the linked C4C binary under QEMU:
`clang_exit=0`, `c4c_exit=Subprocess aborted`; direct rerun showed
`clang_rc=0` and `c4c_rc=134`.

## Suggested Next

Start Step 2 owner tracing from the linked C4C binary runtime abort boundary,
using the current case log and emitted object/binary artifacts to identify the
first divergent global-data or bitfield-lowering fact before any implementation
packet is selected.

## Watchouts

- Do not implement code changes inside this research idea.
- Do not treat the runtime mismatch symptom alone as proof of true runtime
  support ownership.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime comparison behavior, or accounting.

## Proof

Proof log: `test_after.log`.

Command:
`(cmake --build --preset default && ALLOWLIST=build/agent_state/642_step1_pr79737.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

The proof is sufficient for this research refresh packet: it rebuilds the tree,
runs only the delegated one-row allowlist, preserves the row failure artifact,
and leaves the current residual as a runtime mismatch rather than a compile,
object, or link failure.
