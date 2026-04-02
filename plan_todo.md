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
  - Notes: The latest slice teaches the x86 and AArch64 backend-module scalar-global and global-ptrdiff asm fast paths to require explicit `BackendAddressBaseKind::Global` provenance rather than trusting any structured address whose symbol text happens to match a global. New emitter regression tests now mutate lowered global load/store/extern-load and char/int ptrdiff addresses to contradictory `StringConstant` provenance and confirm both targets fall back instead of silently reusing the global asm slice from symbol-and-offset patterns alone. The previous slice taught the x86 and AArch64 backend-module local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets. Earlier Step 3 slices taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updated lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and made `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module object-specific asm fast paths, especially string-constant and other pointer/object seams, for branches that still accept contradictory structured provenance because they key on symbol spelling more than backend-owned object identity.
Blockers: None

Latest Iteration Note: Scalar-global and global-ptrdiff backend-module asm fast paths on x86 and AArch64 now require `BackendAddressBaseKind::Global`, so contradictory structured address provenance forces a safe fallback instead of letting the emitters match the optimized global slices from symbol text alone.

Iteration Update: Added x86 and AArch64 emitter regression coverage that mutates lowered scalar-global load/store/extern-load addresses plus char/int ptrdiff addresses to `StringConstant` provenance and expects fallback IR output. Updated both backends' optimized backend-module matchers to require `BackendAddressBaseKind::Global` on those slices before emitting the direct asm path, preserving the structured contract instead of inferring object identity from `base_symbol` text alone.

Validation: `cmake --build build -j8 --target backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `ctest --test-dir build -R 'backend_lir_adapter_(x86_64|aarch64)_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining backend-module object-specialized fast paths, especially string-literal and mixed object-address seams, for places where structured address kinds should be mandatory before staying on handwritten asm output.
