# Plan Todo

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Source Plan: plan.md

## Active Item

- Step 3: Continue trimming live legacy fallback plumbing after refining the
  RISC-V passthrough seam so BIR-lowerable pre-lowered modules ignore attached
  legacy LLVM text while unsupported slices still keep the legacy text
  fallback.

## In Progress

- Trace the remaining AArch64 nonminimal-lowering branch in
  `src/backend/aarch64/codegen/emit.cpp` and identify the smallest consumer
  that still requires attached `legacy_fallback` for unsupported slices.

## Pending

- Step 4: Clean up surviving transitional `backend_ir` naming.
- Step 5: Finish backend test migration around the BIR-first path.

## Completed

- Activated the runbook from
  `ideas/open/06_bir_cutover_and_legacy_cleanup.md`.
- Step 1 cutover-surface inventory:
  `src/backend/backend.hpp` still defaults `BackendOptions.pipeline` to
  `BackendPipeline::Legacy`.
- `src/backend/backend.cpp` only consults `options.pipeline` on the LIR entry
  path where `BackendModuleInput.module == nullptr`; once callers pass a
  lowered backend module, routing no longer branches on BIR-vs-legacy.
- `src/codegen/llvm/llvm_codegen.cpp` currently builds default backend options
  and never opts into the BIR pipeline, so production codegen still enters the
  legacy route by default.
- Added backend pipeline tests that pin the LIR-entry seam as the only current
  BIR selection surface.
- Added internal backend route tests that pin the real `c4cll --codegen asm`
  entrypoint:
  supported `return_add` now defaults to BIR text on RISC-V, while unsupported
  `global_load` still falls back to legacy LLVM IR.
- Updated `src/codegen/llvm/llvm_codegen.cpp` so the production backend path
  probes `try_lower_to_bir(...)` first, selects `BackendPipeline::Bir` for
  supported slices, and otherwise retains the temporary legacy fallback path.
- Kept `--codegen compare` stable during the cutover by leaving compare-mode
  output on the legacy text path for now.
- Targeted validation passed:
  `backend_codegen_route_riscv64_return_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_global_load_falls_back_to_llvm`,
  `backend_bir_tests`, `backend_runtime_return_add`, and
  `backend_runtime_global_load`.
- Full-suite regression guard passed with zero new failures:
  `test_fail_before.log` vs `test_fail_after.log`.
- Broadened `lower_to_bir(...)` from one-op tiny slices to straight-line
  single-block i32 add/sub chains with def-before-use checks on named values.
- Added backend BIR unit coverage for an add/sub chain at both the lowering and
  BIR-pipeline seams.
- Added internal production-route coverage for
  `backend_codegen_route_riscv64_return_add_sub_chain_defaults_to_bir` plus the
  matching `backend_runtime_return_add_sub_chain` execution check.
- Targeted validation passed:
  `backend_bir_tests`,
  `backend_codegen_route_riscv64_return_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_add_sub_chain_defaults_to_bir`,
  `backend_codegen_route_riscv64_global_load_falls_back_to_llvm`,
  `backend_runtime_return_add`,
  `backend_runtime_return_add_sub_chain`, and
  `backend_runtime_global_load`.
- Full-suite regression remained monotonic:
  `test_before.log` -> `test_after.log` increased total tests from `2672` to
  `2674` with zero newly failing cases.
- Extended `try_lower_to_bir(...)` and the BIR data model so the BIR-first
  production route now accepts one-parameter straight-line single-block i32
  add/sub chains in addition to constant-only slices.
- Added backend BIR coverage for the new bounded parameter-fed slice across
  lowering, RISC-V route text, x86 asm emission, and AArch64 asm emission.
- Added the production-route test
  `backend_codegen_route_riscv64_single_param_add_sub_chain_defaults_to_bir`
  using `tests/c/internal/backend_route_case/single_param_add_sub_chain.c`.
- Targeted validation passed:
  `backend_bir_tests`,
  `backend_codegen_route_riscv64_return_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_add_sub_chain_defaults_to_bir`,
  `backend_codegen_route_riscv64_single_param_add_sub_chain_defaults_to_bir`,
  `backend_codegen_route_riscv64_global_load_falls_back_to_llvm`,
  `backend_runtime_return_add`,
  `backend_runtime_return_add_sub_chain`, and
  `backend_runtime_global_load`.
- Full-suite regression guard passed:
  `test_before.log` -> `test_after.log` increased total tests from `2674` to
  `2675` with zero newly failing cases.
- Added the narrowest failing tests for a bounded two-parameter i32 add helper
  across BIR lowering, BIR pipeline text routing, x86 asm emission, AArch64
  asm emission, and the production RISC-V route seam.
- Extended the bounded affine lowering and asm fast paths from one parameter
  to up to two distinct i32 parameters plus immediate adjustment, which keeps
  the BIR-default route narrow while allowing `int add_pair(int x, int y) {
  return x + y; }` to stay on the BIR path.
- Added the internal production-route test
  `backend_codegen_route_riscv64_two_param_add_defaults_to_bir` using
  `tests/c/internal/backend_route_case/two_param_add.c`.
- Targeted validation passed:
  `backend_bir_tests` and
  `backend_codegen_route_riscv64_two_param_add_defaults_to_bir`.
- Full-suite regression guard passed:
  `test_before.log` -> `test_after.log` kept total tests at `2676` with zero
  newly failing cases.
- Added coverage for the next bounded affine slice:
  a two-parameter i32 add/sub chain with immediate adjustment across BIR
  lowering, BIR text routing, x86 asm emission, AArch64 asm emission, and the
  production RISC-V route seam.
- The new bounded two-parameter affine chain already lowered and emitted
  correctly on the BIR-first path, so this slice landed as missing coverage
  rather than a code fix.
- Added the internal production-route test
  `backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir`
  using `tests/c/internal/backend_route_case/two_param_add_sub_chain.c`.
- Targeted validation passed:
  `backend_bir_tests` and
  `backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir`.
- Nearby backend route validation passed:
  `backend_codegen_route_riscv64_return_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_return_add_sub_chain_defaults_to_bir`,
  `backend_codegen_route_riscv64_single_param_add_sub_chain_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_add_defaults_to_bir`,
  `backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir`, and
  `backend_codegen_route_riscv64_global_load_falls_back_to_llvm`.
- Full-suite regression guard passed:
  `test_before.log` -> `test_after.log` increased total tests from `2676` to
  `2677` with zero newly failing cases.
- Added coverage for one more bounded affine confidence slice:
  a staged two-parameter i32 chain where an immediate adjustment happens before
  the second parameter enters and a final subtraction closes the chain.
- Added backend BIR coverage for the staged affine slice across lowering,
  RISC-V BIR text routing, x86 asm emission, and AArch64 asm emission.
- Added the internal production-route test
  `backend_codegen_route_riscv64_two_param_staged_affine_defaults_to_bir`
  using `tests/c/internal/backend_route_case/two_param_staged_affine.c`.
- Targeted validation passed:
  `backend_bir_tests` and
  `backend_codegen_route_riscv64_two_param_staged_affine_defaults_to_bir`.
- Full-suite regression guard passed:
  `test_fail_before.log` -> `test_fail_after.log` increased total tests from
  `2677` to `2678` with zero newly failing cases.
- Added an AArch64 backend regression guard that pins lowered-module terminal
  backend-IR text fallback as independent from attached legacy LIR metadata.
- Removed the dead `legacy_fallback` branch at the end of
  `src/backend/aarch64/codegen/emit.cpp`, leaving the same backend-IR text
  fallback behavior with less legacy-only plumbing.
- Targeted validation passed:
  `aarch64_backend_lowered_ir_text_fallback_ignores_legacy_lir_metadata` and
  `aarch64_backend_explicit_emit_surface_matches_structured_declared_direct_call_path`.
- Added an x86 backend regression guard that pins the lowered call-crossing
  direct-call seam as independent from attached legacy fallback `main`
  metadata once the structured backend module already matches the minimal asm
  contract.
- Removed the x86 call-crossing fast path's dependency on running shared
  regalloc over `legacy_fallback`, leaving that seam driven by backend-owned
  structured metadata alone.
- Targeted validation passed:
  `test_x86_backend_scaffold_ignores_broken_legacy_fallback_for_call_crossing_slice`,
  `test_x86_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice`,
  `test_x86_backend_keeps_renamed_call_crossing_slice_on_asm_path`,
  `test_x86_backend_scaffold_accepts_renamed_structured_call_crossing_direct_call_ir_without_signature_shims`,
  and
  `test_x86_backend_scaffold_rejects_structured_call_crossing_direct_call_when_helper_body_contract_disagrees`.
- Full-suite regression guard passed:
  `test_before.log` -> `test_after.log` kept total tests at `2678` with zero
  newly failing cases.
- Added an AArch64 backend regression guard that pins the lowered
  call-crossing direct-call asm path as independent from attached legacy
  fallback `main` metadata once the structured backend module already carries
  the required source-add and helper-call seam.
- Removed the AArch64 call-crossing fast path's dependency on consulting
  `legacy_fallback` for shared regalloc inputs, leaving that seam driven by
  backend-owned synthesized regalloc metadata alone.
- Targeted validation passed:
  `backend_lir_adapter_aarch64_tests`.
- Full-suite regression guard passed:
  `test_fail_before.log` -> `test_fail_after.log` kept total tests at `2678`
  with zero newly failing cases.
- Added a RISC-V backend pipeline regression guard that mutates attached
  legacy LIR metadata for a pre-lowered backend-module input and pins the
  passthrough seam to backend IR text when the attached legacy slice is still
  BIR-lowerable.
- Refined `src/backend/backend.cpp` so the RISC-V passthrough seam only uses
  attached legacy LLVM text for unsupported non-BIR slices; pre-lowered
  BIR-owned backend modules now keep their backend IR text surface even if
  stale legacy metadata is attached.
- Targeted validation passed:
  `backend_bir_tests`,
  `backend_codegen_route_riscv64_global_load_falls_back_to_llvm`,
  `backend_lir_adapter_aarch64_tests`, and
  `backend_lir_adapter_x86_64_tests`.
- Full-suite regression guard passed:
  `test_fail_before.log` -> `test_fail_after.log` kept total tests at `2678`
  with zero newly failing cases.

## Notes

- Keep execution backend-focused; do not broaden this into unrelated cleanup.
- Preserve a fallback only if current failures show it is still required.
- Use backend-only targeted validation before broader full-suite regression
  checks.
- Current blocker to an unconditional default flip: `lower_to_bir(...)` still
  only accepts straight-line single-block affine i32 slices with at most two
  parameters and no memory, globals, or control flow.

## Next Slice

- Continue Step 3 by reassessing the remaining AArch64 nonminimal-lowering
  fallback branch and any still-live `legacy_fallback` entry seams that
  protect unsupported slices.

## Blockers

- `lower_to_bir(...)` still lacks control flow, loads/stores, globals, and
  alloca-backed slices, so the legacy fallback remains necessary.
