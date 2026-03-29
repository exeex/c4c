# AArch64 Binary Utils Contract Baseline

Status: Active baseline for `ideas/open/04_backend_binary_utils_contract_plan.md`

## Current C++ Boundary

The current C++ port has a narrower active boundary than the reference Rust backend:

1. `src/codegen/llvm/llvm_codegen.cpp` lowers HIR to LIR and routes `--codegen lir` through `backend::emit_module`.
2. `src/backend/backend.cpp` selects the AArch64 backend emitter for `aarch64-*` triples.
3. `src/backend/aarch64/codegen/emit.cpp` emits GNU-style AArch64 assembly text for the supported fallback slices.
4. `src/apps/c4cll.cpp` writes that backend output to `-o` or stdout. It does not yet drive `.s -> .o -> executable` inside the app.
5. External toolchain assembly and link validation currently lives in test harnesses under `tests/c/internal/cmake/`.

That means the repo's current executable contract is:

- production CLI seam: `HIR -> LIR -> AArch64 assembly text`
- validation seam: `assembly text -> clang --target=aarch64-unknown-linux-gnu -> ELF object or executable`

The in-tree AArch64 assembler and linker subtrees are compiled into the build, but their top-level integration is not yet wired into `c4cll`:

- `src/backend/aarch64/assembler/`
- `src/backend/aarch64/linker/`

## Repo-Local Baseline Cases

### 1. Minimal backend-emitted single-object case

Source: `tests/c/internal/backend_case/return_add.c`

Expected backend assembly shape:

```asm
.text
.globl main
.type main, %function
main:
  mov w0, #5
  ret
```

Expected object contract after external assembly:

- ELF64 little-endian AArch64 object
- `.text` section present
- global function symbol `main`
- no `.rela.text` relocations for this case

Locked by CTest:

- `backend_contract_aarch64_return_add_object`

### 2. Current-backend relocation-bearing object case

Source: `tests/c/internal/backend_case/global_load.c`

Expected backend assembly shape:

```asm
.data
.globl g_counter
.type g_counter, %object
.p2align 2
g_counter:
  .long 11
.size g_counter, 4
.text
.globl main
.type main, %function
main:
  adrp x8, g_counter
  ldr w0, [x8, :lo12:g_counter]
  ret
```

Expected object contract after external assembly:

- `.data`, `.text`, and `.rela.text` sections present
- object symbol `g_counter`
- function symbol `main`
- relocation pair:
  - `R_AARCH64_ADR_PREL_PG_HI21 g_counter`
  - `R_AARCH64_LDST32_ABS_LO12_NC g_counter`

Locked by CTest:

- `backend_contract_aarch64_global_load_object`

### 3. External-call relocation fixture

Fixture: `tests/c/internal/backend_toolchain_case/aarch64_call_extern_linux.s`

This is not emitted by the current fallback backend slice today. It exists to pin the external-toolchain side of the AArch64 call relocation contract that later assembler and linker work must preserve.

Expected object contract after external assembly:

- `.rela.text` section present
- global function symbol `main`
- undefined symbol `helper`
- call relocation `R_AARCH64_CALL26 helper`

Locked by CTest:

- `backend_contract_aarch64_extern_call_object`

## Current Shared Surfaces To Preserve

These are the current C++ include-reachable surfaces later binary-utils work will need to stay aligned with:

- `src/backend/backend.hpp`
- `src/backend/target.hpp`
- `src/backend/common.cpp`
- `src/backend/elf/`
- `src/backend/linker_common/`
- `src/backend/aarch64/assembler/`
- `src/backend/aarch64/linker/`

## Immediate Follow-On Note

The fallback AArch64 emitter still returns LLVM IR for unsupported slices rather than assembly text. During this baseline capture, that showed up for cases such as extern-call lowering and pointer roundtrip cases. Later contract work should keep distinguishing:

- backend slices that already emit GNU-style AArch64 assembly
- cases that still fall back to LLVM IR and therefore are not yet part of the assembly-object compatibility baseline
