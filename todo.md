Status: Active
Source Idea Path: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Aggregate Va Arg Cursor Evidence

# Current Packet

## Just Finished

Step 1 captured fresh BIR, prepared, RV64 object, clang comparison, and qemu
CPU evidence for `tests/c/external/gcc_torture/src/920908-1.c`, focused on
callee `f` aggregate `va_arg` cursor behavior.

The representative still fails at runtime:
`build/agent_state/393_step1_920908-1.run.log` reports `case_exit=1` with
`[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`.

The evidence maps both aggregate `va_arg` operations:
- BIR has two `llvm.va_arg.aggregate` calls in `f`, both for
  `sret(size=4, align=4)` aggregate payloads.
- Prepared variadic helper operands for both calls use
  `source_class=overflow_arg_area`, `payload_size=4`, `copy_size=4`,
  `progression_stride=4`, and `overflow_stride=4`.
- The same prepared variadic entry plan publishes incoming RV64 variadic GPR
  save-area slots at 8-byte stride: `a2` at stack offset 56 and `a3` at stack
  offset 64, each `size=8 align=8`.
- c4c object emission follows the prepared stride: first `va_arg` reads at
  `sp+56` and advances to `sp+60`; second `va_arg` reads `sp+60`, the high
  half of the first 8-byte GPR save-area slot, instead of reading the next slot
  at `sp+64`.
- qemu confirms caller publication is not the owner: `f` is entered with
  `a2=10` and `a3=20`; the second aggregate read compares `0` against `20` and
  branches to `abort`.
- clang's callee advances the cursor by 8 bytes between these register-saved
  aggregate payloads while still loading the 4-byte aggregate payload.

Detailed evidence and line-oriented references are in:
`build/agent_state/393_step1_analysis.log`.

## Suggested Next

Execute Step 2 against prepared aggregate `va_arg` access-plan construction.
The minimal target is the prepared stride fact: preserve `payload_size=4` and
`copy_size=4`, but publish/use an RV64 variadic GPR save-area slot cursor
stride of 8 for this register-save-area-backed aggregate shape.

## Watchouts

- Do not change caller publication for this case; qemu shows the correct
  incoming payload values in `a2` and `a3`.
- Do not conflate payload copy size with cursor progression stride. The object
  payload is 4 bytes, but the ABI save-area slot step is 8 bytes.
- Object emission is currently following prepared facts, so the primary owner
  is prepared variadic aggregate access-plan metadata unless Step 2 finds a
  contradictory implementation guard.
- Do not hard-code `920908-1.c`, callee `f`, concrete registers, stack offsets,
  or the abort branch.

## Proof

Evidence proof command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_920908-1.c; { echo 'Step 1 920908-1 RV64 aggregate va_arg cursor evidence for idea 393'; cmake --build --preset default --target c4cll >/dev/null; build/c4cll --target riscv64-linux-gnu --dump-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/393_step1_920908-1.bir.log 2>&1; echo "dump_bir_exit=$?"; build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/393_step1_920908-1.prepared.log 2>&1; echo "dump_prepared_exit=$?"; case_log=build/agent_state/393_step1_920908-1.case.log; run_log=build/agent_state/393_step1_920908-1.run.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/920908-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; { echo "case_exit=$rc"; cat "$case_log"; } | tee "$run_log"; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o > build/agent_state/393_step1_920908-1.c4c-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin > build/agent_state/393_step1_920908-1.clang-disasm.log 2>&1; timeout 20 qemu-riscv64 -d in_asm,cpu -D build/agent_state/393_step1_920908-1.qemu-cpu.log -L /usr/riscv64-linux-gnu /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin >/dev/null 2>&1 || true; exit 0; } > test_after.log 2>&1`.

Result: evidence captured successfully. `dump_bir_exit=0`,
`dump_prepared_exit=0`, and the representative runtime mismatch is preserved
for the routed Step 2 boundary.

Evidence logs:
- `test_after.log`
- `build/agent_state/393_step1_920908-1.bir.log`
- `build/agent_state/393_step1_920908-1.prepared.log`
- `build/agent_state/393_step1_920908-1.case.log`
- `build/agent_state/393_step1_920908-1.run.log`
- `build/agent_state/393_step1_920908-1.c4c-disasm.log`
- `build/agent_state/393_step1_920908-1.clang-disasm.log`
- `build/agent_state/393_step1_920908-1.qemu-cpu.log`
- `build/agent_state/393_step1_920908-1.c4c-bin-disasm.log`
- `build/agent_state/393_step1_analysis.log`
