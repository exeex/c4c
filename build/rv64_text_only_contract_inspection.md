# RV64 Text-Only Contract Inspection

## Commanded Control

- Source: `tests/c/external/c-testsuite/src/00094.c`
- Relevant source shape: `extern int x;` plus `int main() { return 0; }`
- Target: `riscv64-linux-gnu`
- Route under inspection: `--codegen asm` prepared backend path.

## Current Prepared-BIR Facts

Observed with:

```sh
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c > /tmp/rv64_00094_prepared.txt
```

Current prepared dump facts:

- `prepared.module target=riscv64-linux-gnu route=semantic_bir_shared`
- `completed_phases: legalize stack_layout liveness out_of_ssa regalloc`
- semantic body is present:
  - `bir.func @main() -> i32`
  - `bir.ret i32 0`
- prepared control flow is present:
  - `prepared.func @main`
  - `block entry terminator=return`
- stack/regalloc facts are empty as expected for this no-storage control:
  - frame size is `0`
  - storage values are `0`
  - allocation constraints are `0`

## Current ASM Facts

Observed with:

```sh
./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c -o /tmp/rv64_00094.s
```

Current asm file facts:

- `/tmp/rv64_00094.s` is 10 bytes.
- It contains only:

```asm
    .text
```

- It does not contain `.globl main`, `main:`, a local `main.entry` block label, `li/addi a0, zero, 0`, or `ret`.
- The command exits successfully, so the route reports this text-only output as accepted asm.

## Owner Surface

AST-backed query facts:

- `c4c::backend::riscv::codegen::emit_prepared_module` is defined in `src/backend/mir/riscv/codegen/emit.cpp` and delegates directly to `emit_prepared_module_text`.
- `c4c::backend::riscv::codegen::emit_prepared_module_text` is defined in `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`.
- Its direct callees include:
  - `append_prepared_global_storage_asm`
  - `make_prepared_function_lookups`
  - `prepared_function_name`
  - `append_simple_prepared_bir_function_asm`
  - `append_edge_publication_move_instruction`
- `append_prepared_global_storage_asm` is defined in `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`.

Concrete emission gate:

- `emit_prepared_module_text` calls `append_prepared_global_storage_asm(out, module)` before appending function text.
- If that helper returns `false`, `emit_prepared_module_text` returns the literal successful output string `    .text\n`.
- `append_prepared_global_storage_asm` currently requires every `prepared.module.globals` entry to satisfy `is_simple_defined_i32_global(global)` and have a non-empty label.
- `is_simple_defined_i32_global` delegates to `simple_defined_i32_global_words`.
- `simple_defined_i32_global_words` rejects `global.is_extern`.

For this no-storage control, `extern int x;` is a declaration and should not require emitted storage. The current RV64 gate treats a non-simple/no-storage global declaration as a global-storage emission failure, and the module emitter maps that failure to `.text` instead of either skipping the declaration and emitting `main` or reporting unsupported input.

## Smallest Step 2 Owner Surface

Smallest implementation packet:

- Update `append_prepared_global_storage_asm` in `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp` to distinguish no-storage global declarations from unsupported storage definitions.
- The helper should allow extern/no-storage declarations to pass without emitting `.data` or `.bss`.
- Keep `emit_prepared_module_text` in `src/backend/mir/riscv/codegen/prepared_module_emit.cpp` in scope for review because it currently converts any global-storage failure into successful `.text` output before function emission.

Watchouts:

- Do not special-case `00094.c`, `main`, or the exact source spelling.
- Do not broaden to string storage, aggregate globals, pointer globals, floating globals, external calls, or full RV64 object/link behavior.
- Do not accept `.text`-only output as success when prepared control flow contains a defined function.
