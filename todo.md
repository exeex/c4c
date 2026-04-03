# BIR Cutover And Legacy Cleanup Todo

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Source Plan: plan.md

## Active Item

- Step 4: Clean up surviving names
- Current slice: inventory the highest-signal surviving transitional
  `backend_ir` names and choose the smallest rename surface that moves the
  surviving backend path closer to HIR -> LIR -> BIR -> target emission

## Todo

- [x] Step 1: Establish the cutover surface
- [x] Step 2: Flip the default to BIR
- [x] Step 3: Remove legacy backend plumbing
- [ ] Step 4: Clean up surviving names
- [ ] Step 5: Finish the backend test migration

## Completed

- [x] activated the reopened backend cutover idea into the new single-plan lifecycle
- [x] identified the LIR-entry seam and production backend route surface that
      controlled BIR-vs-legacy behavior
- [x] switched the production backend flow to probe `try_lower_to_bir(...)`
      first and default supported slices to `BackendPipeline::Bir`
- [x] kept unsupported slices on the bounded legacy fallback path
- [x] expanded `lower_to_bir(...)` from tiny constant-only cases to the current
      straight-line single-block affine i32 slice family with up to two
      parameters
- [x] added targeted route and runtime coverage for the bounded affine BIR
      slices on RISC-V, x86, and AArch64 paths
- [x] removed the dead terminal AArch64 `legacy_fallback` branch for lowered
      backend text fallback
- [x] removed x86 lowered call-crossing dependence on shared regalloc over
      attached `legacy_fallback`
- [x] removed AArch64 lowered call-crossing dependence on consulting attached
      `legacy_fallback` for shared regalloc inputs
- [x] refined the RISC-V pre-lowered passthrough seam so BIR-owned backend
      modules keep their backend text surface even when stale legacy metadata is
      attached
- [x] kept targeted backend validation and full-suite regression checks
      monotonic through the landed cutover slices
- [x] closed Step 3 on the basis that the remaining x86 and AArch64
      legacy-fallback seams have already been reduced to bounded, intentional
      residual surfaces, while the current RISC-V passthrough seam is deferred
      until real RISC-V backend work is in scope

## Next Slice

- inventory the largest remaining user-facing and backend-internal
  `backend_ir` names
- separate true Step 4 rename targets from intentionally retained compatibility
  shims
- pick the narrowest rename slice that can land with targeted backend test
  updates and no semantic drift
- before doing broad Step 4 renames, optionally finish the deferred AArch64 seam
  cleanup by porting the real `extern_global_array` production route off the
  remaining `legacy_fallback` early return

## Blockers

- `lower_to_bir(...)` still does not cover control flow, loads/stores, globals,
  or alloca-backed slices, so some unsupported modules still require legacy
  fallback behavior
- Step 4 and Step 5 are still pending after the BIR-default cutover landed

## Notes

- do not use the current RISC-V passthrough fallback as a reason to keep Step 3
  open; defer that seam until real RISC-V backend work is in scope
- the remaining AArch64 nonminimal-lowering seam is no longer treated as a
  Step 3 blocker; any later cleanup there should be handled only if it is
  required by a concrete Step 4 or Step 5 slice
- concrete AArch64 seam experiment result:
  removing the `needs_nonminimal_lowering && legacy_fallback` early return in
  `src/backend/aarch64/codegen/emit.cpp` and running
  `ctest --test-dir build -R backend --output-on-failure` left 289/290 backend
  tests passing
- the only failing case was `backend_contract_aarch64_extern_global_array_object`
  on the real `c4cll --codegen asm` route
- failure mode: output lost `.extern ext_arr` and dropped to the generic GOT
  extern-global load sequence instead of the structured extern-global-array fast
  path
- handoff target if this cleanup is resumed:
  keep the early return removed and make the production-route
  `extern_global_array` slice lower onto the structured
  `parse_minimal_extern_global_array_load_slice(const BackendModule&)` path so
  `emit_minimal_extern_global_array_load_asm(...)` still wins
