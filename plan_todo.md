# AArch64 Global Int Pointer Difference Todo

Status: Active
Source Idea: ideas/open/13_backend_aarch64_global_int_pointer_diff_plan.md
Source Plan: plan.md

## Active Item

- Step 1: lock the exact bounded `global_int_pointer_diff` LIR and test contract before codegen changes

## Todo

- [ ] inspect the synthetic module builder and runtime case for `global_int_pointer_diff`
- [ ] tighten the synthetic backend test so the slice rejects LLVM IR fallback
- [ ] switch the runtime case to `BACKEND_OUTPUT_KIND=asm` for this slice
- [ ] implement the minimal AArch64 parser/emitter path in `src/backend/aarch64/codegen/emit.cpp`
- [ ] run targeted validation and update follow-on notes for the next pointer slice

## Completed

- [x] closed and archived `12_backend_aarch64_global_char_pointer_diff_plan.md`
- [x] selected `global_int_pointer_diff` as the next narrow child instead of reactivating the umbrella roadmap directly
- [x] created `ideas/open/13_backend_aarch64_global_int_pointer_diff_plan.md`
- [x] activated the new child into `plan.md` and `plan_todo.md`

## Next Intended Slice

- capture the exact scaled pointer-difference shape shared by `make_global_int_pointer_diff_module()` and `tests/c/internal/backend_case/global_int_pointer_diff.c`

## Blockers

- none yet

## Resume Notes

- keep this slice narrower than `global_int_pointer_roundtrip`
- preserve the current split: scaled pointer difference now, pointer round-tripping later
