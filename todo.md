# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Emit Defined i32 Global Storage

## Just Finished

Completed `plan.md` Step 3: made the RV64 prepared global-memory owner emit
minimal assembler-visible storage for defined scalar i32 globals without adding
global load/store lowering or runtime registrations.

Implemented storage behavior:

- Nonzero initialized scalar i32 globals emit in `.data` with `.balign 4`, the
  semantic global label, and `.word <initializer>`.
- Zero-initialized scalar i32 globals, including definitions with no explicit
  initializer, emit in `.bss` with `.balign 4`, the semantic global label, and
  `.zero 4`.
- The owner pre-validates all module globals before appending storage, so
  unsupported global forms still fail closed instead of receiving partial data
  emission.
- The existing simple-global gate now admits scalar i32 immediate initializers
  as well as default-zero definitions, while continuing to reject extern
  globals, TLS, non-i32 globals, non-4-byte storage, aggregate/array initializer
  elements, and pointer/symbol initializers.
- `LoadGlobalInst` and `StoreGlobalInst` hooks still return `std::nullopt`
  after validation; symbol-address load/store lowering remains Step 4/6 work.
- `emit.cpp`, runtime case registration, and unrelated backend files were not
  changed.

## Suggested Next

Execute `plan.md` Step 4 by lowering initialized scalar i32 global loads through
the same RV64 prepared global-memory owner, using a semantic global-symbol
address sequence and registering `global_load.c` only after the real RV64 asm
route executes under qemu with exit code `11`.

## Watchouts

- Step 4 should not special-case `g_counter` or the candidate filename; it
  should consume the existing prepared `GlobalSymbol` memory-access metadata and
  the module global definition gate.
- Use one symbol-address authority for loads and later stores. Avoid adding a
  load-only addressing path that Step 6 would have to duplicate.
- The generated assembly for global cases now includes storage but still lacks
  executable global load/store instructions, so runtime registration remains
  intentionally deferred.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, TLS, or
  GOT-heavy work.

## Proof

Ran the supervisor-selected proof successfully; output is captured in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/global_load.c -o /tmp/c4c_global_load.s && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/global_load_zero_init.c -o /tmp/c4c_global_load_zero_init.s && rg 'g_counter:|\.word[[:space:]]+11|\.zero[[:space:]]+4|\.bss|\.data' /tmp/c4c_global_load.s /tmp/c4c_global_load_zero_init.s
```

Result: build passed, all 23 matching `backend_rv64_runtime` tests passed, and
the generated RV64 assembly showed `.data`, `g_counter:`, and `.word 11` for
`global_load.c`, plus `.bss`, `g_counter:`, and `.zero 4` for
`global_load_zero_init.c`.
