# Built-in Assembler Boundary Todo

Status: Active
Source Idea: ideas/open/05_backend_builtin_assembler_boundary_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Inventory The Current Assembler Entry Shape

## In Progress

- Audit the existing `src/backend/aarch64/assembler/` headers and stubbed modules against the ref text-first assembler flow.
- Identify which shared seams from `asm_preprocess.rs`, `asm_expr.rs`, `elf/`, and `elf_writer_common.rs` already have staged C++ counterparts.
- Confirm the exact backend-emitted assembly subset that the current AArch64 tests already lock.

## Next Slice

- Read the current AArch64 codegen and assembler entry points side by side, then decide whether the first concrete boundary patch should be a text-first assembler entry header or a tighter shared-helper extraction.

## Remaining Items

- [ ] Step 1: Inventory The Current Assembler Entry Shape
- [ ] Step 2: Decide And Declare The Boundary
- [ ] Step 3: Prove Compile Integration

## Completed

- [x] Activated `ideas/open/05_backend_builtin_assembler_boundary_plan.md` into the active `plan.md` runbook.
- [x] Created `plan_todo.md` aligned to the same source idea and queued the first execution slice.

## Blockers

- None recorded yet.

## Resume Notes

- Keep this runbook focused on assembler boundary selection and compile integration, not full object emission.
- Use the staged contract from `src/backend/aarch64/BINARY_UTILS_CONTRACT.md` as the compatibility baseline.
- Split out any linker-facing or broad target-generic IR work instead of mutating this plan.
