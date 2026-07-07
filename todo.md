Status: Active
Source Idea Path: ideas/open/576_rv64_pr56982_post_carrier_runtime_mismatch.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Identify the first post-carrier semantic owner

# Current Packet

## Just Finished

Completed Step 2: identified the first post-carrier semantic owner for
`src/pr56982.c` as RV64 prepared/object emission materialization for a
symbol-address value with a persistent callee-saved register home.

Owner:
- Function: `f`
- Source operation path: `int x = setjmp(env); ... longjmp(env, 42);`
- Prepared value: global symbol address `@env`, value id `6`
- Prepared location: `home @env value_id=6 kind=register reg=s1`
- Failing callsite: `longjmp(ptr @env, i32 42)`, prepared block index `6`,
  instruction index `0`

Evidence:
- Owner notes:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/owner-notes.md`
- c4c disassembly:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/c4c.bin.objdump.txt`
- c4c object disassembly:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/c4c.o.objdump.txt`
- clang reference disassembly:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/clang.bin.objdump.txt`
- qemu runtime trace:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/qemu-in-asm-cpu.trace`
- qemu strace:
  `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/qemu-strace.err`

Finding: prepared-BIR records `@env` as `s1` and asks call preservation to keep
that value across `_setjmp`, `g`, and `longjmp`, but generated c4c code only
materializes `&env` into `a0` for `_setjmp`. It never initializes `s1` with the
symbol address. The later `longjmp` path emits `mv a0,s1`, so qemu enters libc
`longjmp` with `a0=1`, `a1=42`, `s1=1` and then faults with
`SIGSEGV si_addr=0x00000000000000d1`.

Recommended implementation surface: RV64 object emission/call argument
materialization for symbol-address values whose prepared home is a register,
especially callee-saved homes used as preserved values across calls. This
appears local to the active idea, not a separate durable initiative.

## Suggested Next

Execute Step 3: add focused RV64 backend proof for the symbol-address register
home owner without matching `src/pr56982.c` by name. The proof should expose
that a global symbol address used across multiple calls must not source a later
call argument from an uninitialized callee-saved register home.

## Watchouts

- Do not change inline asm carrier diagnostics or unsupported classification.
- Do not use filename-specific handling for `src/pr56982.c`.
- Do not claim progress from expectation rewrites, unsupported-marker edits,
  allowlist changes, or runtime comparison changes.
- The observed c4c fault is explained before any general setjmp/longjmp
  semantic modeling issue: the immediate bad fact is `longjmp` receiving
  `a0=1` instead of `&env`.
- The clang reference rematerializes `&env` for both `_setjmp` and `longjmp`;
  c4c rematerializes it only for `_setjmp`.

## Proof

Step 2 proof was evidence inspection plus generated artifact capture under
`build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step2/src_pr56982.c/`.
Commands used:
- `riscv64-linux-gnu-objdump -dr .../c4c.o`
- `riscv64-linux-gnu-objdump -dr .../c4c.bin`
- `riscv64-linux-gnu-objdump -dr .../clang.bin`
- `timeout 20 qemu-riscv64 -L /usr/riscv64-linux-gnu -strace .../c4c.bin`
- `timeout 20 qemu-riscv64 -L /usr/riscv64-linux-gnu -d in_asm,cpu -D .../qemu-in-asm-cpu.trace .../c4c.bin`

Both qemu runs exited `139`, reproducing the segfault. No root
`test_after.log` was produced because this was an evidence/classification
packet and the delegated proof requested Step 2 artifacts.
