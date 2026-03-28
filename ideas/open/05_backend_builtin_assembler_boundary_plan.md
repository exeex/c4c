# Built-in Assembler Boundary Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/04_backend_binary_utils_contract_plan.md`

Should precede:

- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`

## Goal

Decide and implement the internal boundary that the built-in assembler will consume so later target-specific object emission work stays structured and comparable to the existing backend output path.

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

- the chosen assembler input model is explicit and checked into the repo
- the plan names which ref shared helpers should become shared C++ helpers and which remain target-local
- follow-on assembler work can target a stable interface instead of open-ended design notes
- the first target-specific assembler plan can begin without revisiting the input-format decision

## Good First Patch

Prototype the narrowest assembler entry contract for one existing backend-emitted function and lock whether the new path consumes textual assembly or a structured internal representation.
