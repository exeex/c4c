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
  - Notes: The latest slice tightens backend-owned address validation in `src/backend/ir_validate.cpp` so `%local`-style address bases must resolve to declared `local_slots` instead of passing purely because they start with `%`. Added backend-IR regression coverage in `tests/backend/backend_ir_tests.cpp` for undeclared local-slot load/store/ptrdiff addresses, and updated the older ptrdiff negative/misalignment probes plus the mixed global/local ptrdiff probe to use real structured local-slot metadata where those contracts are still supposed to fire. The previous slice tightened structured `ptrdiff_eq` validation so two `%local` addresses can no longer compare across different declared local-slot bases. Earlier Step 3 slices moved more backend seams off raw signature, call, binary, and global-type text and onto structured metadata while keeping backend-scoped regressions stable.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until Step 3 lands enough structured operand and call surfaces.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 3
Next Step: Re-scan the remaining structured pointer and local-object seams for any other places where backend validation or codegen still infers provenance from raw symbol shape instead of declared metadata.
Blockers: None

Latest Iteration Note: Backend IR address validation now rejects undeclared `%local`-style bases for load/store/ptrdiff instructions, so local provenance must come from declared `local_slots` instead of `%`-prefixed symbol shape alone.

Iteration Update: Structured local-slot addresses on the lowered local-array path now carry enough backend-owned provenance for same-base ptrdiff bounds, cross-slot ptrdiff mismatches, and undeclared-local rejection. The validator no longer accepts `%lv.missing`-style addresses for loads, stores, or ptrdiff just because the symbol looks local, and the mixed global/local ptrdiff regression now proves that contract using a real declared local slot rather than an undeclared placeholder.

Validation: `cmake -S . -B build`, `cmake --build build -j8`, `ctest --test-dir build -j --output-on-failure > test_before.log`, `./build/backend_ir_tests`, `./build/backend_lir_adapter_tests`, `./build/backend_lir_adapter_x86_64_tests`, `ctest --test-dir build -j --output-on-failure > test_after.log`, and `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` passed at the monotonic full-suite guard level with `2667 passed / 2 failed` before and after, preserving the same existing failures: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.

Next Slice Hint: Re-scan the remaining structured pointer-shaped seams for any other provenance gaps that still bypass backend-owned metadata, especially codegen or validator paths that may still special-case raw symbol spelling instead of structured local-slot or global records.
