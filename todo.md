Status: Active
Source Idea Path: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun 920908-1.c And Route The Next Boundary

# Current Packet

## Just Finished

Step 5 reran `tests/c/external/gcc_torture/src/920908-1.c` through the RV64
object representative after the aggregate `va_arg` stride repair.

Result:
- The prior aggregate `va_arg` abort boundary is gone. The prepared dump now
  has both aggregate helpers using `payload_size=4`, `copy_size=4`,
  `source_slot=8`, `progression_stride=8`, and `overflow_stride=8`.
- The C4C linked disassembly advances the `overflow_arg_area` cursor by 8 bytes
  after each 4-byte aggregate copy.
- The QEMU trace shows the first aggregate comparison sees `10`, the second
  aggregate comparison sees `20`, and both abort branches are skipped.
- The representative still fails, but now as `c4c_exit=Segmentation fault`
  after both aggregate checks pass.

Later boundary:
- The remaining fault is in the callee return store for stack-homed
  `%ret.sret`, not in aggregate `va_arg` cursor stride.
- `main` passes the same-module sret address in `a0`, but `f` later emits
  `ld t2,0(sp); sw t1,0(t2)` for the `%ret.sret` pointer-value store without
  a prior callee-side publication of `a0` into the stack-homed `%ret.sret`
  slot.
- Evidence and classification are recorded in
  `build/agent_state/393_step5_analysis.log`.

## Suggested Next

Hand idea 393 to the plan owner for completion/closure review. If the
supervisor wants to pursue the remaining representative failure, route it as a
separate same-module sret/callee `%ret.sret` home-publication boundary rather
than expanding idea 393.

## Watchouts

- `test_after.log` is representative/evidence output for Step 5, not a backend
  CTest regression log.
- Step 5 confirms idea 393's owned cursor-stride behavior on the representative:
  the second aggregate `va_arg` reads `20`.
- The remaining segmentation fault is likely owned by the same-module
  sret/callee `sret_param` home-publication route, which should stay split from
  aggregate `va_arg` stride.

## Proof

Delegated representative/evidence command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_920908-1.c && { echo 'Step 5 920908-1 RV64 aggregate va_arg post-repair representative for idea 393'; cmake --build --preset default --target c4cll >/dev/null; build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/393_step5_920908-1.prepared.log 2>&1; echo "dump_prepared_exit=$?"; build/c4cll --target riscv64-linux-gnu --dump-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/393_step5_920908-1.bir.log 2>&1; echo "dump_bir_exit=$?"; case_log=build/agent_state/393_step5_920908-1.case.log; run_log=build/agent_state/393_step5_920908-1.run.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/920908-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; { echo "case_exit=$rc"; cat "$case_log"; } | tee "$run_log"; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o > build/agent_state/393_step5_920908-1.c4c-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin > build/agent_state/393_step5_920908-1.c4c-bin-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin > build/agent_state/393_step5_920908-1.clang-disasm.log 2>&1; timeout 20 qemu-riscv64 -d in_asm,cpu -D build/agent_state/393_step5_920908-1.qemu-cpu.log -L /usr/riscv64-linux-gnu /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin >/dev/null 2>&1 || true; exit 0; } > test_after.log 2>&1`

Result: representative runner reports `case_exit=1` because clang exits `0`
while C4C exits with `Segmentation fault`. Evidence shows the prior aggregate
abort boundary is gone and the remaining failure is a later sret home-publication
boundary. Logs:
- `test_after.log`
- `build/agent_state/393_step5_920908-1.prepared.log`
- `build/agent_state/393_step5_920908-1.bir.log`
- `build/agent_state/393_step5_920908-1.case.log`
- `build/agent_state/393_step5_920908-1.run.log`
- `build/agent_state/393_step5_920908-1.c4c-disasm.log`
- `build/agent_state/393_step5_920908-1.c4c-bin-disasm.log`
- `build/agent_state/393_step5_920908-1.clang-disasm.log`
- `build/agent_state/393_step5_920908-1.qemu-cpu.log`
- `build/agent_state/393_step5_analysis.log`
