# Built-in AArch64 Linker Todo

Status: Active
Source Idea: ideas/open/08_backend_builtin_linker_aarch64_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Apply The First AArch64 Static-Link Path

## In Progress

- Step 3 now targets the first real static-link execution cut on the bounded `CALL26` slice: resolve `main -> helper_ext`, apply only the required relocation subset, and emit the smallest stable executable surface needed for tests.

## Next Slice

- Add the narrowest linker test that drives the bounded `CALL26` slice past inspection into real relocation application and minimal static ELF emission, while keeping archive policy and dynamic-link behavior out of scope.

## Remaining Items

- [x] Step 1: Lock The First Static-Link Slice
- [x] Step 2: Connect Shared Input Loading To AArch64 Linker Entry Points
- [ ] Step 3: Apply The First AArch64 Static-Link Path

## Completed

- [x] Closed `ideas/open/07_backend_linker_object_io_plan.md` after finishing the first shared object/archive IO slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/08_backend_builtin_linker_aarch64_plan.md` into the active `plan.md` runbook.
- [x] Named the first bounded static-link slice as a two-object AArch64 `CALL26` case (`main -> helper_ext`) and pinned its expected input-loading, symbol-resolution, relocation, and merged `.text` outputs with a new linker inspection API plus backend unit coverage.
- [x] Added a real Step 2 object-loading seam in `src/backend/aarch64/linker/input.cpp` plus `test_aarch64_linker_loads_first_static_objects_through_shared_input_seam()`, keeping the first static-link slice on shared ELF parsing while separating input loading from link orchestration.
- [x] Extended the Step 2 input seam to accept a bounded helper archive through shared archive parsing, and added `test_aarch64_linker_loads_first_static_objects_from_archive_through_shared_input_seam()` to prove the same `CALL26` slice still resolves into two loaded object surfaces.

## Blockers

- None yet.

## Resume Notes

- Keep the first linker slice static-link-first and relocation-bounded; do not absorb dynamic-linking or broad archive policy work into this runbook.
- Reuse the shared object/archive IO layer that now lives in `src/backend/elf/` and `src/backend/linker_common/` rather than adding AArch64-local input parsing.
- The bounded contract still lives in `src/backend/aarch64/linker::inspect_first_static_link_slice(...)` and `test_aarch64_linker_names_first_static_call26_slice()`, but Step 2 now also allows the helper definition to arrive from a one-member archive through `load_first_static_input_objects(...)`.
- Step 3 should build on the same caller/helper contract and reuse the new object-plus-archive seam only as needed; avoid widening into general archive extraction policy until the first relocation-and-emission slice is proven.
