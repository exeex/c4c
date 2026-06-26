Status: Active
Source Idea Path: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Sret Param Home Evidence

# Current Packet

## Just Finished

Step 1 captured fresh prepared/BIR/object/qemu evidence for
`tests/c/external/gcc_torture/src/920908-1.c`, focused on callee `f`
`%ret.sret`.

Findings:
- Caller-side same-module sret address construction is correct in the
  representative. The prepared call plan has `memory_return=%t8`,
  `memory_slot=#7`, `memory_stack_offset=16`, and arg 0 source selection
  `local_frame_address_materialization`; linked disassembly materializes
  `a0=sp+16` before calling `f`.
- QEMU confirms `f` receives the caller sret address in incoming `a0`.
- Prepared state gives callee `%ret.sret` a stack home:
  `source_kind=sret_param`, slot `#0`, `stack_offset=0`,
  `requires_home_slot=yes`, `permanent_home_slot=yes`.
- The final return store is prepared as `base=pointer_value` through
  `pointer=%ret.sret`, and object emission lowers it as
  `ld t2,0(sp); sw t1,0(t2)`.
- The callee prologue saves variadic `a2`-`a7` and initializes the `va_list`
  cursor, but does not publish/save incoming `a0` into the `%ret.sret` home at
  `0(sp)`.

Conclusion: the fault comes from an uninitialized callee `%ret.sret` stack
home, not from caller-side sret address construction. The exact fact gap and
evidence are recorded in `build/agent_state/394_step1_analysis.log`.

## Suggested Next

Execute Step 2 by selecting the narrow route for callee-entry publication of
stack-homed RV64 `sret_param` pointer values. Start from the prepared
`sret_param` object/home facts and the RV64 object-emission prologue path, and
decide whether the publication should be explicit prepared metadata or a
strict object-emission entry rule.

## Watchouts

- Keep the route semantic: incoming RV64 sret pointer argument to a callee
  `sret_param` object with an explicit stack home. Do not hard-code
  `920908-1.c`, callee `f`, stack offsets, or the final store sequence.
- Object emission already supports the final pointer-value local-memory store
  shape once `%ret.sret` home contains the incoming pointer.
- The representative still exits with a segmentation fault because Step 1 is an
  evidence packet only; no implementation was changed.

## Proof

Delegated representative/evidence command run:
`mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_920908-1.c && { echo 'Step 1 920908-1 RV64 sret callee home evidence for idea 394'; cmake --build --preset default --target c4cll >/dev/null; build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/394_step1_920908-1.prepared.log 2>&1; echo "dump_prepared_exit=$?"; build/c4cll --target riscv64-linux-gnu --dump-bir tests/c/external/gcc_torture/src/920908-1.c > build/agent_state/394_step1_920908-1.bir.log 2>&1; echo "dump_bir_exit=$?"; case_log=build/agent_state/394_step1_920908-1.case.log; run_log=build/agent_state/394_step1_920908-1.run.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/920908-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; { echo "case_exit=$rc"; cat "$case_log"; } | tee "$run_log"; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.o > build/agent_state/394_step1_920908-1.c4c-disasm.log 2>&1; riscv64-linux-gnu-objdump -dr /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin > build/agent_state/394_step1_920908-1.c4c-bin-disasm.log 2>&1; timeout 20 qemu-riscv64 -d in_asm,cpu -D build/agent_state/394_step1_920908-1.qemu-cpu.log -L /usr/riscv64-linux-gnu /workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920908-1.c/c4c.bin >/dev/null 2>&1 || true; exit 0; } > test_after.log 2>&1`

Result: evidence captured. `dump_prepared_exit=0`, `dump_bir_exit=0`, and the
representative still reports `case_exit=1` because clang exits `0` while C4C
exits with `Segmentation fault`, as expected for this evidence-only packet.
Logs:
- `test_after.log`
- `build/agent_state/394_step1_analysis.log`
- `build/agent_state/394_step1_920908-1.prepared.log`
- `build/agent_state/394_step1_920908-1.bir.log`
- `build/agent_state/394_step1_920908-1.case.log`
- `build/agent_state/394_step1_920908-1.run.log`
- `build/agent_state/394_step1_920908-1.c4c-disasm.log`
- `build/agent_state/394_step1_920908-1.c4c-bin-disasm.log`
- `build/agent_state/394_step1_920908-1.qemu-cpu.log`
