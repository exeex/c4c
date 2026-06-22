Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Probe Global-Pointer Data Initializer Boundary

# Current Packet

## Just Finished

Completed plan.md Step 6 by probing the three pointer-valued global
initializer boundary cases without adding support or runtime registrations.

Observed boundary:

- `defined_pointer_global_array.c`: remains unsupported. The source has a
  global pointer data initializer, `int *gp = arr`, and runtime probing emits
  only a 10-byte assembly file containing `.text`. Linking fails before qemu
  with `undefined reference to main`, so the case does not execute through real
  RV64 assembly.
- `defined_pointer_global_array_offset.c`: remains unsupported. The source has
  a global pointer data initializer with an offset, `int *gp = &arr[1]`, and
  runtime probing emits only `.text`. Linking fails before qemu with
  `undefined reference to main`.
- `defined_pointer_global_array_store.c`: remains unsupported. The source has
  a global pointer data initializer, `int *gp = arr`, followed by a store
  through that global pointer slot; runtime probing emits only `.text`. Linking
  fails before qemu with `undefined reference to main`.

Why: all three require pointer-valued global initializer/data-slot lowering
and relocation semantics for global pointer values. The active plan only owns
local pointer values initialized inside a function from a direct global `i32`
array address.

## Suggested Next

Continue with plan.md Step 7 only if delegated: run focused RISC-V validation
for the accepted local-pointer global-array load/store capability.

## Watchouts

- The Step 6 probes do not produce fallback BIR/LLVM text, but they also do not
  produce a runnable `main`; the runtime harness therefore fails closed at the
  clang link step before qemu.
- Do not treat the three pointer-valued global initializer cases as accepted
  runtime cases under this local-pointer idea without a separate source-intent
  change.
- If a future idea owns these cases, the smallest missing surface is explicit
  global pointer data initializer lowering, including data-slot emission and
  relocation/address semantics for initialized global pointer values.

## Proof

Build proof passed:

`cmake --build --preset default`

Result: build succeeded; ninja reported no work to do.

Manual runtime-harness probes, no `test_after.log` written because no CTest
subset was run:

`cmake -DCOMPILER=$PWD/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=$PWD/tests/backend/case/defined_pointer_global_array.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_ASM=$PWD/build/backend_rv64_runtime/defined_pointer_global_array.manual.s -DOUT_BIN=$PWD/build/backend_rv64_runtime/defined_pointer_global_array.manual.bin -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=10 -DEXPECTED_RUN_CODE=7 -P tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Result: failed closed at `[BACKEND_RV64_CLANG_FAIL]` with `undefined reference
to main`; emitted assembly was only `.text`.

`cmake -DCOMPILER=$PWD/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=$PWD/tests/backend/case/defined_pointer_global_array_offset.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_ASM=$PWD/build/backend_rv64_runtime/defined_pointer_global_array_offset.manual.s -DOUT_BIN=$PWD/build/backend_rv64_runtime/defined_pointer_global_array_offset.manual.bin -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=10 -DEXPECTED_RUN_CODE=9 -P tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Result: failed closed at `[BACKEND_RV64_CLANG_FAIL]` with `undefined reference
to main`; emitted assembly was only `.text`.

`cmake -DCOMPILER=$PWD/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=$PWD/tests/backend/case/defined_pointer_global_array_store.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_ASM=$PWD/build/backend_rv64_runtime/defined_pointer_global_array_store.manual.s -DOUT_BIN=$PWD/build/backend_rv64_runtime/defined_pointer_global_array_store.manual.bin -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=10 -DEXPECTED_RUN_CODE=9 -P tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Result: failed closed at `[BACKEND_RV64_CLANG_FAIL]` with `undefined reference
to main`; emitted assembly was only `.text`.
