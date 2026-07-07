Status: Active
Source Idea Path: ideas/open/576_rv64_pr56982_post_carrier_runtime_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce post-carrier runtime mismatch and collect evidence

# Current Packet

## Just Finished

Completed Step 1: reproduced the fresh `src/pr56982.c` RV64 object-route
post-carrier runtime mismatch and saved route plus prepared-BIR evidence under
`build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/`.

Route command:
`cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/pr56982.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

Artifacts:
- Route command: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/object-route.cmd`
- Route rc: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/object-route.rc` (`1`)
- Route stdout: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/object-route.out`
- Route stderr: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/object-route.err`
- Merged route log: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/object-route.log`
- Generated binaries/objects: `clang.bin`, `c4c.o`, `c4c.bin` in the same directory
- Prepared-BIR command: `build/agent_state/576_rv64_pr56982_post_carrier_runtime_mismatch/step1/src_pr56982.c/dump-prepared-bir.cmd`
- Prepared-BIR rc/stdout/stderr: `dump-prepared-bir.rc` (`0`), `dump-prepared-bir.txt`, `dump-prepared-bir.err`

Current symptom: `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
`c4c_exit=Segmentation fault`; both outputs are empty in the route log. The old
inline asm carrier compile diagnostic is absent from the route log, and the
prepared dump still records the inline asm carrier evidence for later tracing.

## Suggested Next

Execute Step 2: inspect the saved Step 1 route artifacts and prepared-BIR dump
to identify the first post-carrier semantic owner of the c4c segfault/runtime
mismatch.

## Watchouts

- Do not change inline asm carrier diagnostics or unsupported classification.
- Do not use filename-specific handling for `src/pr56982.c`.
- Do not claim progress from expectation rewrites, unsupported-marker edits,
  allowlist changes, or runtime comparison changes.

## Proof

Step 1 reproduction proof ran the saved RV64 object route command in
`object-route.cmd`; result was the expected failing route rc `1` with
`RV64_BACKEND_RUNTIME_MISMATCH`. Supporting prepared-BIR dump ran with
`build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr56982.c`
and exited `0`. No root `test_after.log` was produced because this packet was
evidence-only and the delegated proof requested per-case route artifacts.
