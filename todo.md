Status: Active
Source Idea Path: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `va-arg-13.c` And Route The Next Boundary

# Current Packet

## Just Finished

Step 5 reran `tests/c/external/gcc_torture/src/va-arg-13.c` through the RV64
gcc-torture object runner after the `va_start` destination-address
materialization slice.

Result:

- The representative case still fails, but it advanced past the previous
  segmentation fault.
- Current runner result is `[RV64_BACKEND_RUNTIME_MISMATCH]` with
  `clang_exit=0` and `c4c_exit=Subprocess aborted`.
- This means the c4c object now links and runs into the testcase's `abort()`
  path instead of crashing while storing through an invalid `va_start`
  destination address.

Evidence:

- `build/agent_state/389_step5_va-arg-13.case.log` records the runtime
  mismatch and `Subprocess aborted`.
- `build/agent_state/389_step5_va-arg-13.objdump.log` shows the `test`
  function now materializes the `va_start` destination address before the
  store:
  `addi s1,sp,72`; `addi t1,sp,72`; `sd t1,0(s1)`.
- The same disassembly shows the later call setup still copies the `va_list`
  argument from the ordinary field/slot path, and `dummy` then loads the
  `va_list` overflow pointer and compares the first vararg against `1234`
  before branching to `abort`.
- Focused BIR for `dummy` contains the lowered `va_arg`-like sequence:
  load the `va_list` pointer from the incoming argument, update the incoming
  `va_list`, load the `int`, then compare against `1234`.

Next boundary:

- The current Step 5 boundary is no longer destination-address validity.
- The likely next owner is a separate `va_list` value publication/copy
  boundary: after `va_start` writes through the prepared destination address,
  the value used for the subsequent `dummy(statep->ap)` / `dummy(state.ap)`
  call argument still appears to come from the ordinary `state.ap` storage path
  rather than the newly initialized prepared `va_start` destination value.
- This should be handled as a follow-on lifecycle decision or split, not as
  more implementation inside this Step 5 evidence packet.

## Suggested Next

Ask the plan owner/supervisor to decide whether idea 389 should close as the
destination-address boundary fixed and split the remaining `va_list`
publication/copy runtime abort into a new idea, or extend the active lifecycle
with a new runbook step for that later boundary.

## Watchouts

- Do not treat the current abort as a regression of the `va_start`
  destination-address materialization; the representative case advanced past
  the previous segmentation fault.
- The next route should not broaden this packet into `va_arg` or same-module
  call fixes without lifecycle owner approval.
- Preserve the closed idea 386 frame-slot-address call-argument, idea 387 sret,
  and idea 388 `va_end` routes.

## Proof

Ran the supervisor-selected Step 5 representative rerun command:

```bash
mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 5 va-arg-13 RV64 object runner for idea 389'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/389_step5_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1
```

Result: completed the proof command; representative case returned
`case_exit=1` with the later runtime abort boundary described above.

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/389_step5_va-arg-13.case.log`
- `build/agent_state/389_step5_va-arg-13.objdump.log`
- `build/agent_state/389_step5_va_arg_13.prepared_bir.log`
- `build/agent_state/389_step5_va_arg_13.dummy.bir.log`
- `build/agent_state/389_step5_va_arg_13.dummy.prepared_bir.log`
