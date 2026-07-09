Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove breadth and guardrails

# Current Packet

## Just Finished

Step 4 reran the 28 saved cast residual rows from
`build/agent_state/623_step1_cast_residuals.tsv` and the 60 non-cast guard
rows from `build/agent_state/623_step1_non_cast_guard_rows.tsv` through the
RV64 gcc torture backend-object CMake runner into isolated Step 4 artifacts:
`build/agent_state/623_step4_cast_guard.allowlist`,
`build/agent_state/623_step4_cast_guard_summary.tsv`,
`build/agent_state/623_step4_cast_guard_failed.txt`,
`build/agent_state/623_step4_cast_guard_notes.md`, and
`build/agent_state/623_step4_rerun_logs/`.

The rerun did not clobber the main mutable scan summary/failed artifacts.
Cast results were 1 pass and 27 fail. The only passing saved cast row was
`src/p18298.c` in `rv64-consumer:width-preserving-trunc-i32-to-i32`.
Remaining cast rows by Step 2 owner bucket:
18 `rv64-consumer:width-preserving-zext-i32-to-i32`, 3
`rv64-consumer:width-preserving-trunc-i32-to-i32`, 2
`rv64-consumer:ptrtoint-value-ptr-to-i32`, 1
`rv64-consumer:ptrtoint-global-ptr-to-i32`, 1
`rv64-consumer:ptrtoint-local-memory-ptr-to-i32`, and 2
`floating-policy:f128-sitofp`.

The non-cast guard rows remained closed: all 60 failed with
`RV64_C4C_OBJ_COMPILE_FAIL`, split as 10 `BinaryInst`, 39 `CallInst`, 1
`LoadLocalInst`, 7 `SelectInst`, and 3 `StoreLocalInst` rows. This preserves
the intended no-code guardrail boundary, but the cast rerun is a justified
no-breadth blocker for treating the current width-preserving implementation as
a broad Step 2 bucket repair.

## Suggested Next

Route a supervisor review/plan-owner decision before more code work. The next
coherent packet should classify why the remaining width-preserving rows still
reject as `CastInst` unsupported, and why `src/pr81556.c` reaches
`RV64_BACKEND_RUNTIME_MISMATCH`, before widening the RV64 cast consumer.

## Watchouts

- This packet made no code, expectation, unsupported-marker, allowlist, or main
  scan-artifact changes.
- `src/pr81556.c` no longer stops at object compile in the Step 4 rerun, but it
  aborts at runtime against clang's expected zero exit. Treat that separately
  from the still-unsupported `CastInst` rows.
- Do not accept testcase-shaped widening. The next packet should classify the
  shared semantic/home facts for the remaining 18 zext and 3 trunc rows before
  implementation.

## Proof

Supervisor-selected proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1
```

Step 4 rerun command shape:

```sh
cmake --build "$(realpath -m build)" --target c4cll
cmake -DCOMPILER="$(realpath -m build)/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="tests/c/external/gcc_torture/<case>" -DROOT="$(realpath -m tests/c/external/gcc_torture)" -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN="build/agent_state/623_step4_rerun_logs/<row>/clang.bin" -DOUT_OBJECT="build/agent_state/623_step4_rerun_logs/<row>/c4c.o" -DOUT_C4C_BIN="build/agent_state/623_step4_rerun_logs/<row>/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

Result: passed for the delegated proof. `test_after.log` is the canonical proof
artifact; the backend CTest subset completed successfully after the isolated
Step 4 rerun/classification.
