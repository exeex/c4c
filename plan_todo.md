Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations and implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated the touched backend suites plus a monotonic full-suite regression check.
  - Blockers: None.
- [x] Step 2: Isolate LIR syntax decoding
  - Notes: Shared lowering helpers in `src/backend/lowering/call_decode.*` now own the staged LIR call and signature parsing that had been spread across x86 and AArch64 emitters. Targeted lowering and backend tests remained green, and full-suite regression checks stayed monotonic with the same two known AArch64 object-contract failures.
  - Blockers: None.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: The latest slice teaches the x86 and AArch64 backend-module extern global-array asm fast paths to reject out-of-bounds structured byte offsets instead of trusting any non-negative offset once the symbol and address kind line up. New emitter regression tests now mutate lowered extern global-array loads to byte offset `8` against a structured `[2 x i32]` declaration and confirm both targets fall back rather than silently emitting handwritten asm that escapes the referenced array bounds. The previous slice taught the x86 and AArch64 backend-module string-literal and extern global-array asm fast paths to require explicit `BackendAddressBaseKind` provenance instead of trusting symbol text and byte offsets alone. Earlier slices taught the x86 and AArch64 backend-module scalar-global and global-ptrdiff asm fast paths to require explicit `BackendAddressBaseKind::Global` provenance rather than trusting any structured address whose symbol text happens to match a global, taught local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets, taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updated lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and made `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module object-specialized asm slices for any branches that still accept structurally impossible offsets or mixed-object call/address combinations, then either tighten those matchers or record the residual seams that must remain transitional.
Blockers: None

Latest Iteration Note: Extern global-array backend-module asm fast paths on x86 and AArch64 now require structured offsets that stay within the referenced `[N x i32]` bounds, so contradictory lowered offsets force backend-IR fallback instead of emitting handwritten asm beyond the structured array.

Iteration Update: Added x86 and AArch64 emitter regression coverage that mutates lowered extern global-array loads to byte offset `8` against the structured `[2 x i32]` fixture and expects backend IR fallback. Updated both backends' optimized backend-module extern global-array matchers to require offsets that remain aligned to `i32` elements and within the structured array element count, preserving the structured array-bounds contract instead of trusting any non-negative offset paired with the right symbol text.

Validation: `cmake --build build -j8 --target backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `ctest --test-dir build -R 'backend_lir_adapter_(x86_64|aarch64)_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_fail_before.log`, `ctest --test-dir build -j --output-on-failure > test_fail_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining backend-module object-specialized fast paths for any residual call/address seams that still accept structurally impossible offsets, widths, or mixed-object combinations after the address-kind cleanup, and either add the missing structured metadata guard or document why the seam must stay transitional.
