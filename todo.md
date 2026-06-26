Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Same-Module Sret Evidence

# Current Packet

## Just Finished

Step 1 refreshed same-module sret evidence for
`tests/c/external/gcc_torture/src/920908-1.c`.

The BIR same-module sret fragment is still:
`bir.call void f(ptr sret(size=4, align=4) %t8, i32 2, i64 %t4, i64 %t7)`
in `main`. The BIR call-source comments record arg 0 as the frame-slot address
source for `%t8`, arg 1 as an immediate, and args 2/3 as register values
`%t4` and `%t7`.

Prepared facts still publish an explicit same-module sret call plan:
`wrapper_kind=same_module`, `callee=f`, `memory_return=%t8`,
`memory_encoding=frame_slot`, `sret_arg_index=0`, `memory_slot=#7`,
`memory_stack_offset=16`, `memory_size=4`, and `memory_align=4`.
The sret address source is arg 0 with
`arg.source_selection=local_frame_address_materialization`; `%t8` is held in
`s1`, and its frame-slot address materialization points at slot `#7`, stack
offset `16`. Ordinary arguments place immediate `2` in `a1`, `%t4` from `t0`
in `a2`, and `%t7` from `s2` in `a3`.

The refreshed object-route representative does not currently reach runtime.
The case exits `1` with `[RV64_C4C_OBJ_COMPILE_FAIL]` and the current rejection
point is:
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing`.

Classification: the old same-module `call_plan->memory_return` admission-gate
shape still exists in the prepared call plan, but it is not the first observed
boundary in this refreshed run. The current first boundary is local-memory
admission before the call can be emitted. Once that is cleared, the
`memory_return=%t8` same-module call remains the next sret owner to verify.

Detailed evidence:
`build/agent_state/387_step1_analysis.log`.

## Suggested Next

Execute Step 2 from `plan.md`: pin the local-memory admission failure for
`920908-1.c` before implementing any sret call emission repair. The next packet
should determine why the prepared local memory records for the initial `main`
stores fail RV64 object-route admission and whether that is inside idea 387 or
needs a narrow split before returning to the `memory_return` gate.

## Watchouts

- Do not hard-code `920908-1.c`, callee `f`, stack offsets, or the current
  diagnostic.
- Do not delete or weaken the `memory_return` gate until the earlier
  local-memory admission boundary is understood and the sret address placement
  is proven.
- The prepared call plan already carries explicit sret authority; the refreshed
  first failure is earlier than same-module call emission.

## Proof

Delegated evidence command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_920908-1.c && { echo 'Step 1 920908-1 RV64 same-module sret evidence for idea 387'; cmake --build --preset default --target c4cll >/dev/null; build/c4cll --target riscv64-linux-gnu --dump-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/387_step1_920908-1.bir.log 2>&1; echo "dump_bir_exit=$?"; build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/387_step1_920908-1.prepared.log 2>&1; echo "dump_prepared_exit=$?"; case_log=build/agent_state/387_step1_920908-1.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/920908-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1`.

Result: `dump_bir_exit=0`, `dump_prepared_exit=0`, `case_exit=1` with the
expected preserved failure evidence. Logs:
`test_after.log`,
`build/agent_state/387_step1_920908-1.bir.log`,
`build/agent_state/387_step1_920908-1.prepared.log`,
`build/agent_state/387_step1_920908-1.case.log`, and
`build/agent_state/387_step1_analysis.log`.
