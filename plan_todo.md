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
  - Notes: The latest slice teaches `src/backend/ir.hpp` to carry explicit `BackendAddressBaseKind` provenance on backend addresses, updates lowering to stamp global, string-constant, and `local_slot` addresses with structured ownership, and makes `src/backend/ir_validate.cpp` reject address-kind mismatches instead of inferring object class from raw symbol spelling alone. The previous slice taught `src/backend/ir_validate.cpp` to carry explicit referenced-object kinds for globals, string constants, and `local_slots` during validation instead of partially classifying provenance from raw `%`-prefixed symbol spelling. Local-slot load/store mismatches now diagnose against local-slot element metadata directly, and earlier Step 3 slices moved more backend seams off raw signature, call, binary, and global-type text and onto structured metadata while keeping backend-scoped regressions stable.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining structured pointer and local-object seams for any codegen or validation paths that still reconstruct backend semantics from raw text fields rather than the new structured address provenance and object records.
Blockers: None

Latest Iteration Note: Backend IR addresses now carry explicit structured provenance metadata, so lowering stamps whether an address targets a global, string constant, or local slot and the validator rejects kind/symbol mismatches instead of silently re-deriving object class from symbol spelling.

Iteration Update: Structured local-slot addresses already carried explicit slot bounds and element metadata. This slice extends that work by making `BackendAddress` itself carry provenance kind metadata, updates lowering to stamp that ownership on produced addresses, and teaches the validator to enforce that structured provenance against the referenced symbol table before range/type checks. Added backend IR regression coverage for mismatched structured address kinds on both load and ptrdiff paths to pin the contract down.

Validation: `cmake -S . -B build`, `cmake --build build -j8 --target backend_ir_tests`, `./build/backend_ir_tests`, `cmake --build build -j8`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` completed for the touched slice with the same full-suite steady state before and after of `2667 passed / 2 failed`, preserving the known failures `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Re-scan the remaining structured pointer-shaped seams for any backend paths that still consult raw text fields or symbol-shape conventions after lowering, especially codegen and validator branches that can now pivot onto `BackendAddressBaseKind` or other backend-owned metadata instead.
