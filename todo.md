Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Post-Repair Runtime State

# Current Packet

## Just Finished

Post-repair Step 1 captured fresh RV64 object/disassembly/runtime evidence for
`va-arg-13.c` after commit `9fb88adc`. The representative still fails with
`[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`.

The old caller-side `mv t1,s1` local-storage-address overwrite before the
`dummy` calls is gone. Fresh C4C disassembly shows both call-argument objects
are now populated from `t0`:

- first call: `sd t1,24(sp)` twice, then `a0 = sp + 24`, then `dummy`
- second call: `sd t1,32(sp)` twice, then `a0 = sp + 32`, then `dummy`

Runtime reaches the first `dummy` only. At first `dummy` entry, QEMU shows
`a0 = 0x0000ffff9eb4d7c8`, the first argument object address, with `t0/t1 =
0x0000ffff9eb4d860`. `dummy` executes `ld s2,0(a0)`, then `lw s1,0(s2)`;
the loaded pointer is `s2 = 0x0000ffff9eb4d860` and the loaded int is
`s1 = 0xffffffff9eb4d9f8`, not `1234`, so the first call branches to `abort`.

The second `dummy` call is not reached because the first call aborts. Static
object evidence shows the second call would pass `a0 = sp + 32` and populate
that argument object through the same `t0` route, not through `s1`.

Analysis log:
`build/agent_state/392_postrepair_step1_analysis.log`.

## Suggested Next

Execute Step 2 from `plan.md`: trace why the post-repair caller payload value
for both `dummy` call-argument objects is still the wrong pointer. Start from
the fresh disassembly/runtime logs and classify the prepared or RV64
materialization owner for the `t0` value that reaches the argument objects.

## Watchouts

- Keep the caller argument object address (`a0 = sp + 24` / `sp + 32`) separate
  from the object contents (`0x0000ffff9eb4d860` in the first observed call).
- The first call aborts before the second call executes, so second-call runtime
  state is not available from this QEMU run; only static object state was
  captured for the second call.
- The old `mv t1,s1` route is no longer the live boundary. The remaining issue
  is that `t0` still materializes a pointer that makes `dummy` load from
  `0x0000ffff9eb4d860` instead of the saved variadic argument value `1234`.
- Do not reopen idea 391 unless fresh evidence shows incoming variadic GPR
  payloads are no longer saved into the backing area.

## Proof

Delegated proof/evidence command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Post-repair Step 1 va-arg-13 RV64 object/runtime evidence for idea 392'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/392_postrepair_step1_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o > build/agent_state/392_postrepair_step1_va-arg-13.c4c-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin > build/agent_state/392_postrepair_step1_va-arg-13.clang-disasm.log 2>&1; timeout 20 qemu-riscv64 -L /usr/riscv64-linux-gnu -strace /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin > build/agent_state/392_postrepair_step1_va-arg-13.qemu-strace.log 2>&1 || true; timeout 20 qemu-riscv64 -d in_asm,cpu -D build/agent_state/392_postrepair_step1_va-arg-13.qemu-cpu.log -L /usr/riscv64-linux-gnu /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin >/dev/null 2>&1 || true; exit 0; } > test_after.log 2>&1`.

Result: evidence captured; representative still failed with `case_exit=1`,
`[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`. Logs:
`test_after.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.case.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.c4c-disasm.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.clang-disasm.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.qemu-strace.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.qemu-cpu.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.c4c-bin-disasm.log`,
`build/agent_state/392_postrepair_step1_va-arg-13.c4c-nm.log`, and
`build/agent_state/392_postrepair_step1_analysis.log`.
