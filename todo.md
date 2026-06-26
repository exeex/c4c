Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `920908-1.c` And Route The Next Boundary

# Current Packet

## Just Finished

Step 5 reran the `920908-1.c` RV64 object-route representative after the
same-module sret call support.

Representative result:
`build/agent_state/387_step5_920908-1.run.log` and
`build/agent_state/387_step5_920908-1.case.log` report
`case_exit=1` with `[RV64_BACKEND_RUNTIME_MISMATCH]`: `clang_exit=0`,
`c4c_exit=Subprocess aborted`.

The prior same-module sret object-emission boundary is gone. Fresh c4c
disassembly shows `main` now emits the expected sret call sequence:
`addi a0,sp,16`, `li a1,2`, `mv a2,t0`, `mv a3,s2`, followed by
`R_RISCV_CALL_PLT f`. The qemu CPU log confirms entry to `f` with caller
publication `a2=10` and `a3=20`.

The remaining boundary is callee-side variadic aggregate consumption. In c4c
`f`, the first aggregate `va_arg` reads the 32-bit payload at the start of the
first 8-byte variadic GPR save slot and gets `10`, so the first check passes.
The cursor then advances by 4 bytes (`addi t1,t1,4`) instead of advancing to
the next 8-byte variadic register slot. The second aggregate `va_arg` therefore
reads the high half of the first slot at `sp+60`, gets `0`, and branches to the
second `abort` while expecting `20`.

Concrete analysis is recorded in:
`build/agent_state/387_step5_analysis.log`.

## Suggested Next

Route the next packet to the RV64 variadic aggregate `va_arg` consumption
stride/layout boundary. The minimal repair target is the callee-side aggregate
`va_arg` cursor advance for register-passed variadic aggregate payloads: it
must move across ABI variadic GPR save-area slots for this shape, not only by
the aggregate object size.

## Watchouts

- Do not reopen the same-module sret call emission route without contradictory
  evidence; Step 5 proves the representative now reaches runtime with the sret
  address and ordinary argument registers materialized.
- Caller publication is not the remaining owner for this representative:
  qemu shows `a2=10` and `a3=20` at `f` entry.
- Compare any future repair against clang's callee behavior: clang advances
  the variadic cursor by 8 bytes between these register-saved aggregate
  payloads.

## Proof

Delegated proof/evidence command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_920908-1.c && { echo 'Step 5 920908-1 RV64 object runner for idea 387'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/387_step5_920908-1.case.log; run_log=build/agent_state/387_step5_920908-1.run.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/920908-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; { echo "case_exit=$rc"; cat "$case_log"; } | tee "$run_log"; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o > build/agent_state/387_step5_920908-1.c4c-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin > build/agent_state/387_step5_920908-1.clang-disasm.log 2>&1; timeout 20 qemu-riscv64 -d in_asm,cpu -D build/agent_state/387_step5_920908-1.qemu-cpu.log -L /usr/riscv64-linux-gnu /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin >/dev/null 2>&1 || true; exit 0; } > test_after.log 2>&1`.

Result: evidence captured; representative still fails at runtime with
`c4c_exit=Subprocess aborted`, which is the routed later boundary for the next
packet.

Evidence logs:
- `test_after.log`
- `build/agent_state/387_step5_920908-1.case.log`
- `build/agent_state/387_step5_920908-1.run.log`
- `build/agent_state/387_step5_920908-1.c4c-disasm.log`
- `build/agent_state/387_step5_920908-1.clang-disasm.log`
- `build/agent_state/387_step5_920908-1.qemu-cpu.log`
- `build/agent_state/387_step5_920908-1.c4c-bin-disasm.log`
- `build/agent_state/387_step5_analysis.log`
