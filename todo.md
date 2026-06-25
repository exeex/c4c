Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Data and Current Rejection Path

# Current Packet

## Just Finished

Completed Plan Step 1: inspected prepared data publication and the current RV64
object-route rejection path for globals and string constants.

Prepared module data available to RV64 object emission:

- `bir::Module::globals` publishes `bir::Global` records with `name`,
  `link_name_id`, `type`, `is_extern`, `is_thread_local`, `is_constant`,
  `size_bytes`, `align_bytes`, `initializer`, `initializer_symbol_name`,
  `initializer_symbol_name_id`, `initializer_elements`, and
  `address_materialization_policy`.
- `bir::Module::string_constants` publishes `bir::StringConstant` records with
  `name`, `name_id`, `bytes`, and `align_bytes`.
- LIR string-pool lowering also publishes byte-addressable string globals in
  `module.globals`: name from the string pool, `type=I8`, `is_extern=true`,
  `is_constant=true`, `size_bytes=byte_length`, `align_bytes=1`. These are the
  addressable symbol/storage contract for string literals; the literal bytes
  live in `module.string_constants`.
- Per-function prepared metadata publishes data references through
  `PreparedAddressMaterialization` and `PreparedMemoryAccess`: string address
  materializations carry `kind=StringConstant`, `text_name`, `result_value_name`,
  `byte_offset`, address-space, and TLS flags; global memory accesses carry
  `address.base_kind=GlobalSymbol`, `address.symbol_name`, `byte_offset`,
  `size_bytes`, `align_bytes`, `can_use_base_plus_offset`, range verdicts, and
  result/stored value names.

Current RV64 rejection sites:

- `src/backend/mir/riscv/codegen/object_emission.cpp` rejects any non-empty
  `prepared.module.string_constants` before function lowering with
  `module_string_constants: RV64 object route does not emit prepared string constants`.
- The same file rejects `LoadGlobalInst` and `StoreGlobalInst` during
  instruction admission with
  `unsupported_global_data: RV64 object route does not lower prepared global memory instructions`.
- The object route already has narrow global storage support in
  `append_rv64_prepared_constant_global_objects`: constant scalar immediate
  globals are emitted to `.rodata`, with a global object symbol. Unsupported
  global shapes are rejected after text-object construction with
  `unsupported_global_data: RV64 object route supports only constant scalar global objects`.
- The prepared text emitter has a broader reference model in
  `prepared_global_memory_emit.cpp`: it recognizes no-storage extern globals,
  simple defined i32 globals, simple zero storage, string-backed globals, direct
  global address materializations, string-constant address materializations, and
  i32 global load/store emission. This is useful precedent, but Step 2 should
  emit through the object model, not copy text-only assumptions as the proof.

Representative diagnostics observed:

- `src/20000112-1.c` and `src/20000223-1.c` fail `--codegen obj` at the
  `module_string_constants` rejection. Prepared dumps show string constants are
  referenced by `kind=string_constant` address materializations, e.g. `.str0`,
  with call-argument sources published as symbol addresses.
- `src/20000224-1.c` and `src/20000227-1.c` fail `--codegen obj` at the
  global memory instruction rejection. Prepared dumps show `load_global` and
  `store_global` instructions plus matching `base=global_symbol` memory-access
  records with symbol, offset, size, alignment, and in-bounds verdicts.

Missing upstream contract facts found for this Step 1 audit: none for the
initial supported packet. The prepared contract already exposes enough data to
emit string bytes, string/global symbols, scalar or byte-array global storage,
and exact direct symbol references for the inspected cases. Broader pointer
initializers, TLS/GOT policies, aggregate pointer relocation contents, and
non-immediate/unsupported initializer elements should remain rejected with
precise diagnostics until a later packet or upstream contract idea owns them.

## Suggested Next

Delegate Step 2: add object-model data section and symbol emission for
prepared string constants plus supported prepared global storage. Scope the
first repair to semantic prepared records only:

- Move the string-constant rejection behind an object emission path that writes
  `.rodata` bytes from `module.string_constants`, including a trailing NUL only
  when the prepared bytes do not already include one, and defines object symbols
  using the prepared text spelling.
- Generalize current constant scalar global object emission into a shared
  object-data helper that also handles mutable/constant i32 scalar storage,
  i32 initializer arrays, simple zero storage/BSS when represented by
  `initializer_elements`, and string-backed globals already paired with
  `module.string_constants`.
- Preserve unsupported shapes behind structured diagnostics: extern no-storage
  globals need no section; TLS/GOT, pointer initializers, missing labels,
  duplicate symbols, unsupported element types, and absent byte representation
  must fail closed instead of being inferred from source or case names.
- Add focused object tests in the existing RV64 object-emission test surface
  that inspect emitted sections, symbol kind/binding/size/offset/alignment, and
  ELF writer success for strings and supported globals. Do not attempt global
  load/store instruction lowering or data relocations in this packet.

## Watchouts

- Do not infer missing initializer bytes, symbol semantics, or address-use
  meaning inside RV64 object emission.
- Do not special-case representative torture filenames.
- Do not treat the text emitter as sufficient proof for object behavior; reuse
  its semantic predicates only where they map directly to prepared records.
- `20000224-1.c` and `20000227-1.c` will still need later instruction/relocation
  support after Step 2 data sections are emitted, because they currently fail at
  `LoadGlobalInst`/`StoreGlobalInst` admission.
- `20000227-1.c` uses i8 global loads at byte offsets; Step 2 data emission can
  prepare the storage, but Step 3 or a later packet must decide instruction
  width support and relocation/address materialization.

## Proof

Audit-only; no build was required and no `test_after.log` was created.

Read-only inspection commands included:

```sh
rg -n "string_constants|globals|Prepared.*Module|prepared.module|module.globals|module.string" src tests scripts -g '*.cpp' -g '*.hpp' -g '*.cmake' -g '*.sh'
rg -n "build_rv64_prepared_text_object_module|module_string_constants|unsupported_global_data|constant scalar|LoadGlobalInst|StoreGlobalInst" src/backend/mir/riscv tests/backend/mir/backend_riscv_object_emission_test.cpp
build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj -o /tmp/c4c_357_20000112.o tests/c/external/gcc_torture/src/20000112-1.c
build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj -o /tmp/c4c_357_20000223.o tests/c/external/gcc_torture/src/20000223-1.c
build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj -o /tmp/c4c_357_20000224.o tests/c/external/gcc_torture/src/20000224-1.c
build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj -o /tmp/c4c_357_20000227.o tests/c/external/gcc_torture/src/20000227-1.c
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000112-1.c
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000223-1.c
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000224-1.c
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000227-1.c
```

The four representative `--codegen obj` runs failed with the expected current
diagnostics listed above, which is sufficient proof for this read-only audit
packet.
