# Backend Binary Utils Contract Todo

Status: Active
Source Idea: ideas/open/04_backend_binary_utils_contract_plan.md
Source Plan: plan.md

## Active Item

- Step 4: Stage Shared Contract Surfaces

## In Progress

- Audit whether any later binary-utils trees still need shared declarations promoted out of local implementation files.
- Check whether the current include-reachable `elf/`, `linker_common/`, `aarch64/assembler/`, and `aarch64/linker/` surfaces are enough for compile integration.
- Avoid adding declaration churn unless a concrete later consumer would otherwise need to duplicate a contract.

## Next Slice

- Review the current AArch64 assembler/linker trees against the new contract note and decide if any bounded header or type seam should be extracted now.

## Remaining Items

- [x] Step 1: Inventory The Active Toolchain Boundary
- [x] Step 2: Capture Representative Baseline Cases
- [x] Step 3: Document Assembly And Object Contract Details
- [ ] Step 4: Stage Shared Contract Surfaces
- [x] Step 5: Lock Golden Validation

## Completed

- [x] Activated `ideas/open/04_backend_binary_utils_contract_plan.md` into the active `plan.md` runbook.
- [x] Created `plan_todo.md` aligned to the same source idea and queued the first execution slice.
- [x] Mapped the current C++ AArch64 boundary: `c4cll` currently stops at backend-emitted assembly text, while `.s -> .o` validation happens through test harnesses using external `clang`.
- [x] Added a repo-local AArch64 binary-utils baseline note at `src/backend/aarch64/BINARY_UTILS_CONTRACT.md`.
- [x] Locked a minimal backend-emitted single-object object contract for `return_add.c`.
- [x] Locked a current-backend relocation-bearing object contract for `global_load.c`, including the `ADRP` + `LDST32_LO12` relocation pair against `g_counter`.
- [x] Added an external-call relocation fixture that pins `R_AARCH64_CALL26 helper` for later assembler/linker work.
- [x] Added repeatable object-contract CTests for the minimal single-object case, the current-backend global relocation case, and the external-call relocation fixture.
- [x] Re-ran the full suite after the patch: total tests increased from 574 to 577, with the same four pre-existing failures and no newly failing tests.

## Blockers

- None recorded yet.

## Resume Notes

- Keep this plan strictly scoped to contract capture, shared declaration staging, and golden validation.
- If execution turns into built-in assembler or linker implementation work, write that as a separate initiative instead of expanding this runbook.
- Current AArch64 fallback coverage is still partial: unsupported slices may return LLVM IR instead of GNU-style assembly. Keep the contract baseline limited to cases that actually cross the assembly/object seam today.
