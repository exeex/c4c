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
  - Notes: The latest slice teaches the x86 and AArch64 backend-module string-literal and extern global-array asm fast paths to require explicit `BackendAddressBaseKind` provenance instead of trusting symbol text and byte offsets alone. New emitter regression tests now mutate lowered string-literal loads and extern global-array loads to contradictory address kinds and confirm both targets fall back instead of silently staying on the handwritten asm path. The previous slice taught the x86 and AArch64 backend-module scalar-global and global-ptrdiff asm fast paths to require explicit `BackendAddressBaseKind::Global` provenance rather than trusting any structured address whose symbol text happens to match a global. Earlier slices taught the x86 and AArch64 backend-module local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets, taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updated lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and made `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module object-specialized asm slices for any other branches that still key on symbol spelling or offsets without first checking the structured referenced-object kind, then either tighten those matchers or record the residual seams that must remain transitional.
Blockers: None

Latest Iteration Note: String-literal and extern global-array backend-module asm fast paths on x86 and AArch64 now require the structured address-kind contract, so contradictory lowered provenance forces fallback instead of matching handwritten asm from symbol text and byte offsets alone.

Iteration Update: Added x86 and AArch64 emitter regression coverage that mutates lowered string-literal loads and extern global-array loads to contradictory address kinds and expects backend IR fallback. Updated both backends' optimized backend-module string-literal matchers to require `BackendAddressBaseKind::StringConstant` and their extern global-array matchers to require `BackendAddressBaseKind::Global`, preserving the structured object contract instead of inferring object identity from `base_symbol` text and offsets alone.

Validation: `cmake --build build -j8 --target backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `ctest --test-dir build -R 'backend_lir_adapter_(x86_64|aarch64)_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining backend-module object-specialized fast paths for any mixed-object or call/address seams that still infer object identity from `base_symbol` spelling, and either add the missing structured-kind guard or document why the seam must stay transitional.
