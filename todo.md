Status: Active
Source Idea Path: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify The Runtime Abort

# Current Packet

## Just Finished

Step 1 reproduced and classified the `src/20000622-1.c` RV64 object-route
runtime abort. The delegated runner still fails with
`[RV64_BACKEND_RUNTIME_MISMATCH]`, matching the 572 evidence:
`clang_exit=0 c4c_exit=Subprocess aborted`. Direct qemu reruns recorded
`clang_rc=0` and `c4c_rc=134`.

Artifacts are under
`build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/`,
including `c4c.o`, `c4c.bin`, `clang.bin`, `c4c.o.disasm`,
`c4c.bin.disasm`, `clang.bin.disasm`, qemu stdout/stderr captures, strace
captures, and `classification-summary.txt`.

First-family classification: this is not the old ordinary same-module
`CallInst` fallback and is not primarily `bar` call-result publication. In
c4c `baz`, the value later passed as `foo` argument 0 is loaded from an
`s1`-derived stack value at function entry even though the source requires
`d = (long)c`, where `c` is incoming argument `a2` with value 12. Clang stores
and reloads incoming `a2`; c4c instead passes the wrong first argument to
`foo`, which reaches the `abort()` path.

## Suggested Next

Delegate Step 2: add focused RV64 object-route coverage for incoming pointer
parameter materialization through a pointer-to-integer local value that must
survive a nested same-module call before being passed to another call.

## Watchouts

- Keep this route classification-first.
- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  behavior, or the gcc_torture runner.
- Do not add filename-specific handling for `src/20000622-1.c`.
- Preserve the same-module call/result behavior repaired under 572.
- The classified bad value is the `baz` local/parameter materialization for
  `d = (long)c`; `bar(a, 1)` still returns 1 into the second `foo` argument.
- The first focused case should prove the semantic family without depending on
  the gcc torture filename or exact generated symbol names.

## Proof

Proof log: `test_after.log`.

Commands run:

- `cmake --build --preset default --target c4cll` -> `BUILD_RC=0`
- `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000622-1.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/c4c.bin -DSYSROOT=/usr/riscv64-linux-gnu -DTARGET_TRIPLE=riscv64-linux-gnu -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` -> `CASE_RC=1`, expected reproduction failure
- Read-only diagnostics: `riscv64-linux-gnu-objdump -dr` for `c4c.o`,
  `c4c.bin`, and `clang.bin`; `riscv64-linux-gnu-nm -n` for both linked
  binaries; direct qemu reruns for `clang.bin` and `c4c.bin`; qemu `-strace`
  rerun for `c4c.bin`.
