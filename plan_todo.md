# Backend Linker Object IO Todo

Status: Active
Source Idea: ideas/open/07_backend_linker_object_io_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Lock The Minimal Shared Object IO Slice

## In Progress

- Read the shared ELF and linker-common surfaces to identify the first bounded relocatable object fixture and the exact parser outputs later linker work will need.

## Next Slice

- Keep the first object-IO slice narrow: one relocatable object with explicit section, symbol, and relocation inventory checks before archive work begins.

## Remaining Items

- [ ] Step 1: Lock The Minimal Shared Object IO Slice
- [ ] Step 2: Parse One Real Relocatable Object Through Shared Types
- [ ] Step 3: Add The First Archive IO Slice

## Completed

- [x] Closed `ideas/open/06_backend_builtin_assembler_aarch64_plan.md` after completing the first built-in AArch64 object-emission slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/07_backend_linker_object_io_plan.md` into the active `plan.md` runbook.

## Blockers

- None yet.

## Resume Notes

- This runbook is shared-linker infrastructure only; do not widen into relocation application or executable layout.
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md` depends on this shared object/archive IO layer and should stay inactive until these shared seams are explicit.
