Status: Active
Source Idea Path: ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove The Row Moves Past LocalMemoryAccess

# Current Packet

## Just Finished

Step 3 - Prove The Row Moves Past LocalMemoryAccess completed the delegated
one-row RV64 gcc torture backend proof for `src/960209-1.c`.

The proof no longer reports `unsupported_local_memory_access`. The current
row-level first blocker is now:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel
multi-source stack-destination authority
```

Classification: the Step 2 pointer-value byte-store local-memory repair is
proved past the old local-memory diagnostic for this row. The newly exposed
blocker belongs to the prepared move-bundle classifier route, not to the owned
local-memory addressing shape.

## Suggested Next

Supervisor should route the newly exposed
`ambiguous_non_parallel_multi_source_stack_destination` prepared move-bundle
classifier blocker as a separate lifecycle decision or packet. No semantic
repair was attempted in this proof-only packet.

## Watchouts

- The delegated proof command failed at the CMake row harness because the row
  now reaches the prepared move-bundle classifier blocker shown above.
- This packet did not touch implementation, expectation files, unsupported
  markers, allowlists, runtime comparison behavior, `plan.md`, source ideas, or
  `test_before.log`.
- The proof log is the current canonical evidence for the blocker:
  `test_after.log`.

## Proof

Proof log: `test_after.log`.

Commands run:

```sh
cmake --build --preset default
cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/960209-1.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE="riscv64-linux-gnu" -DSYSROOT="/usr/riscv64-linux-gnu" -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.bin" -DCASE_TIMEOUT_SEC="20" -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"
```

Result: blocked as expected for classification. The build completed with
`ninja: no work to do`; the row harness failed with
`[RV64_C4C_OBJ_COMPILE_FAIL]` at the newly exposed prepared move-bundle
classifier diagnostic above.
