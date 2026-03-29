# Backend AArch64 Global Int Pointer Roundtrip Todo

Status: Active
Source Idea: ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md
Source Plan: plan.md

## Current Active Item

- Step 1: reconstruct the exact bounded AArch64 global-int pointer-roundtrip seam from existing repo and runtime-emitted LIR context

## Todo

- [ ] inspect current backend/AArch64 sources, tests, and fallback output for `global_int_pointer_roundtrip`
- [ ] write down the exact bounded matcher shape and files expected to change
- [ ] add or update the narrowest targeted failing test for this seam
- [ ] implement the minimal backend change
- [ ] run targeted and broader validation

## Completed

- [x] switched away from the stale umbrella-derived `global_int_pointer_diff` runbook after confirming that slice is already implemented and archived in `ideas/closed/13_backend_aarch64_global_int_pointer_diff_plan.md`
- [x] updated `ideas/open/__backend_port_plan.md` so the roadmap now points at `ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md`
- [x] activated a new active plan from `ideas/open/14_backend_aarch64_global_int_pointer_roundtrip_plan.md`

## Next Intended Slice

- Compare the runtime-emitted LIR for `tests/c/internal/backend_case/global_int_pointer_roundtrip.c` against `make_global_int_pointer_roundtrip_module()` and identify the smallest matcher that can replace LLVM IR fallback.

## Blockers

- None yet.

## Resume Notes

- The previous active runbook was stale: the `global_int_pointer_diff` code, tests, and closed child idea already exist in the tree.
- `global_int_pointer_roundtrip` still falls back to LLVM IR in `tests/backend/backend_lir_adapter_tests.cpp`, so this child remains a distinct unfinished seam.
