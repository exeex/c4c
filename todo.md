Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Representative Executable Progress

# Current Packet

## Just Finished

Step 5 repaired the `src/20000113-1.c` RV64 object-route runtime mismatch
without touching the separate `src/20030216-1.c` compile-time blocker. The
mismatch was a prepared join-transfer select carrier being recomputed in
`logic.end.40` after `%t43` reused `%t33`'s `s1` home; the recomputed select
condition read the newer bitfield value instead of the source-branch value and
sent the c4c binary to `abort`.

RV64 object emission now treats a prepared join-transfer select carrier as an
already-materialized carrier when all of its edge transfers have published
parallel-copy bundles in the shared traversal stream. Ordinary selects and
carrier fixtures without published edge-copy bundles keep the existing scalar
select emission path.

The representative allowlist improved to 3 passed
(`src/20000113-1.c`, `src/20000205-1.c`, `src/20010119-1.c`) and 1 failed
(`src/20030216-1.c`). The remaining failed case still reports
`[RV64_C4C_OBJ_COMPILE_FAIL]` with `RISC-V backend object route unsupported
prepared module shape`.

## Suggested Next

Diagnose the remaining `src/20030216-1.c` unsupported prepared module shape as
a separate Step 5 packet, without folding it into the now-fixed
`src/20000113-1.c` runtime route.

## Watchouts

- `src/20000113-1.c` now passes the RV64 object-route allowlist proof.
- `src/20030216-1.c` remains a separate compile-time unsupported prepared
  module shape and was not diagnosed or claimed here.
- Keep the route shared-traversal-first; do not scan
  `control_flow.parallel_copy_bundles`, do not use
  `prepared_object_parallel_copy_event_kind` from RV64, and do not add
  filename/test-name shortcuts, expectation weakening, rendered-text probes, or
  target-local CFG reconstruction.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true`

Result: build completed, `backend_riscv_object_emission` passed, and the
allowlist is now 3/4 passed and 1/4 failed. `src/20000113-1.c` passes.
`src/20030216-1.c` still fails at compile time with `RISC-V backend object route
unsupported prepared module shape`. Proof log: `test_after.log`.

Additional focused validation before the full allowlist proof:
`CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-20000113-only.txt VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
passed `src/20000113-1.c` in isolation.
