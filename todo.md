Status: Active
Source Idea Path: ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Capture Runtime Abort

# Current Packet

## Just Finished

Step 1 reproduced the current RV64 runtime abort for
`tests/c/external/gcc_torture/src/20000819-1.c` with fresh artifacts under
`build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step1/src_20000819-1.c/`.

- Build command: `cmake --build --preset default --target c4cll`, return code
  `0`.
- Prepared dump command: `build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000819-1.c`,
  return code `0`.
- RV64 object/runtime route command: `cmake ... -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`,
  return code `1`.
- Current failure mode remains `[RV64_BACKEND_RUNTIME_MISMATCH]` with
  `clang_exit=0` and `c4c_exit=Subprocess aborted`.
- Fresh route artifacts include `dump-prepared-bir.txt`, `object-route.log`,
  `c4c.o`, `c4c.bin`, and `clang.bin`.
- `unsupported_pointer_arithmetic` is absent from the fresh Step 1 route logs
  and artifacts.

## Suggested Next

Run Step 2: isolate the first divergence or abort owner by comparing the fresh
`clang.bin` and `c4c.bin` behavior and inspecting the generated RV64 object and
prepared BIR around the abort path.

## Watchouts

- Do not classify the representative as unsupported or weaken the runtime
  comparison.
- Do not match on filename, function name, block name, value name, or exact
  command output.
- Do not reopen pointer arithmetic publication unless fresh evidence proves the
  published pointer result is still semantically wrong.
- The Step 1 route now builds C4C object and linked binary successfully before
  runtime comparison aborts; this is downstream of the old compile-time pointer
  publication owner.

## Proof

Step 1 reproduction proof wrote `test_after.log`.

- `cmake --build --preset default --target c4cll`
- `build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000819-1.c`
- `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000819-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step1/src_20000819-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step1/src_20000819-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step1/src_20000819-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`
