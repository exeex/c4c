# x86 Global Addressing Todo

Status: Active
Source Idea: ideas/open/18_backend_x86_global_addressing_plan.md
Source Plan: plan.md

## Active Item

- [x] Step 1: choose the narrowest promotable global-addressing case and record the exact first slice for implementation
- [x] Step 2: tighten one backend adapter or emitter test so the chosen seam stops accepting fallback
- [x] Step 3: implement the bounded RIP-relative base-address formation plus one indexed-load seam
- [x] Step 4: promote the chosen runtime case through `BACKEND_OUTPUT_KIND=asm`
- [x] Step 5: run full validation, compare regression logs, and record the next follow-on slice

## Planned Queue

- [ ] Preserve the next bounded x86 global-addressing slice as a separate follow-on without widening this runbook

## Completed

- [x] Activated `ideas/open/18_backend_x86_global_addressing_plan.md` into the active `plan.md`
- [x] Chose `tests/c/internal/backend_case/string_literal_char.c` as the smallest remaining x86 global-addressing slice because `backend_runtime_string_literal_char` still falls back to LLVM text while `backend_runtime_extern_global_array` already passes through the x86 asm path
- [x] Added x86 backend adapter coverage that requires `.rodata` string-pool emission, RIP-relative base materialization, and indexed byte load/sign-extension for `string_literal_char`
- [x] Implemented the minimal x86 string-pool slice in `src/backend/x86/codegen/emit.cpp` and promoted `backend_runtime_string_literal_char` through the asm backend path
- [x] Full-suite regression guard passed with `test_before.log` -> `test_after.log`: passed `2317 -> 2318`, failed `22 -> 21`, no newly failing tests

## Next Intended Slice

Keep the current runbook bounded and treat `global_int_pointer_diff` / `global_int_pointer_roundtrip` as separate follow-on slices unless a smaller backend-owned global-addressing seam is identified first.

## Blockers

- None recorded during activation.

## Resume Notes

- Targeted validation now passes for `backend_lir_adapter_tests`, `backend_runtime_string_literal_char`, and `backend_runtime_extern_global_array`.
- Regression guard summary: `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` => PASS with `backend_runtime_string_literal_char` resolved and zero new failing tests.
- `21_backend_x86_global_int_pointer_diff_plan.md` and `22_backend_x86_global_int_pointer_roundtrip_plan.md` remain follow-on work and should not be folded into this plan.
- `23_backend_builtin_assembler_x86_plan.md` and `24_backend_builtin_linker_x86_plan.md` are separate later-stage initiatives.
