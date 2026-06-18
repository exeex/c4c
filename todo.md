Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Emit Simple I32 Array Global Storage

# Current Packet

## Just Finished

Completed plan.md Step 2 storage emission in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`.

Changed owner surface:
- Replaced scalar-only storage classification with
  `simple_defined_i32_global_words`, which accepts non-extern,
  non-thread-local BIR globals with `type == I32`, `size_bytes` divisible by 4,
  `align_bytes >= 4`, no symbol initializer, and either a scalar immediate
  `i32` initializer or flattened immediate `i32` `initializer_elements`.
- Preserved scalar `i32` storage behavior for `global_load.c` and
  `global_store.c`.
- Added semantic flattened storage emission for simple initialized `i32` array
  globals, including zero padding up to declared `size_bytes` when the flattened
  element list is shorter than the fixed object size.
- Kept pointer/symbol, extern, thread-local, non-`i32`, oversized, and
  non-immediate initializer forms fail-closed.

## Suggested Next

Execute Step 3 in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`: generalize the
prepared global load path to accept a proven constant nonzero byte offset for a
single aligned `i32` access and emit `lw <offset>(base)` through the existing
global-symbol address authority.

## Watchouts

Nonzero load/store access offsets are intentionally still unsupported after
this packet. The two array cases now emit `arr` storage words, then stop before
function access emission because `load.byte_offset`, `store.byte_offset`, and
`PreparedMemoryAccess::address.byte_offset` still require `0`; Step 3 should
relax only the load side.

## Proof

Passed. Proof output is captured in `test_after.log`.

Command:
`cmake --build --preset default && { build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/global_load.c; build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/global_store.c; build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/defined_global_array.c; build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/defined_global_array_store.c; }`

Observed in `test_after.log`:
- Scalar globals still emit `.word 11` plus `lw/sw 0(...)`.
- Both defined-global-array cases now emit flattened storage:
  `.word 1`, `.word 2`, `.word 3`, `.word 7`.
- Array access lowering remains incomplete as intended for Steps 3/4.
