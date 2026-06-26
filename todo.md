Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure or Handoff Check

# Current Packet

## Just Finished

Step 4 classified the remaining RV64 GCC torture backend object-route failure
for `src/20000112-1.c` without implementing a new repair. The c4c binary still
segfaults under `qemu-riscv64 -L /usr/riscv64-linux-gnu`, while the clang
reference exits 0, but the concrete failure remains in the call-argument
reload/prior-preservation owner family: after `strchr(fmt, '*')` returns
`NULL` for `"ee"`, c4c enters the next `strchr` call with `a0 == NULL` instead
of reloading or preserving the original `fmt` pointer for `strchr(fmt, 'V')`.
The observed crash is `SIGSEGV si_addr=NULL` in the libc `strchr` path.
Evidence is summarized in
`build/agent_state/380_step4_20000112.classification.txt`, with supporting
artifacts under `build/agent_state/380_step4_20000112.*`.

## Suggested Next

Do not close idea 380 as resolved on the current evidence. The next coherent
packet is a repair or plan-owner split for the still-unresolved
call-argument reload/prior-preservation route, focused on preserving or
re-materializing the original function argument across the first failed
`strchr` call before later short-circuit operands are evaluated.

## Watchouts

This packet did not implement a new repair and did not rerun or overwrite root
proof logs. The classification rejects a distinct backend/runtime owner for the
remaining crash: c4c's own `special_format` code reaches the second `strchr`
with `a0 == 0`. Clang's reference code stores `fmt` on the stack and reloads it
before each `strchr` call. If the plan owner decides the current runbook is
exhausted, the follow-up should stay in the same reload/preservation initiative
rather than opening a separate runtime-crash idea.

## Proof

Audit/classification only; no root proof log was rerun or overwritten. Existing
proof artifacts inspected: `test_after.log`,
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`,
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/c4c.o`,
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/c4c.bin`, and
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/clang.bin`.

Diagnostic commands generated `build/agent_state/380_step4_20000112.*` via
`riscv64-linux-gnu-objdump`, `riscv64-linux-gnu-readelf`,
`riscv64-linux-gnu-nm`, and `qemu-riscv64`/`qemu-riscv64 -strace`. Key paths:
`build/agent_state/380_step4_20000112.classification.txt`,
`build/agent_state/380_step4_20000112.c4c_trace_tail.txt`,
`build/agent_state/380_step4_20000112.c4c_qemu_L_strace.err`,
`build/agent_state/380_step4_20000112.clang_qemu_L_strace.err`,
`build/agent_state/380_step4_20000112.c4c_o_objdump.txt`, and
`build/agent_state/380_step4_20000112.clang_bin_objdump.txt`.
