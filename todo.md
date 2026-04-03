# BIR Cutover And Legacy Cleanup Todo

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Source Plan: plan.md

## Active Item

- Step 5: Finish the backend test migration
- Current slice: identify the highest-value backend test surface that still
  frames the pipeline in transitional backend-IR terms and convert it to the
  BIR-oriented layered story without widening scope

## Todo

- [x] Step 1: Establish the cutover surface
- [x] Step 2: Flip the default to BIR
- [x] Step 3: Remove legacy backend plumbing
- [x] Step 4: Clean up surviving names
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
- [x] renamed the user-facing backend printer and validator APIs to
      `print_backend_module(...)` / `validate_backend_module(...)`, updated
      backend callers and tests, and isolated the old `*_backend_ir` spellings
      behind explicit compatibility shims
- [x] renamed the shared/public lowering entrypoints to
      `lower_lir_to_backend_module(...)` /
      `lower_bir_to_backend_module(...)`, updated production callers, and kept
      the old `lower_to_backend_ir(...)` spellings only as explicit lowering
      compatibility shims
- [x] moved the three `backend_lir_adapter*` test binaries off the
      `lower_to_backend_ir(...)` compatibility shim onto
      `lower_lir_to_backend_module(...)`
- [x] moved the remaining stable backend test surfaces off the
      `lower_to_backend_ir(...)` compatibility shim by switching
      `tests/backend/backend_ir_tests.cpp` to
      `lower_lir_to_backend_module(...)` and the BIR pipeline coverage to
      `lower_bir_to_backend_module(...)`
- [x] re-ran targeted backend validation plus a clean full-suite regression
      pass with no new failures relative to the existing EASTL recipe baseline

## Next Slice

- start Step 5 by auditing `tests/backend/*` and `tests/c/internal/backend_*`
  for coverage that still treats transitional backend-IR compatibility shims as
  the primary contract, then move one narrow surface to the BIR-first layered
  story with targeted validation
- keep the deferred AArch64 `extern_global_array` production-route cleanup out
  of scope unless it blocks a concrete Step 5 backend test migration slice

## Blockers

- `lower_to_bir(...)` still does not cover control flow, loads/stores, globals,
  or alloca-backed slices, so some unsupported modules still require legacy
  fallback behavior
- Step 5 is still pending after the BIR-default cutover landed

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
- current Step 4 slice is intentionally narrow: user-facing
  `print_backend_ir(...)` / `validate_backend_ir(...)` naming first, broader
  lowering-entrypoint renames later
- regression check for the printer/validator rename slice:
  `backend_ir_tests` and `backend_contract_aarch64_extern_global_array_object`
  passed after the rename, and `test_before.log` vs `test_after.log` stayed
  flat at 2720 passing / 6 failing with the same EASTL recipe failures
- regression check for the lowering-entrypoint rename slice:
  `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`,
  `backend_lir_adapter_x86_64_tests`, `backend_ir_tests`, and
  `backend_bir_tests` all passed after the rename; the full suite finished at
  2720 passing / 6 failing with the same EASTL recipe failures recorded in the
  existing baseline notes
- regression check for the adapter-test rename follow-up:
  `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, and
  `backend_lir_adapter_x86_64_tests` passed after switching to
  `lower_lir_to_backend_module(...)`; `test_fail_before.log` vs
  `test_fail_after.log` stayed flat at 2720 passing / 6 failing with the same
  EASTL recipe failures
- regression check for the stable backend-test rename follow-up:
  `backend_ir_tests` and `backend_bir_tests` passed after switching the stable
  test surfaces to `lower_lir_to_backend_module(...)` /
  `lower_bir_to_backend_module(...)`; `test_fail_before.log` vs `test_after.log`
  stayed flat at 2720 passing / 6 failing with the same EASTL recipe failures
