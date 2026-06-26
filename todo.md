Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reprove And Decide Disposition

# Current Packet

## Just Finished

Post-repair Step 4 reproved the focused backend coverage and reran the RV64
object-route representative for idea 392.

Backend proof passed with the full `^backend_` subset: `test_after.log` reports
`100% tests passed, 0 tests failed out of 326`.

The `va-arg-13.c` representative also passed through the RV64 object runner:
`build/agent_state/392_postrepair_step4_va-arg-13.run.log` records
`case_exit=0` and
`[PASS][rv64-gcc-torture-backend-obj] /workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c`.

This satisfies the Step 4 disposition check for the idea 392 representative:
the prior abort boundary is gone, no later representative boundary appeared in
this packet, and the slice has lifecycle-ready completion evidence.

## Suggested Next

Supervisor or plan-owner disposition is the next coherent packet: review/commit
the completed slice, then decide whether idea 392 can close or needs a
runbook-level follow-up outside this representative route.

## Watchouts

- Step 4 did not modify implementation or tests; it only refreshed proof and
  representative evidence.
- The same-C4C `va-arg-13.c` representative passes. Any broader clang/RV64
  scalar-`va_list` ABI question should be handled as a separate lifecycle
  decision if still relevant.

## Proof

Delegated proof command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Post-repair Step 4 backend proof for idea 392'; cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R '^backend_'; echo 'Post-repair Step 4 va-arg-13 RV64 object runner for idea 392'; case_log=build/agent_state/392_postrepair_step4_va-arg-13.case.log; run_log=build/agent_state/392_postrepair_step4_va-arg-13.run.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; { echo "case_exit=$rc"; cat "$case_log"; } | tee "$run_log"; exit 0; } > test_after.log 2>&1`.

Result: passed. Logs:
`test_after.log`,
`build/agent_state/392_postrepair_step4_va-arg-13.case.log`, and
`build/agent_state/392_postrepair_step4_va-arg-13.run.log`.
