# Backend Linker Object IO Todo

Status: Active
Source Idea: ideas/open/07_backend_linker_object_io_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Add The First Archive IO Slice

## In Progress

- Step 2 is now closed: the shared ELF constants, object types, and `parse_elf64_object()` surface cover both the bounded relocation-bearing fixture and the current built-in AArch64 emitted object path; the active work can move to the first bounded archive-member parsing slice.

## Next Slice

- Add a narrow archive test first, then parse one bounded single-member archive into shared archive/member inventory without widening into full linker orchestration.

## Remaining Items

- [x] Step 1: Lock The Minimal Shared Object IO Slice
- [x] Step 2: Parse One Real Relocatable Object Through Shared Types
- [ ] Step 3: Add The First Archive IO Slice

## Completed

- [x] Closed `ideas/open/06_backend_builtin_assembler_aarch64_plan.md` after completing the first built-in AArch64 object-emission slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/07_backend_linker_object_io_plan.md` into the active `plan.md` runbook.
- [x] Added shared ELF constants, linker-common object types, and a bounded `parse_elf64_object()` surface with backend coverage for `.text`, `.rela.text`, `.symtab`, `.strtab`, `.shstrtab`, symbol inventory, and relocation inventory.
- [x] Proved the shared object parser against the current built-in AArch64 emitted object path by parsing the bounded `return_add` object emitted through `assemble_module()`.

## Blockers

- None yet.

## Resume Notes

- This runbook is shared-linker infrastructure only; do not widen into relocation application or executable layout.
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md` depends on this shared object/archive IO layer and should stay inactive until these shared seams are explicit.
- The next slice should stay in `src/backend/elf/` and `src/backend/linker_common/` by adding the first bounded archive-member parsing proof, not new target-local linker policy.
