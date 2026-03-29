# Built-in Assembler Boundary Plan

Status: Complete

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/04_backend_binary_utils_contract_plan.md`

Should precede:

- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`

## Goal

Decide and implement the internal boundary that the built-in assembler will consume so later target-specific object emission work stays structured and comparable to the existing backend output path, starting with include/build integration for the mirrored assembler tree.

## Simplified Staging Note

This is now less a separate product decision and more a staged integration problem:

- first make the mirrored assembler tree compile
- then settle the narrow text-vs-structured input boundary
- only then tighten actual encoding behavior

If execution shows those steps are inseparable, this idea can be folded into the AArch64 assembler plan.

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/asm_preprocess.rs`
- `ref/claudes-c-compiler/src/backend/asm_expr.rs`
- `ref/claudes-c-compiler/src/backend/elf/`
- `ref/claudes-c-compiler/src/backend/elf_writer_common.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/elf_writer.rs`

## Why This Is Separate

- syntax parsing and structured object emission are different design choices with long-term consequences
- deciding this boundary early prevents per-target assembler ports from each inventing their own input model
- the assembler input contract should stabilize before target-specific encoding work begins

## Ref Bias

- ref uses raw assembly text as the stable boundary between codegen and assembler
- parsing is still layered:
  shared preprocessing and expression handling first,
  then target-local typed parsing,
  then target-local encoding plus ELF object writing
- unless the C++ port has a strong reason to diverge, the default should be to preserve that text-first boundary so backend codegen stays comparable to ref output and the external toolchain fallback remains easy to keep alive during bring-up

## Scope

- make the mirrored shared and AArch64 assembler files include-clean enough to compile together
- decide whether the built-in assembler remains syntax-driven or consumes a more structured internal form
- define the data flow from current backend output into the future assembler entry point
- decide how much of ref's shared assembler surface should be mirrored as shared C++ code:
  preprocessing,
  expression evaluation,
  numeric-label handling,
  string parsing,
  and generic ELF writer support
- identify which existing backend text-emission assumptions must remain stable and which can be replaced
- explicitly choose whether target codegen continues to emit GNU-style textual assembly identical or near-identical to ref, or whether a second structured IR is introduced
- add narrow tests that lock the chosen assembler input boundary

## Acceptance Stages

### Stage 1: Assembler tree compiles

- shared assembler helpers and AArch64 assembler files can be included together and built

### Stage 2: Boundary is explicit

- one clear assembler entry boundary is chosen and declared in code

### Stage 3: Backend can target that boundary

- AArch64 backend output can be threaded toward the chosen assembler entry point without local one-off adapters

## Decision Rules

- prefer a text-first boundary if it preserves external-toolchain comparability and keeps the AArch64 port closer to ref
- only introduce a structured internal assembler IR if it clearly reduces duplicated parser complexity across targets without forcing the backend to maintain two divergent output forms
- keep shared pieces shared only where ref already shows commonality:
  `asm_preprocess.rs`,
  `asm_expr.rs`,
  shared ELF helpers,
  and generic writer support
- avoid inventing a target-generic instruction IR that ref itself does not rely on

## Suggested Execution Order

1. pin down the minimal assembly-text subset currently emitted by the AArch64 backend
2. decide whether to keep ref's text-first `parse -> encode -> write ELF` pipeline
3. carve out shared assembler helpers analogous to `asm_preprocess.rs`, `asm_expr.rs`, and common ELF-writing support
4. define the target-local assembler entry point around that boundary
5. only then begin target-specific instruction encoding work in the AArch64 assembler plan

## Explicit Non-Goals

- full instruction encoding for any target
- linker work
- broad backend redesign beyond the assembler entry boundary

## Validation

- first validation gate: mirrored assembler files compile and share one coherent declaration surface
- second validation gate: the chosen assembler input model is explicit and checked into the repo
- follow-on assembler work can target a stable interface instead of open-ended design notes

## Good First Patch

Make the mirrored assembler tree compile, then expose one narrow assembler entry contract for one existing backend-emitted function.

## Completion Notes

- The mirrored assembler tree now compiles behind explicit contract headers, with the text-first `AssembleRequest -> AssembleResult` seam staged in `src/backend/aarch64/assembler/mod.hpp`.
- `src/backend/aarch64/codegen/emit.hpp` now exposes `assemble_module(const LirModule&, output_path)` so one production AArch64 backend path emits assembly text and forwards it through the named assembler seam without a test-local adapter.
- The staged assembler result now preserves `output_path` alongside `staged_text`, which keeps the current stub seam observable until real object emission lands.
- Repo-local contract notes in `src/backend/aarch64/BINARY_UTILS_CONTRACT.md` and backend adapter tests lock the text-first boundary and the production handoff helper.

## Leftover Issues

- Full-suite baseline remains unchanged at four unrelated failures:
  - `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  - `cpp_positive_sema_decltype_bf16_builtin_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_llvm_initializer_list_runtime_materialization`
- Actual AArch64 object emission, parser coverage, and shared assembler helper behavior remain follow-on work for `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`.
