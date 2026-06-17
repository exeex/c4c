# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Prepared Global-Memory Owner Surface

## Just Finished

Completed `plan.md` Step 2: added the narrow RV64 prepared global-memory owner
surface and routed prepared `LoadGlobalInst`/`StoreGlobalInst` dispatch through
it without registering runtime cases.

Implemented surface:

- Added `prepared_global_memory_emit.hpp/.cpp` to own prepared global storage,
  global load, and global store hooks for the RV64 prepared route.
- Added the new owner source to `src/backend/CMakeLists.txt`.
- Routed `StoreGlobalInst` and `LoadGlobalInst` from
  `prepared_function_emit.cpp` into the owner after local-memory dispatch.
- Routed `prepared_module_emit.cpp` through the owner before `.text`, while
  keeping non-empty global storage fail-closed until Step 3 emits definitions.

Fail-closed behavior kept by this slice:

- The owner validates only same-module, non-extern, non-TLS, scalar i32 globals
  with prepared default-address-space direct global-symbol memory facts.
- Loads require named i32 results, offset 0, alignment at least 4, a matching
  prepared result value, and prepared `GlobalSymbol` access metadata.
- Stores require i32 immediate or named values, offset 0, alignment at least 4,
  matching prepared stored-value metadata for named values, and prepared
  `GlobalSymbol` access metadata.
- The owner intentionally does not emit storage, `lw`, or `sw` yet, so global
  modules remain unsupported until the Step 3/4/6 semantic emission packets.
- `emit.cpp` was not changed, and no runtime cases were registered.

## Suggested Next

Execute `plan.md` Step 3 by making `prepared_global_memory_emit.*` emit minimal
assembler-visible storage for defined scalar i32 globals, including initialized
and zero-initialized definitions, while continuing to reject unsupported global
forms.

## Watchouts

- Step 3 should replace the current non-empty-global storage rejection in
  `append_prepared_global_storage_asm` with real `.data`/`.bss` or equivalent
  storage emission for only scalar i32 globals.
- Step 3 should preserve the current owner gates for extern globals, TLS,
  non-i32 widths, aggregate/array globals, pointer initializers, and missing
  prepared metadata.
- `LoadGlobalInst`/`StoreGlobalInst` now reach the owner, but the load/store
  hooks still return `std::nullopt` after validation until Step 4/6 add real
  symbol-address lowering.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, TLS, or
  GOT-heavy work.

## Proof

Ran the supervisor-selected proof successfully; output is captured in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure
```

Result: build passed and all 23 matching `backend_rv64_runtime` tests passed.
