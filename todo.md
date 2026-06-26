Status: Active
Source Idea Path: ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure or Handoff Check

# Current Packet

## Just Finished

Step 4 classified the remaining `src/20000112-1.c` RV64 object-route runtime
segfault after the select-publication repair. The remaining failure is a
distinct backend/runtime owner, not the repaired select/publication owner.

Concrete symptom: `case.log` still reports
`[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Segmentation fault`.
Replaying the binaries under `qemu-riscv64 -L /usr/riscv64-linux-gnu` shows
clang exits 0, while c4c exits 139 with `SIGSEGV si_addr=NULL`.

Concrete failure location: in c4c `special_format`, the first
`strchr(format, '*')` returns NULL. The generated c4c code then enters the next
short-circuit RHS and calls `strchr` with `a0` still NULL and `a1='V'`; QEMU
traces execution from c4c address `0x555555556740` through the PLT to libc
`strchr`, where the NULL string load faults. The clang reference preserves the
incoming string parameter and reloads it before each `strchr` call.

Evidence paths:
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`,
`build/agent_state/379_step4_20000112.classification.txt`,
`build/agent_state/379_step4_20000112.c4c_o_objdump.txt`,
`build/agent_state/379_step4_20000112.c4c_bin_objdump.txt`,
`build/agent_state/379_step4_20000112.clang_bin_objdump.txt`,
`build/agent_state/379_step4_20000112.c4c_qemu_L_strace.err`,
`build/agent_state/379_step4_20000112.c4c_qemu_trace.log`, and
`build/agent_state/379_step4_20000112.clang_qemu_L_strace.err`.

## Suggested Next

Ask the plan owner to close idea 379 if the select/publication repair is
otherwise complete, then open or switch to a follow-up idea for RV64 object
lowering that preserves/reloads call arguments across short-circuit RHS calls.

## Watchouts

No implementation repair was attempted. The new owner should be treated as a
general call-argument liveness/reload issue around short-circuit lowering, not
as a `src/20000112-1.c` named-case special case. Do not weaken expectations or
touch real external allowlists for this handoff.

## Proof

Audit/classification only; no root proof logs were rerun or overwritten.

Commands used for diagnostic evidence wrote artifacts under `build/agent_state/`
only: `riscv64-linux-gnu-objdump`, `riscv64-linux-gnu-readelf`,
`riscv64-linux-gnu-nm`, and `qemu-riscv64 -L /usr/riscv64-linux-gnu` with
`-strace`/`-d in_asm,exec,cpu`. The available host `gdb` was not RISC-V-aware,
so `build/agent_state/379_step4_20000112.c4c_gdb.txt` is retained only as a
tooling note and was not used for classification.
