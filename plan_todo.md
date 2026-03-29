# AArch64 Local Memory Addressing Todo

Status: Active
Source Idea: ideas/open/09_backend_aarch64_local_memory_addressing_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Pin The First Local-Array Contract

## In Progress

- Inspect the current `local_array.c` frontend-emitted AArch64 LIR and isolate the narrowest synthetic backend shape that captures stack-local base-address formation plus indexed local access.

## Next Slice

- Add the smallest synthetic adapter/emitter test that names the exact local-array addressing seam before promoting the runtime case onto the AArch64 asm path.

## Remaining Items

- [ ] Step 1: Pin The First Local-Array Contract
- [ ] Step 2: Make The Backend-Owned Local Addressing Seam Explicit
- [ ] Step 3: Promote The Runtime Case Through AArch64 Asm

## Completed

- [x] Closed `ideas/open/08_backend_builtin_linker_aarch64_plan.md` after landing the first bounded AArch64 built-in linker slice and moving it to `ideas/closed/`.
- [x] Activated `ideas/open/09_backend_aarch64_local_memory_addressing_plan.md` into the active `plan.md` runbook.

## Blockers

- None yet.

## Resume Notes

- Keep this runbook scoped to stack-local integer array addressing; do not absorb global-addressing or linker work.
- Start from the exact `local_array.c` LIR rather than inventing a broader local-memory abstraction up front.
- If the first bounded implementation pulls in wider pointer or aggregate lowering, split a new idea instead of silently expanding this plan.
