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
  - Notes: The latest slice teaches the x86 and AArch64 backend-module local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets. New emitter regression tests now mutate the lowered local-slot metadata to a non-`i32` shape and confirm both targets fall back instead of silently treating contradictory structured metadata as a valid local-array slice. The previous slice taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updates lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and makes `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone. Earlier Step 3 slices moved more backend seams off raw signature, call, binary, and global-type text and onto structured metadata while keeping backend-scoped regressions stable.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module pointer and local-object asm fast paths for other slices that still match only on symbol/offset patterns and can now consume `BackendAddressBaseKind` or structured object metadata directly.
Blockers: None

Latest Iteration Note: Backend-module local-array asm fast paths now require the structured `local_slots` contract, so contradictory local-slot element metadata forces a safe fallback instead of letting x86 or AArch64 match the optimized array slice from symbol-and-offset patterns alone.

Iteration Update: Structured local-slot addresses already carried explicit slot bounds and element metadata. This slice pushes that metadata into the backend-module emit seam by making the x86 and AArch64 local-array matchers require a matching `BackendLocalSlot` record for a two-element `i32` array and reject the optimized slice when the lowered slot metadata disagrees. Added target-specific emitter regression coverage that mutates the slot metadata to `i8` and confirms both backends fall back rather than emitting the stale array fast path.

Validation: `cmake -S . -B build`, `cmake --build build -j8 --target backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `./build/backend_lir_adapter_x86_64_tests`, `./build/backend_lir_adapter_aarch64_tests`, `cmake --build build -j8`, `ctest --test-dir build -R 'backend_lir_adapter_(x86_64|aarch64)_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Re-scan the remaining structured pointer-shaped backend-module seams, especially global and ptrdiff asm slices, for branches that still match only on symbols and offsets and can now pivot onto `BackendAddressBaseKind` or related backend-owned metadata.
