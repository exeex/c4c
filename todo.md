Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Allow Constant Offset I32 Global Stores

# Current Packet

## Just Finished

Completed plan.md Step 4 store-offset support in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`.

Changed owner surface:
- Generalized the prepared global `i32` store access check to accept a proven
  constant byte offset from the prepared global-symbol address authority.
- Required the access to remain a single default-address-space non-volatile
  `i32` store with prepared `size_bytes == 4`, `align_bytes >= 4`,
  `can_use_base_plus_offset`, non-negative aligned BIR byte offset, signed
  12-bit RV64 store immediate range, and object bounds within the simple
  defined `i32` global.
- Preserved scalar `global_store.c` behavior with `sw 0(base)`.
- Registered `backend_rv64_runtime_defined_global_array_store` with expected
  exit code 9; generated assembly stores `arr[1][0]` as `sw ..., 8(...)` and
  reloads the same offset.

## Suggested Next

Execute Step 5 optional boundary probes for
`defined_global_array_pointer.c` and `defined_global_array_pointer_store.c` if
the supervisor wants to classify same-rule fallout now; otherwise proceed to
Step 6 focused RISC-V validation.

## Watchouts

Load and store offsets now accept only signed 12-bit immediates because the
prepared RV64 global memory path materializes the symbol base with `lla` and
emits one `lw` or `sw`. Larger constant offsets should remain fail-closed until
a separate address materialization rule exists. Pointer-local boundary cases
may require wider address/pointer work and should not be accepted unless they
fall out of the same global-symbol plus constant-offset rule.

## Proof

Passed. Proof output is captured in `test_after.log`.

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime_(global_store|defined_global_array|defined_global_array_store)$'`

Observed in `test_after.log`:
- `backend_rv64_runtime_global_store` passed under qemu with expected exit code
  7.
- `backend_rv64_runtime_defined_global_array` passed under qemu with expected
  exit code 7.
- `backend_rv64_runtime_defined_global_array_store` passed under qemu with
  expected exit code 9.
