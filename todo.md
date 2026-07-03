Status: Active
Source Idea Path: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rehydrate Evidence And Reproduce

# Current Packet

## Just Finished

- Step 1 Rehydrate Evidence And Reproduce completed for
  `src/20000622-1.c`.
- Rehydrated saved 577 evidence from
  `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/`:
  `classification-summary.txt`, `dump-prepared-bir.txt`, `c4c.bin.disasm`,
  and `clang.bin.disasm`.
- Fresh rerun artifacts are under
  `build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/src_20000622-1.c/`:
  `clang.bin`, `c4c.o`, `c4c.bin`, `c4c.o.disasm`, `c4c.bin.disasm`,
  `clang.bin.disasm`, `c4c.bin.nm`, and `clang.bin.nm`.
- The fresh runner still reports
  `[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Subprocess aborted`
  with runner rc `1`, after a successful `c4cll` build.
- The first remaining bad area is still the post-577 `foo` logical/select
  family: saved evidence preserves the fixed `baz` route where arg0 to `foo`
  is sourced from preserved `s2`, while `foo` has prepared select-chain rows
  for `%t13` and `%t24` with missing/unsupported select carrier alias
  authority.

## Suggested Next

- Delegate Step 2: Classify The First Bad Owner by tracing the `%t13`/`%t24`
  select materializations and carrier alias publication through RV64 lowering
  to the first wrong runtime/object behavior.

## Watchouts

- Do not treat expectation rewrites, unsupported-marker edits, allowlist changes,
  runtime comparison changes, or filename-specific handling as progress.
- Preserve the fixed 577 `baz` route: incoming `a2` materialized through
  `ptrtoint`, preserved across `bar`, and passed as `foo` argument 0.
- The fresh c4c disassembly still shows `baz` moving `s2` into `a0` before the
  `foo` call; do not route this as the old 577 wrong-argument family without
  new contradictory evidence.

## Proof

- Proof log: `test_after.log`.
- Build command: `cmake --build build --target c4cll` returned rc `0`.
- Reproduction command returned rc `1` as the expected current mismatch:
  `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000622-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/src_20000622-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/src_20000622-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/src_20000622-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`.
