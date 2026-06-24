# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Scalar Prepared Object Support

## Just Finished

- Step 2 added minimal RV64 object-emitter support for the scalar
  `local_temp.c` frame-slot pattern and restored its object runtime scan case.
- `src/backend/mir/riscv/codegen/object_emission.cpp` now accepts the prepared
  local scalar shape with one block, prepared frame-slot memory accesses,
  immediate 4-byte `store_local`, 4-byte `load_local` into a GPR home, and a
  stack-frame prologue/epilogue using `sp`. It emits `sw`/`lw` directly as RV64
  object bytes.
- The frame-slot support is deliberately narrow and fail-closed: no arrays,
  globals/data, pointer arithmetic, volatile memory, non-4-byte local accesses,
  local/call frame combinations, or broad memory operations.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` now covers the
  prepared local-frame shape directly, including the 16-byte object frame,
  `addi t1, zero, 5`, `sw t1, 0(sp)`, `lw t0, 0(sp)`, return move, frame
  teardown, and no relocations.
- `tests/backend/CMakeLists.txt` restored
  `backend_obj_runtime_rv64_local_temp`; it links the compiler-produced `.o`
  and runs under qemu with expected return code 5.
- Step 2's intended RV64 scalar object-runtime subset is now green:
  `return_add_sub_chain.c`, `return_add.c`, `two_arg_helper.c`, and
  `local_temp.c`.

## Suggested Next

- Move to Step 3: add AArch64 selected move-immediate plus immediate
  `Add`/`Sub` object encoding for `return_add_sub_chain.c`, then restore the
  focused AArch64 object-byte scan case if it emits a valid ELF object.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- RV64 scalar local support is intentionally limited to `local_temp.c`'s simple
  stack-home pattern; do not broaden this result to arrays, local pointers,
  local/frame-slot call args, or aggregate locals without separate proof.
- AArch64 object emission is unchanged in this packet; its next small candidate
  is still `return_add_sub_chain.c`, not the frame/call-heavy `return_add.c`.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_obj_runtime_rv64_two_arg_helper|backend_obj_runtime_rv64_local_temp|backend_rv64_runtime_local_temp)$' --output-on-failure) > test_after.log 2>&1`
- Result: 4/4 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
