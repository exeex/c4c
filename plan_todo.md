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
  - Notes: The latest slice teaches shared backend IR validation to reject functions whose `is_declaration` bit disagrees with the structured signature linkage (`define` vs `declare`), so malformed modules cannot present declaration-only bodies or definition-only signatures to target fast paths. New regression coverage mutates a lowered extern helper declaration into a `define` signature and mutates a lowered `main` definition into a `declare` signature, then confirms the shared validator rejects both linkage-shape mismatches before backend-specific asm selection. The prior slice teaches shared backend IR validation to reject direct-global calls whose structured return type or fixed parameter types disagree with the referenced callee signature, so backend-module asm fast paths no longer have to defend that seam one matcher at a time. New regression coverage mutates lowered two-argument direct calls by changing the helper signature param type after compatibility text is cleared, and confirms the shared validator plus x86 and AArch64 backend emitters all reject the malformed structured call contract before asm specialization. The prior slice taught both backend-module emitters to run shared `validate_backend_ir()` checks before trying handwritten asm fast paths, so structurally invalid lowered modules already fell back to backend IR text instead of being accepted only because an individual matcher missed a contract guard. Earlier slices taught the x86 and AArch64 backend-module scalar-global, scalar-global store/reload, extern scalar-global, and extern global-array asm fast paths to reject contradictory structured load widths instead of trusting only the `i32` result type. Earlier slices also taught the AArch64 backend-module string-literal asm fast path to reject structured byte offsets that escape the referenced string constant bounds, taught string-literal and extern global-array asm fast paths to require explicit `BackendAddressBaseKind` provenance instead of trusting symbol text and byte offsets alone, taught the scalar-global and global-ptrdiff asm fast paths to require explicit `BackendAddressBaseKind::Global` provenance rather than trusting any structured address whose symbol text happens to match a global, taught local-array asm fast paths to require the structured `function.local_slots` contract rather than matching only on shared base symbols and hard-coded offsets, taught `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updated lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and made `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining backend-module call-specialized asm slices for residual direct-call seams beyond fixed param, return-type, and declaration-linkage validation, especially vararg/external declaration cases or matcher-local assumptions that still rely on synthesized compatibility data.
Blockers: None

Latest Iteration Note: Shared backend IR validation now rejects functions whose declaration bit disagrees with the structured signature linkage, so malformed lowered declaration/definition shapes are filtered before x86 or AArch64 target selection sees them.

Iteration Update: Added backend IR validator regressions for declaration/signature linkage mismatches by mutating a lowered extern helper declaration into `define` and a lowered `main` definition into `declare`, then rejecting both malformed shapes through shared `validate_backend_ir()` before backend-specific fast paths run.

Validation: `cmake --build build -j8 --target backend_ir_tests backend_lir_adapter_x86_64_tests backend_lir_adapter_aarch64_tests`, `./build/backend_ir_tests direct_call`, `./build/backend_ir_tests declaration_bit`, `./build/backend_lir_adapter_x86_64_tests extern_decl`, `./build/backend_lir_adapter_aarch64_tests extern_decl`, `ctest --test-dir build -R 'backend_ir_tests' --output-on-failure`, `ctest --test-dir build -R 'backend_lir_adapter_x86_64_tests' --output-on-failure`, `ctest --test-dir build -R 'backend_lir_adapter_aarch64_tests' --output-on-failure`, `ctest --test-dir build -j --output-on-failure > test_fail_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` completed with the same known steady-state full-suite result of `2667 passed / 2 failed` and no newly failing tests.

Next Slice Hint: Inspect direct extern-call and other call-specialized fast paths for structured contracts that still are not validated against referenced declarations, then either extend shared validation for those surfaces or document why the seam must remain transitional.
