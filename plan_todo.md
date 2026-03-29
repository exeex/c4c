# Backend Linker Object IO Todo

Status: Active
Source Idea: ideas/open/07_backend_linker_object_io_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Parse One Real Relocatable Object Through Shared Types

## In Progress

- The shared ELF constants, object types, and `parse_elf64_object()` surface now parse one bounded relocation-bearing ELF64 object fixture with explicit section, symbol, and relocation inventory checks; widen the next slice to current emitter-produced objects without broadening into archive work yet.

## Next Slice

- Reuse the new shared parser against the current built-in AArch64 object-emission path so Step 2 proves the shared object surface works on emitted objects before shifting to archive member parsing.

## Remaining Items

- [x] Step 1: Lock The Minimal Shared Object IO Slice
- [ ] Step 2: Parse One Real Relocatable Object Through Shared Types
- [ ] Step 3: Add The First Archive IO Slice

## Completed

- [x] Closed `ideas/open/06_backend_builtin_assembler_aarch64_plan.md` after completing the first built-in AArch64 object-emission slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/07_backend_linker_object_io_plan.md` into the active `plan.md` runbook.
- [x] Added shared ELF constants, linker-common object types, and a bounded `parse_elf64_object()` surface with backend coverage for `.text`, `.rela.text`, `.symtab`, `.strtab`, `.shstrtab`, symbol inventory, and relocation inventory.

## Blockers

- None yet.

## Resume Notes

- This runbook is shared-linker infrastructure only; do not widen into relocation application or executable layout.
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md` depends on this shared object/archive IO layer and should stay inactive until these shared seams are explicit.
- The next object-parsing slice should pivot from the bounded ELF fixture to current emitted objects so later linker work consumes the same object surface the backend already writes.
