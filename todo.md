Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Global Array Inputs

# Current Packet

## Just Finished

Completed plan.md Step 1 inspection for the two RV64 defined-global-array
runtime inputs.

Findings:
- `tests/backend/case/defined_global_array.c` lowers for RV64 prepared BIR to
  `bir.load_global i32 @arr, offset 12`; prepared addressing records
  `base=global_symbol result=%t4 symbol=arr offset=12 size=4 align=4
  base_plus_offset=yes range_verdict=proven_in_bounds`.
- `tests/backend/case/defined_global_array_store.c` lowers for RV64 prepared
  BIR to `bir.store_global @arr, offset 8, i32 9` followed by
  `bir.load_global i32 @arr, offset 8`; prepared addressing records both
  accesses as `base=global_symbol symbol=arr offset=8 size=4 align=4
  base_plus_offset=yes range_verdict=proven_in_bounds`.
- Existing scalar-global success path is `tests/backend/case/global_load.c` and
  `tests/backend/case/global_store.c`: RV64 asm emits `.data`, `.balign 4`,
  `.word 11`, then `lla`, `lw 0(...)`, and for store `sw ... 0(...)`.
- Likely owner surface is
  `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`; AST lookup
  found the relevant helper cluster:
  `append_prepared_global_storage_asm`,
  `emit_riscv_simple_load_global`, `emit_riscv_simple_store_global`, and
  `simple_i32_global_access_for`.
- Missing storage behavior: `is_simple_defined_i32_global` currently rejects
  any global with non-empty `initializer_elements`, while BIR array globals are
  represented as `type=I32`, `size_bytes=16`, `align_bytes=4`, and flattened
  `initializer_elements`.
- Missing access behavior: `simple_i32_global_access_for`,
  `emit_riscv_simple_load_global`, and `emit_riscv_simple_store_global` all
  require byte offset `0`; the two target cases already provide the desired
  constant byte offsets in BIR/prepared metadata (`12` for `arr[1][1]`, `8` for
  `arr[1][0]`).
- Current RV64 asm for both array cases is only `    .text`, because
  `append_prepared_global_storage_asm` fails closed before function emission.

## Suggested Next

Execute Step 2 in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`: add a narrow
helper for simple defined `i32` array globals that accepts non-extern,
non-thread-local globals with `type == I32`, `size_bytes` divisible by `4`,
`align_bytes >= 4`, no pointer/symbol initializer, and flattened immediate
`initializer_elements`; emit `.data` with one `.word` per element for nonzero
initialized arrays and preserve scalar `i32` behavior. Leave nonzero access
offsets for Steps 3 and 4.

## Watchouts

Do not match `arr` or filenames. The semantic rule should be global symbol plus
flattened immediate `i32` storage. Pointer-valued global initializers and named
pointer `initializer_elements` must continue to fail closed. Access support is
not just storage: even after storage emission, Step 3/4 must relax offset `0`
checks and emit `lw/sw <offset>(base)` from the prepared constant byte offset.

## Proof

Read-only inspection packet. No code validation was required and no
`test_after.log` was produced.

Inspection commands run:
- `build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir tests/backend/case/defined_global_array.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir tests/backend/case/defined_global_array_store.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/defined_global_array.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/defined_global_array_store.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir tests/backend/case/global_load.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --dump-prepared-bir tests/backend/case/global_store.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/global_load.c`
- `build/c4cll --target riscv64-unknown-linux-gnu --codegen asm tests/backend/case/global_store.c`
- `build/c4cll --codegen llvm tests/backend/case/defined_global_array.c`
- `build/c4cll --codegen llvm tests/backend/case/defined_global_array_store.c`
- `build/c4cll --codegen llvm tests/backend/case/global_load.c`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp append_prepared_global_storage_asm build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp emit_riscv_simple_load_global build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp emit_riscv_simple_store_global build/compile_commands.json`
