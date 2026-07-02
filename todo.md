Status: Active
Source Idea Path: ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove The Row-Level Outcome

# Current Packet

## Just Finished

Step 3 - Prove The Row-Level Outcome completed by running the delegated build
and one-row RV64 gcc torture backend object proof for `src/960209-1.c`.

The row-level first blocker is retained as the precise prepared contract
rejection:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel
multi-source stack-destination authority
```

This matches the Step 1 facts and Step 2 retained prepared contract: an
authority=none, non-parallel bundle with multiple register sources targeting one
stack destination must fail closed before RV64 consumes it. No expectations,
unsupported markers, allowlists, runtime comparison behavior, implementation
files, or tests were changed.

## Suggested Next

Hand back to the supervisor for lifecycle routing. The runbook has proven that
the current row-level outcome is the retained precise prepared classifier
rejection, not an RV64 local-memory addressing blocker.

## Watchouts

- Do not continue this work as RV64 local-memory addressing.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c` or materialize ambiguous bundles in
  RV64 by guessing source ownership.
- Treat any acceptance route that picks `%t43` or `%t44`, drops one move, or
  infers ordering from dump order as testcase-overfit.
- The retained contract is semantic, not row-shaped: it does not depend on
  function name `f`, block `tern.end.38`, slot #21, or `%t43`/`%t44`/`%t45`.
- The delegated one-row proof exits nonzero because the row intentionally still
  fails closed at object compilation with the retained classifier diagnostic.

## Proof

Proof commands run exactly as delegated, with combined output recorded in
`test_after.log`:

```sh
cmake --build --preset default
cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/960209-1.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE="riscv64-linux-gnu" -DSYSROOT="/usr/riscv64-linux-gnu" -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.bin" -DCASE_TIMEOUT_SEC="20" -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"
```

Result: build succeeded (`ninja: no work to do.`). The one-row object proof
exited nonzero with `[RV64_C4C_OBJ_COMPILE_FAIL]` because the current first
blocker remains the expected prepared classifier rejection:
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
The supervisor-selected proof was sufficient for this no-code-change
row-outcome recording slice, and `test_after.log` is the canonical proof log.
