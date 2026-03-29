# Built-in AArch64 Linker Todo

Status: Active
Source Idea: ideas/open/08_backend_builtin_linker_aarch64_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Lock The First Static-Link Slice

## In Progress

- New plan activated from `ideas/open/08_backend_builtin_linker_aarch64_plan.md`; implementation has not started yet.

## Next Slice

- Inspect the current `src/backend/aarch64/linker/` surfaces and tests, then name the first bounded relocation-bearing multi-object static-link case before any code changes.

## Remaining Items

- [ ] Step 1: Lock The First Static-Link Slice
- [ ] Step 2: Connect Shared Input Loading To AArch64 Linker Entry Points
- [ ] Step 3: Apply The First AArch64 Static-Link Path

## Completed

- [x] Closed `ideas/open/07_backend_linker_object_io_plan.md` after finishing the first shared object/archive IO slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/08_backend_builtin_linker_aarch64_plan.md` into the active `plan.md` runbook.

## Blockers

- None yet.

## Resume Notes

- Keep the first linker slice static-link-first and relocation-bounded; do not absorb dynamic-linking or broad archive policy work into this runbook.
- Reuse the shared object/archive IO layer that now lives in `src/backend/elf/` and `src/backend/linker_common/` rather than adding AArch64-local input parsing.
