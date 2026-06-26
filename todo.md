Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reprove Focused Coverage And Representative

# Current Packet

## Just Finished

Continuation Step 4 reproved the focused backend coverage and reran the
`va-arg-13.c` RV64 object-route representative after the Step 3 repair. The
backend subset passed: `ctest --test-dir build -j --output-on-failure -R
'^backend_'` reported `100% tests passed, 0 tests failed out of 326`.

The representative did not advance past the prior abort boundary. The case
runner compiled the C4C object, validated the ELF header, built and ran the
clang reference with `clang_exit=0`, linked the C4C object into `c4c.bin`, then
reported `[RV64_BACKEND_RUNTIME_MISMATCH]` with `c4c_exit=Subprocess aborted`
and empty stdout/stderr. Evidence:
`build/agent_state/392_cont_step4_va-arg-13.case.log`; combined proof log:
`test_after.log`.

## Suggested Next

Trace the remaining `va-arg-13.c` runtime abort boundary in the emitted RV64
object route. The next packet should start from the Step 4 case log and classify
which emitted object/ABI state still differs, without changing prepared fact
schema or broadening beyond the representative route.

## Watchouts

- Step 4 was proof/evidence only; no implementation or tests were changed.
- The focused backend contract remains green, including the Step 3 load-local
  payload route and its fail-closed equality guard.
- The representative abort is now a runtime mismatch after successful C4C object
  compile and link, not an object-production failure. The prior
  `Subprocess aborted` boundary remains present.

## Proof

Delegated proof run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 4 backend proof for idea 392'; cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R '^backend_'; echo 'Step 4 va-arg-13 RV64 object runner for idea 392'; case_log=build/agent_state/392_cont_step4_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1`.

Result: backend proof passed; representative case failed with
`case_exit=1`, `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`. Proof log: `test_after.log`. Case log:
`build/agent_state/392_cont_step4_va-arg-13.case.log`.
