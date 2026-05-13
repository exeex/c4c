# AArch64 Binary Utils Contract Baseline

Status: Active baseline for `ideas/open/04_backend_binary_utils_contract_plan.md`

## Current C++ Boundary

The current C++ port has a narrower active boundary than the reference Rust backend:

1. `src/codegen/llvm/llvm_codegen.cpp` lowers HIR to LIR and routes `--codegen asm` through `backend::emit_module`.
2. `src/backend/backend.cpp` lowers LIR through semantic BIR, prepared BIR, the AArch64 prepared-module builder, and selected machine nodes for `aarch64-*` triples.
3. `src/backend/mir/aarch64/codegen/machine_printer.cpp` prints supported selected machine nodes as GNU-style AArch64 `.s` text.
4. `src/apps/c4cll.cpp` writes that `.s` printer output to `-o` or stdout. It does not drive `.s -> .o -> executable` inside the app.
5. External toolchain assembly and link validation currently lives in focused backend tests for the public `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s` route.

That means the repo's current executable contract is:

- production CLI seam: `HIR -> LIR -> semantic BIR -> prepared BIR -> AArch64 target module -> selected machine nodes -> .s printer output`
- validation seam: `.s printer output -> external AArch64 assembler/linker toolchain -> ELF object or executable`

The in-tree AArch64 assembler and linker subtrees are present in the source
tree, but their top-level object/link integration is not wired into `c4cll`:

- `src/backend/mir/aarch64/assembler/`
- `src/backend/mir/aarch64/linker/`

## Current Assembler Inventory

Step-5 inventory of the staged assembler boundary currently shows a printer
surface plus deferred text-first assembler stubs, not a structured internal
assembler IR:

- backend handoff today: `src/backend/backend.cpp` invokes the AArch64 prepared-module builder and prints selected function machine nodes.
- printer entry today: `src/backend/mir/aarch64/codegen/machine_printer.hpp` exposes `print_machine_instruction_nodes(...)` for selected node records.
- parser entry today: `src/backend/mir/aarch64/assembler/parser.hpp` exposes `parse_asm(const std::string&)`, but this is not used as the internal bridge from codegen to object emission.
- assembler entry today: `src/backend/mir/aarch64/assembler/mod.hpp` exposes a named `AssembleRequest -> AssembleResult` text-first seam, plus a compatibility overload `assemble(const std::string&, const std::string&)`; the current stub returns staged text and reports `object_emitted = false`.
- target-local writer staging today: `src/backend/mir/aarch64/assembler/elf_writer.md` records legacy writer notes, but there is no current AArch64 ELF object writer implementation in this route.
- shared helper staging today:
  - `src/backend/asm_preprocess.cpp` is a mirrored placeholder for ref `asm_preprocess.rs`
  - `src/backend/asm_expr.cpp` is a mirrored placeholder for ref `asm_expr.rs`
  - `src/backend/elf/` contains real shared ELF support modules
  - `src/backend/elf_writer_common.cpp` is a mirrored placeholder for ref `elf_writer_common.rs`

This means the current compile-integrated contract is now:

- `PreparedBirModule -> AArch64 target module -> selected machine nodes -> .s printer output`
- `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s`

Later boundary work can narrow or replace that shape, but it should treat this
printer path as the current compatibility baseline only. It is not an internal
assembler, parser roundtrip, encoder, object writer, or linker: codegen-owned
semantics must flow through structured target MIR records and AArch64 machine
instruction nodes. A future built-in encoder or object writer must consume
those nodes or a lower structured encoding record derived from them, while
`parse_asm(...)` remains an external assembly input path.

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

This is not emitted by the current selected-machine-node printer route today.
It exists to pin the external-toolchain side of the AArch64 call relocation
contract that later assembler and linker work must preserve.

Expected object contract after external assembly:

- `.rela.text` section present
- global function symbol `main`
- undefined symbol `helper`
- call relocation `R_AARCH64_CALL26 helper`

Locked by CTest:

- `backend_contract_aarch64_extern_call_object`

## Current Shared Surfaces To Preserve

These are the current staged C++ headers later binary-utils work should include against instead of inventing local seams:

- `src/backend/backend.hpp`
- `src/backend/target.hpp`
- `src/backend/elf/mod.hpp`
- `src/backend/linker_common/mod.hpp`
- `src/backend/mir/aarch64/assembler/mod.hpp`
- `src/backend/mir/aarch64/linker/mod.hpp`

These headers are intentionally narrow. They stage the include boundary for later compile-integration work without implying that full built-in assembler or linker behavior is implemented in this plan.

## Immediate Follow-On Note

The AArch64 `.s` printer route fails closed for unsupported slices rather than
falling back to LLVM-generated assembly or treating printed text as backend
semantics. Later contract work should keep distinguishing:

- backend slices that already produce selected printable machine nodes
- cases that stop before selected printable machine nodes and therefore are not yet part of the assembly-object compatibility baseline
