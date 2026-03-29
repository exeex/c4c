# Backend X86 External Call Object Todo

Status: Active
Source Idea: ideas/open/33_backend_x86_external_call_object_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: Lock the failing x86 contract for `tests/c/internal/backend_case/call_helper.c`.

## Planned Items

- [ ] Step 2: Keep `call_helper.c` on the backend-owned x86 asm path with the smallest target-local change.
- [ ] Step 3: Prove object/relocation behavior and rerun the targeted plus full test suite.

## Completed Items

- [x] Confirmed from the umbrella roadmap that `ideas/open/33_backend_x86_external_call_object_plan.md` is the next explicitly staged child.
- [x] Materialized the missing child idea under `ideas/open/`.
- [x] Switched the active runbook away from the umbrella staging plan to the new bounded x86 child.

## Notes

- `tests/c/internal/InternalTests.cmake` already forces `backend_runtime_call_helper` onto `BACKEND_OUTPUT_KIND=asm`.
- The new execution target is the backend-owned x86 direct external-call object contract, not general linker bring-up.
- The most likely object-level reference point is an unresolved `helper` symbol with an x86-64 call relocation such as `R_X86_64_PLT32`, but execution should confirm this against Clang/objdump rather than assume it.

## Next Slice

- Add or enable the narrowest x86 contract test for `call_helper.c`, capture the current failure mode, and compare the backend-produced object against Clang before changing x86 codegen.
