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
  - Notes: The latest slice teaches both backend-module emitters to run shared `validate_backend_ir()` checks before trying handwritten asm fast paths, so structurally invalid lowered modules now fall back to backend IR text instead of being accepted only because an individual matcher missed a contract guard. New regression coverage mutates structured two-argument direct calls so `call.param_types.size()` no longer matches `call.args.size()` after compatibility text is cleared, and confirms x86 plus AArch64 both reject the malformed call metadata. The previous slice taught the x86 and AArch64 backend-module scalar-global, scalar-global store/reload, extern scalar-global, and extern global-array asm fast paths to reject contradictory structured load widths instead of trusting only the `i32` result type. Earlier slices taught the AArch64 backend-module string-literal asm fast path to reject structured byte offsets that escape the referenced string constant bounds, taught string-literal and extern global-array asm fast paths to require explicit `BackendAddressBaseKind` provenance instead of trusting symbol text and byte offsets alone, taught the scalar-global and global-ptrdiff asm fast paths to require explicit `BackendAddressBaseKind::Global` provenance rather than trusting any structured address whose symbol text happens to match a global, taught local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets, taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updated lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and made `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module call-specialized asm slices for malformed structured call/result contracts that still bypass shared validation or rely on synthesized compatibility data, then either tighten the specific matcher or record why the seam must remain transitional.
Blockers: None

Latest Iteration Note: The x86 and AArch64 backend-module emitters now validate structured backend IR before matching asm fast paths, so malformed direct-call metadata such as mismatched `param_types` versus `args` counts falls back to printed backend IR instead of slipping through call matchers that can reconstruct types from compatibility fields.

Iteration Update: Added x86 and AArch64 emitter regression coverage that mutates lowered two-argument direct calls so `param_types` no longer matches `args` after compatibility text is cleared, and updated both backend-module emitters to reject any validator-failing structured backend IR before attempting target-specific asm lowering.

Validation: `cmake --build build -j8 --target backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `ctest --test-dir build -R 'backend_lir_adapter_x86_64_tests' --output-on-failure`, `ctest --test-dir build -R 'backend_lir_adapter_aarch64_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Inspect the remaining backend-module call-specialized fast paths for residual malformed signature, result-type, or arg-shape seams that validator-first entry no longer covers narrowly enough, and either add the matcher-local guard or document why the seam must stay transitional.
