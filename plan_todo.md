# AArch64 Extern Global Array Addressing Todo

Status: Active
Source Idea: ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md
Source Plan: plan.md

## Active Item

- Step 1: confirm the exact extern global array decay plus indexed element-load slice and decide whether a bounded runtime case exists

## Todo

- [ ] inspect the synthetic backend fixture and runtime candidates for the smallest extern-array address-formation seam
- [ ] capture the relevant AArch64 LIR or LLVM IR shape for the chosen case
- [ ] decide whether one bounded runtime case can be promoted without widening scope
- [ ] tighten targeted backend tests for the explicit extern-array base-address plus indexed-load seam
- [ ] implement the minimal AArch64 backend lowering for the chosen seam
- [ ] promote one runtime case through `BACKEND_OUTPUT_KIND=asm` or explicitly defer runtime promotion
- [ ] run targeted validation and broader regression checks

## Completed

- [x] activated `ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md` into `plan.md`

## Next Intended Slice

- compare `make_extern_global_array_load_module()` against real frontend-emitted candidates and lock the exact bounded seam before changing tests or lowering

## Blockers

- none recorded

## Resume Notes

- the completed string-literal slice lives in `ideas/closed/10_backend_aarch64_global_addressing_plan.md`
- keep this runbook narrower than general pointer arithmetic and split out pointer-difference or round-trip work if it appears during execution
