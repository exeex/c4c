# Built-in AArch64 Linker Todo

Status: Active
Source Idea: ideas/open/08_backend_builtin_linker_aarch64_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Connect Shared Input Loading To AArch64 Linker Entry Points

## In Progress

- Step 2 slice now routes the bounded two-object `CALL26` case through `load_first_static_input_objects(...)` in `src/backend/aarch64/linker/input.cpp`, and `inspect_first_static_link_slice(...)` consumes that seam instead of reading object bytes directly in `link.cpp`.

## Next Slice

- Add a narrow backend test that exercises the new AArch64 object-loading seam directly on the same caller/helper two-object `CALL26` fixture, then re-point `inspect_first_static_link_slice(...)` to that seam without widening into archive policy yet.

## Remaining Items

- [x] Step 1: Lock The First Static-Link Slice
- [ ] Step 2: Connect Shared Input Loading To AArch64 Linker Entry Points
- [ ] Step 3: Apply The First AArch64 Static-Link Path

## Completed

- [x] Closed `ideas/open/07_backend_linker_object_io_plan.md` after finishing the first shared object/archive IO slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/08_backend_builtin_linker_aarch64_plan.md` into the active `plan.md` runbook.
- [x] Named the first bounded static-link slice as a two-object AArch64 `CALL26` case (`main -> helper_ext`) and pinned its expected input-loading, symbol-resolution, relocation, and merged `.text` outputs with a new linker inspection API plus backend unit coverage.
- [x] Added a real Step 2 object-loading seam in `src/backend/aarch64/linker/input.cpp` plus `test_aarch64_linker_loads_first_static_objects_through_shared_input_seam()`, keeping the first static-link slice on shared ELF parsing while separating input loading from link orchestration.

## Blockers

- None yet.

## Resume Notes

- Keep the first linker slice static-link-first and relocation-bounded; do not absorb dynamic-linking or broad archive policy work into this runbook.
- Reuse the shared object/archive IO layer that now lives in `src/backend/elf/` and `src/backend/linker_common/` rather than adding AArch64-local input parsing.
- The current slice lives in `src/backend/aarch64/linker::inspect_first_static_link_slice(...)` and `test_aarch64_linker_names_first_static_call26_slice()`, which should stay as the bounded contract while Step 2 introduces explicit shared input-loading seams.
- The next Step 2 cut can widen from object-only loading toward bounded archive-aware entry points, but only if the same caller/helper slice still remains the contract and no dynamic-link policy leaks in.
