Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Allow Constant Offset I32 Global Loads

# Current Packet

## Just Finished

Completed plan.md Step 3 load-offset support in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`.

Changed owner surface:
- Generalized the prepared global `i32` load access check to accept a proven
  constant byte offset from the prepared global-symbol address authority.
- Required the access to remain a single default-address-space non-volatile
  `i32` load with prepared `size_bytes == 4`, `align_bytes >= 4`,
  `can_use_base_plus_offset`, non-negative aligned BIR byte offset, signed
  12-bit RV64 load immediate range, and object bounds within the simple
  defined `i32` global.
- Preserved scalar `global_load.c` behavior with `lw 0(base)`.
- Registered `backend_rv64_runtime_defined_global_array` with expected exit
  code 7; generated assembly loads `arr[1][1]` as `lw ..., 12(...)`.
- Kept store nonzero offsets unsupported for Step 4; store validation still
  requires byte offset 0.

## Suggested Next

Execute Step 4 in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`: reuse the load
offset validation model for prepared global `i32` stores, lower
`sw <offset>(base)` through the same global-symbol authority, then register and
prove `backend_rv64_runtime_defined_global_array_store` with expected exit code
9.

## Watchouts

Load offsets now accept only signed 12-bit immediates because the prepared RV64
global load path materializes the symbol base with `lla` and emits one `lw`.
Larger constant offsets should remain fail-closed until a separate address
materialization rule exists. Store nonzero offsets remain intentionally
unsupported after this packet.

## Proof

Passed. Proof output is captured in `test_after.log`.

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime_(global_load|defined_global_array)$'`

Observed in `test_after.log`:
- `backend_rv64_runtime_global_load` passed under qemu with expected exit code
  11.
- `backend_rv64_runtime_defined_global_array` passed under qemu with expected
  exit code 7.
