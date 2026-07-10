Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Publish The `%t23` Compare Pointer Source Chain

# Current Packet

## Just Finished

Step 4A, `Publish The %t23 Compare Pointer Source Chain`, now has fresh
focused integration/runtime evidence for the `src/loop-2e.c` `%t23` path after
the clobber-safety fix.

- Semantic and prepared BIR both contain `%t23 = bir.add ptr %t21, 156`
  followed by `%t24 = bir.ne ptr %t20, %t23`.
- Prepared output records `%t23` as value id `26` in `slot #50+stack368`, with
  RHS `branch_stack_load_authority` `status=available` and
  `pointer_status=proven`.
- RV64 object emission exits 0; disassembly shows the selected branch
  materializes the RHS pointer with `ld` from `360(sp)`, `addi ..., 156`,
  stores it to `368(sp)`, reloads it with `ld`, and branches on that value.
- The one-case RV64 GCC torture object runtime compare still fails with
  `clang_exit=0` and `c4c_exit=Subprocess aborted`.
- The runtime abort is downstream of Step 4A: callee `f` updates the local
  stack home for `q` at `0(sp)` instead of storing the computed pointer through
  the caller-provided `*q++` destination, so `main` reloads unchanged `q[39]`
  from `312(sp)` before comparing it with the now-materialized `%t23`.
- Evidence is in
  `build/agent_state/653_step4a_loop_t23_runtime_probe/summary.md`.

## Suggested Next

Treat Step 4A as complete for the `%t23` pointer-source
publication/materialization boundary and route the next packet or lifecycle
decision to the downstream callee indirect-store / `*q++` writeback owner if
that owner is in scope for the current idea.

## Watchouts

- Do not chase the old `%t23` value id `27` / `slot #46+stack336` preservation
  shape as if it were still current; the semantic producer changes value
  numbering and moves `%t23` to value id `26` / `slot #50+stack368`.
- Do not treat the remaining failure as a missing semantic producer:
  `%t23 = bir.add ptr %t21, 156` is present in semantic and prepared BIR.
- Do not re-open the `%t23` clobber-safety owner without checking the current
  prepared authority row first; it is now `status=available` for the refreshed
  `%t23` value id `26` / `slot #50+stack368` shape.
- Do not classify the fresh `loop-2e.c` runtime abort as a pointer-source
  authority failure; object emission and branch materialization now advance.
- Do not broaden Step 4A into the parked `%t6` runtime owner; that owner is
  downstream of the pointer-source publication/materialization boundary.
- Do not weaken expectation files, unsupported markers, allowlists, or runtime
  accounting to claim progress.

## Proof

`test_after.log`: `{ cmake --build --preset default && ctest --test-dir build
-j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365, matching `test_before.log` by failed test name. No new
backend failure names were introduced. `test_after.log` is the preserved proof
log.

Focused proof:

- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/loop-2e.c` exits 0 and writes
  `build/agent_state/653_step4a_loop_t23_runtime_probe/loop-2e.bir.txt`.
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/loop-2e.c` exits 0 and writes
  `build/agent_state/653_step4a_loop_t23_runtime_probe/loop-2e.prepared.txt`.
- `build/c4cll --target riscv64-linux-gnu --codegen obj
  tests/c/external/gcc_torture/src/loop-2e.c -o
  build/agent_state/653_step4a_loop_t23_runtime_probe/loop-2e.o` exits 0.
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh` with a one-case
  `src/loop-2e.c` allowlist fails only at runtime compare:
  `clang_exit=0`, `c4c_exit=Subprocess aborted`; copied case log is
  `build/agent_state/653_step4a_loop_t23_runtime_probe/loop-2e.runtime.case.log`.
