# AArch64 Return Zero Runtime Promotion Todo

Status: Active
Source Idea: ideas/open/19_backend_aarch64_return_zero_runtime_promotion_plan.md
Source Plan: plan.md

## Current Active Item

- Step 1: Confirm that `return_zero` already matches the existing bounded AArch64 constant-return asm seam

## Incomplete Items

- [ ] Inspect the runtime-emitted LIR for `tests/c/internal/backend_case/return_zero.c`
- [ ] Confirm whether the existing AArch64 constant-return matcher/emitter already covers the runtime shape
- [ ] Promote `return_zero` to `BACKEND_OUTPUT_KIND=asm`
- [ ] Run targeted backend/runtime validation for `return_zero`
- [ ] Run full regression validation and compare before/after results

## Completed Items

- [x] Closed and archived `ideas/closed/18_backend_aarch64_nested_param_member_array_plan.md`
- [x] Derived the next narrow child idea from the remaining backend runtime promotion gap
- [x] Activated `ideas/open/19_backend_aarch64_return_zero_runtime_promotion_plan.md` into `plan.md`

## Next Intended Slice

- Inspect the runtime LIR for `return_zero`, confirm it matches the existing constant-return emitter seam, and only then flip the runtime harness to asm.

## Blockers

- None recorded.

## Resume Notes

- `ideas/open/__backend_port_plan.md` is the only umbrella file left in `ideas/open/`; do not reactivate it directly while child `19` is active.
- `return_zero` currently appears to be the only non-skipped backend runtime case still using the default LLVM-IR output path.
